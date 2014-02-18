#include "VideoFile.h"

#include "filesystem/File.h"
#include <fcntl.h>

//#define USE_FADVISE

#define WRITE_BUFFER KILOBYTE(800)

CVideoFile::CVideoFile(void) :
  m_curpos(0),
  m_cachedstart(0),
  m_cachedend(0),
  m_begin(0),
  m_lastpos(0),
  m_ahead(0),
  m_readahead(0),
  m_written(0),
  m_totwritten(0)
{
}

CVideoFile::~CVideoFile()
{
  Close();
}

bool CVideoFile::Open(const char *FileName, int Flags, mode_t Mode)
{
  Close();

  bool bReturn = ((Flags & O_RDWR) != 0) ? m_file.OpenForWrite(FileName) : m_file.Open(FileName, Flags);
  m_curpos = 0;
#ifdef USE_FADVISE
  begin = lastpos = ahead = 0;
  cachedstart = 0;
  cachedend = 0;
  readahead = KILOBYTE(128);
  written = 0;
  totwritten = 0;
  if (fd >= 0)
     posix_fadvise(fd, 0, 0, POSIX_FADV_RANDOM); // we could use POSIX_FADV_SEQUENTIAL, but we do our own readahead, disabling the kernel one.
#endif
  return bReturn;
}

void CVideoFile::Close(void)
{
  if (m_file.IsOpen())
  {
#ifdef USE_FADVISE
     if (totwritten)    // if we wrote anything make sure the data has hit the disk before
        fdatasync(fd);  // calling fadvise, as this is our last chance to un-cache it.
     posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);
#endif
    m_file.Close();
    return;
  }
  errno = EBADF;
}

// When replaying and going e.g. FF->PLAY the position jumps back 2..8M
// hence we do not want to drop recently accessed data at once.
// We try to handle the common cases such as PLAY->FF->PLAY, small
// jumps, moving editing marks etc.

#define FADVGRAN   KILOBYTE(4) // AKA fadvise-chunk-size; PAGE_SIZE or getpagesize(2) would also work.
#define READCHUNK  MEGABYTE(8)

void CVideoFile::SetReadAhead(size_t ra)
{
//  readahead = ra;
}

int CVideoFile::FadviseDrop(off_t Offset, off_t Len)
{
#ifdef USE_FADVISE
  // rounding up the window to make sure that not PAGE_SIZE-aligned data gets freed.
  return posix_fadvise(fd, Offset - (FADVGRAN - 1), Len + (FADVGRAN - 1) * 2, POSIX_FADV_DONTNEED);
#endif
  return 0;
}

off_t CVideoFile::Seek(off_t Offset, int Whence)
{
  if (Whence == SEEK_SET && Offset == m_curpos)
    return m_curpos;

  m_curpos = m_file.Seek(Offset, Whence);
  return m_curpos;
}

ssize_t CVideoFile::Read(void *Data, size_t Size)
{
  if (m_file.IsOpen())
  {
#ifdef USE_FADVISE
     off_t jumped = curpos-lastpos; // nonzero means we're not at the last offset
     if ((cachedstart < cachedend) && (curpos < cachedstart || curpos > cachedend)) {
        // current position is outside the cached window -- invalidate it.
        FadviseDrop(cachedstart, cachedend-cachedstart);
        cachedstart = curpos;
        cachedend = curpos;
        }
     cachedstart = ::min(cachedstart, curpos);
#endif

     ssize_t bytesRead = m_file.Read(Data, Size);
     if (bytesRead > 0)
     {
        m_curpos += bytesRead;
#ifdef USE_FADVISE
        cachedend = ::max(cachedend, curpos);

        // Read ahead:
        // no jump? (allow small forward jump still inside readahead window).
        if (jumped >= 0 && jumped <= (off_t)readahead) {
           // Trigger the readahead IO, but only if we've used at least
           // 1/2 of the previously requested area. This avoids calling
           // fadvise() after every read() call.
           if (ahead - curpos < (off_t)(readahead / 2)) {
              posix_fadvise(fd, curpos, readahead, POSIX_FADV_WILLNEED);
              ahead = curpos + readahead;
              cachedend = ::max(cachedend, ahead);
              }
           if (readahead < Size * 32) { // automagically tune readahead size.
              readahead = Size * 32;
              }
           }
        else
           ahead = curpos; // jumped -> we really don't want any readahead, otherwise e.g. fast-rewind gets in trouble.
#endif
        }
#ifdef USE_FADVISE
     if (cachedstart < cachedend) {
        if (curpos - cachedstart > READCHUNK * 2) {
           // current position has moved forward enough, shrink tail window.
           FadviseDrop(cachedstart, curpos - READCHUNK - cachedstart);
           cachedstart = curpos - READCHUNK;
           }
        else if (cachedend > ahead && cachedend - curpos > READCHUNK * 2) {
           // current position has moved back enough, shrink head window.
           FadviseDrop(curpos + READCHUNK, cachedend - (curpos + READCHUNK));
           cachedend = curpos + READCHUNK;
           }
        }
     lastpos = curpos;
#endif
     return bytesRead;
     }
  return -1;
}

ssize_t CVideoFile::Write(const void *Data, size_t Size)
{
  if (m_file.IsOpen())
  {
    ssize_t bytesWritten = m_file.Write(Data, Size);
#ifdef USE_FADVISE
     if (bytesWritten > 0) {
        begin = ::min(begin, curpos);
        curpos += bytesWritten;
        written += bytesWritten;
        lastpos = ::max(lastpos, curpos);
        if (written > WRITE_BUFFER) {
           if (lastpos > begin) {
              // Now do three things:
              // 1) Start writeback of begin..lastpos range
              // 2) Drop the already written range (by the previous fadvise call)
              // 3) Handle nonpagealigned data.
              //    This is why we double the WRITE_BUFFER; the first time around the
              //    last (partial) page might be skipped, writeback will start only after
              //    second call; the third call will still include this page and finally
              //    drop it from cache.
              off_t headdrop = ::min(begin, off_t(WRITE_BUFFER * 2));
              posix_fadvise(fd, begin - headdrop, lastpos - begin + headdrop, POSIX_FADV_DONTNEED);
              }
           begin = lastpos = curpos;
           totwritten += written;
           written = 0;
           // The above fadvise() works when writing slowly (recording), but could
           // leave cached data around when writing at a high rate, e.g. when cutting,
           // because by the time we try to flush the cached pages (above) the data
           // can still be dirty - we are faster than the disk I/O.
           // So we do another round of flushing, just like above, but at larger
           // intervals -- this should catch any pages that couldn't be released
           // earlier.
           if (totwritten > MEGABYTE(32)) {
              // It seems in some setups, fadvise() does not trigger any I/O and
              // a fdatasync() call would be required do all the work (reiserfs with some
              // kind of write gathering enabled), but the syncs cause (io) load..
              // Uncomment the next line if you think you need them.
              //fdatasync(fd);
              off_t headdrop = ::min(off_t(curpos - totwritten), off_t(totwritten * 2));
              posix_fadvise(fd, curpos - totwritten - headdrop, totwritten + headdrop, POSIX_FADV_DONTNEED);
              totwritten = 0;
              }
           }
        }
#endif
    return bytesWritten;
  }
  return -1;
}

CVideoFile *CVideoFile::Create(const char *FileName, int Flags, mode_t Mode)
{
  CVideoFile *File = new CVideoFile;
  if (!File->Open(FileName, Flags, Mode))
  {
    delete File;
    File = NULL;
  }
  return File;
}
