#include "FileName.h"
#include "Config.h"
#include "filesystem/Videodir.h"
#include "filesystem/VideoFile.h"
#include "utils/Tools.h"
#include "devices/Remux.h"
//XXX
#include <unistd.h>

#define MAXFILESPERRECORDINGPES 255
#define RECORDFILESUFFIXPES     "/%03d.vdr"
#define MAXFILESPERRECORDINGTS  65535
#define RECORDFILESUFFIXTS      "/%05d.ts"
#define RECORDFILESUFFIXLEN 20 // some additional bytes for safety...

namespace VDR
{

cFileName::cFileName(const std::string& strFileName, bool Record, bool Blocking, bool IsPesRecording)
{
  m_file = NULL;
  m_iFileNumber = 0;
  m_bRecord = Record;
  m_bBlocking = Blocking;
  m_bIsPesRecording = IsPesRecording;
  // Prepare the file name:
  m_strFileName = strFileName;
  SetOffset(1);
}

cFileName::~cFileName()
{
  Close();
}

bool cFileName::GetLastPatPmtVersions(int &PatVersion, int &PmtVersion)
{
  if (!m_strFileName.empty() && !m_bIsPesRecording)
  {
    // Find the last recording file:
    int Number = 1;
    for (; Number <= MAXFILESPERRECORDINGTS + 1; Number++)
    { // +1 to correctly set Number in case there actually are that many files
      m_strFileOffset = StringUtils::Format(RECORDFILESUFFIXTS, Number);
      if (!CFile::Exists(m_strFileName + m_strFileOffset))
      { // file doesn't exist
        Number--;
        break;
      }
    }

    m_strFileOffset = StringUtils::Format(RECORDFILESUFFIXTS, Number);

    for (; Number > 0; Number--)
    {
      // Search for a PAT packet from the end of the file:
      cPatPmtParser PatPmtParser;
      CFile file;
      if (file.Open(m_strFileName + m_strFileOffset))
      {
        off_t pos = file.Seek(-TS_SIZE, SEEK_END);
        while (pos >= 0)
        {
          // Read and parse the PAT/PMT:
          uchar buf[TS_SIZE];
          while (file.Read(buf, sizeof(buf)) == sizeof(buf))
          {
            if (buf[0] == TS_SYNC_BYTE)
            {
              int Pid = TsPid(buf);
              if (Pid == PATPID)
                PatPmtParser.ParsePat(buf, sizeof(buf));
              else if (PatPmtParser.IsPmtPid(Pid))
              {
                PatPmtParser.ParsePmt(buf, sizeof(buf));
                if (PatPmtParser.GetVersions(PatVersion, PmtVersion))
                {
                  return true;
                }
              }
              else
                break; // PAT/PMT is always in one sequence
            }
            else
              return false;
          }
          pos = file.Seek(pos - TS_SIZE, SEEK_SET);
        }
      }
      else
        break;
    }
  }
  return false;
}

CVideoFile *cFileName::Open(void)
{
  if (!m_file)
  {
    int BlockingFlag = m_bBlocking ? 0 : O_NONBLOCK;
    std::string fileName = m_strFileName + m_strFileOffset;
    if (m_bRecord)
    {
      dsyslog("recording to '%s'", fileName.c_str());
      m_file = OpenVideoFile(fileName.c_str(), O_RDWR | O_CREAT | O_LARGEFILE | BlockingFlag);
      if (!m_file)
        LOG_ERROR_STR(fileName.c_str());
    }
    else
    {
      if (CFile::Exists(fileName))
      {
        dsyslog("playing '%s'", fileName.c_str());
        m_file = CVideoFile::Create(fileName.c_str(), O_RDONLY | O_LARGEFILE | BlockingFlag);
        if (!m_file)
          LOG_ERROR_STR(fileName.c_str());
      }
      else if (errno != ENOENT)
        LOG_ERROR_STR(fileName.c_str());
    }
  }
  return m_file;
}

void cFileName::Close(void)
{
  if (m_file)
  {
    if (CloseVideoFile(m_file) < 0)
      LOG_ERROR_STR((m_strFileName + m_strFileOffset).c_str());
    m_file = NULL;
  }
}

CVideoFile *cFileName::SetOffset(int Number, off_t Offset)
{
  if (m_iFileNumber != Number)
    Close();
  int MaxFilesPerRecording = m_bIsPesRecording ? MAXFILESPERRECORDINGPES : MAXFILESPERRECORDINGTS;
  if (0 < Number && Number <= MaxFilesPerRecording)
  {
    m_iFileNumber = uint16_t(Number);
    m_strFileOffset = StringUtils::Format(m_bIsPesRecording ? RECORDFILESUFFIXPES : RECORDFILESUFFIXTS, m_iFileNumber);
    if (m_bRecord)
    {
      if (CFile::Exists(m_strFileName + m_strFileOffset))
      {
        // file exists, check if it has non-zero size
        struct __stat64 buf;
        if (CFile::Stat(m_strFileName + m_strFileOffset, &buf) == 0)
        {
          if (buf.st_size != 0)
            return SetOffset(Number + 1); // file exists and has non zero size, let's try next suffix
          else
          {
            // zero size file, remove it
            dsyslog("cFileName::SetOffset: removing zero-sized file %s", (m_strFileName + m_strFileOffset).c_str());
            CFile::Delete(m_strFileName + m_strFileOffset);
          }
        }
        else
          return SetOffset(Number + 1); // error with fstat - should not happen, just to be on the safe side
      }
      else if (errno != ENOENT)
      { // something serious has happened
        LOG_ERROR_STR((m_strFileName + m_strFileOffset).c_str());
        return NULL;
      }
      // found a non existing file suffix
    }
    if (Open() >= 0)
    {
      if (!m_bRecord && Offset >= 0 && m_file && m_file->Seek(Offset, SEEK_SET) != Offset)
      {
        LOG_ERROR_STR((m_strFileName + m_strFileOffset).c_str());
        return NULL;
      }
    }
    return m_file;
  }
  esyslog("ERROR: max number of files (%d) exceeded", MaxFilesPerRecording);
  return NULL;
}

CVideoFile *cFileName::NextFile(void)
{
  return SetOffset(m_iFileNumber + 1);
}

}
