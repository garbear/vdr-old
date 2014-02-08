#include "FileName.h"
#include "Config.h"
#include "filesystem/Videodir.h"
#include "utils/Tools.h"
#include "devices/Remux.h"
//XXX
#include <unistd.h>

#define MAXFILESPERRECORDINGPES 255
#define RECORDFILESUFFIXPES     "/%03d.vdr"
#define MAXFILESPERRECORDINGTS  65535
#define RECORDFILESUFFIXTS      "/%05d.ts"
#define RECORDFILESUFFIXLEN 20 // some additional bytes for safety...

cFileName::cFileName(const char *FileName, bool Record, bool Blocking, bool IsPesRecording)
{
  file = NULL;
  fileNumber = 0;
  record = Record;
  blocking = Blocking;
  isPesRecording = IsPesRecording;
  // Prepare the file name:
  fileName = MALLOC(char, strlen(FileName) + RECORDFILESUFFIXLEN);
  if (!fileName) {
     esyslog("ERROR: can't copy file name '%s'", fileName);
     return;
     }
  strcpy(fileName, FileName);
  pFileNumber = fileName + strlen(fileName);
  SetOffset(1);
}

cFileName::~cFileName()
{
  Close();
  free(fileName);
}

bool cFileName::GetLastPatPmtVersions(int &PatVersion, int &PmtVersion)
{
  if (fileName && !isPesRecording) {
     // Find the last recording file:
     int Number = 1;
     for (; Number <= MAXFILESPERRECORDINGTS + 1; Number++) { // +1 to correctly set Number in case there actually are that many files
         sprintf(pFileNumber, RECORDFILESUFFIXTS, Number);
         if (!CFile::Exists(fileName)) { // file doesn't exist
            Number--;
            break;
            }
         }
     for (; Number > 0; Number--) {
         // Search for a PAT packet from the end of the file:
         cPatPmtParser PatPmtParser;
         sprintf(pFileNumber, RECORDFILESUFFIXTS, Number);
         int fd = open(fileName, O_RDONLY | O_LARGEFILE, DEFFILEMODE);
         if (fd >= 0) {
            off_t pos = lseek(fd, -TS_SIZE, SEEK_END);
            while (pos >= 0) {
                  // Read and parse the PAT/PMT:
                  uchar buf[TS_SIZE];
                  while (read(fd, buf, sizeof(buf)) == sizeof(buf)) {
                        if (buf[0] == TS_SYNC_BYTE) {
                           int Pid = TsPid(buf);
                           if (Pid == PATPID)
                              PatPmtParser.ParsePat(buf, sizeof(buf));
                           else if (PatPmtParser.IsPmtPid(Pid)) {
                              PatPmtParser.ParsePmt(buf, sizeof(buf));
                              if (PatPmtParser.GetVersions(PatVersion, PmtVersion)) {
                                 close(fd);
                                 return true;
                                 }
                              }
                           else
                              break; // PAT/PMT is always in one sequence
                           }
                        else
                           return false;
                        }
                  pos = lseek(fd, pos - TS_SIZE, SEEK_SET);
                  }
            close(fd);
            }
         else
            break;
         }
     }
  return false;
}

cUnbufferedFile *cFileName::Open(void)
{
  if (!file) {
     int BlockingFlag = blocking ? 0 : O_NONBLOCK;
     if (record) {
        dsyslog("recording to '%s'", fileName);
        file = OpenVideoFile(fileName, O_RDWR | O_CREAT | O_LARGEFILE | BlockingFlag);
        if (!file)
           LOG_ERROR_STR(fileName);
        }
     else {
        if (CFile::Exists(fileName)) {
           dsyslog("playing '%s'", fileName);
           file = cUnbufferedFile::Create(fileName, O_RDONLY | O_LARGEFILE | BlockingFlag);
           if (!file)
              LOG_ERROR_STR(fileName);
           }
        else if (errno != ENOENT)
           LOG_ERROR_STR(fileName);
        }
     }
  return file;
}

void cFileName::Close(void)
{
  if (file) {
     if (CloseVideoFile(file) < 0)
        LOG_ERROR_STR(fileName);
     file = NULL;
     }
}

cUnbufferedFile *cFileName::SetOffset(int Number, off_t Offset)
{
  if (fileNumber != Number)
     Close();
  int MaxFilesPerRecording = isPesRecording ? MAXFILESPERRECORDINGPES : MAXFILESPERRECORDINGTS;
  if (0 < Number && Number <= MaxFilesPerRecording) {
     fileNumber = uint16_t(Number);
     sprintf(pFileNumber, isPesRecording ? RECORDFILESUFFIXPES : RECORDFILESUFFIXTS, fileNumber);
     if (record) {
       if (CFile::Exists(fileName)) {
           // file exists, check if it has non-zero size
           struct __stat64 buf;
           if (CFile::Stat(fileName, &buf) == 0) {
              if (buf.st_size != 0)
                 return SetOffset(Number + 1); // file exists and has non zero size, let's try next suffix
              else {
                 // zero size file, remove it
                 dsyslog("cFileName::SetOffset: removing zero-sized file %s", fileName);
                 unlink(fileName);
                 }
              }
           else
              return SetOffset(Number + 1); // error with fstat - should not happen, just to be on the safe side
           }
        else if (errno != ENOENT) { // something serious has happened
           LOG_ERROR_STR(fileName);
           return NULL;
           }
        // found a non existing file suffix
        }
     if (Open() >= 0) {
        if (!record && Offset >= 0 && file && file->Seek(Offset, SEEK_SET) != Offset) {
           LOG_ERROR_STR(fileName);
           return NULL;
           }
        }
     return file;
     }
  esyslog("ERROR: max number of files (%d) exceeded", MaxFilesPerRecording);
  return NULL;
}

cUnbufferedFile *cFileName::NextFile(void)
{
  return SetOffset(fileNumber + 1);
}
