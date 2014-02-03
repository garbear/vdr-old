#include "Recordings.h"
#include "utils/Tools.h"
#include "filesystem/Directory.h"
#include "filesystem/Videodir.h"

using namespace PLATFORM;

cRecordings Recordings;

char *cRecordings::updateFileName = NULL;

cRecordings::cRecordings(bool Deleted)
{
  deleted = Deleted;
  lastUpdate = 0;
  state = 0;
}

cRecordings::~cRecordings()
{
  StopThread(3000);
}

void* cRecordings::Process(void)
{
  Refresh();
  return NULL;
}

const char *cRecordings::UpdateFileName(void)
{
  if (!updateFileName)
     updateFileName = strdup(AddDirectory(VideoDirectory, ".update"));
  return updateFileName;
}

void cRecordings::Refresh(bool Foreground)
{
  lastUpdate = time(NULL); // doing this first to make sure we don't miss anything
  {
    CThreadLock lock(this);
    Clear();
    ChangeState();
  }
  ScanVideoDir(VideoDirectory, Foreground);
}

void cRecordings::ScanVideoDir(const std::string& strDirName, bool Foreground, int LinkLevel)
{
  DirectoryListing dirListing;
  CDirectory::GetDirectory(strDirName, dirListing);
  dsyslog("scanning in directory '%s' for recordings", strDirName.c_str());
  for (DirectoryListing::const_iterator it = dirListing.begin(); it != dirListing.end() && !IsStopped(); ++it)
  {
    std::string filename = *AddDirectory(strDirName.c_str(), (*it).Name().c_str());
    struct __stat64 st;
    if (CFile::Stat(filename, &st) == 0)
    {
      int Link = 0;
      if (S_ISLNK(st.st_mode))
      {
        if (LinkLevel > MAX_LINK_LEVEL)
        {
          isyslog("max link level exceeded - not scanning %s", filename.c_str());
          continue;
        }

        Link = 1;
        if (CFile::Stat(filename, &st) != 0)
          continue;
      }
      if (S_ISDIR(st.st_mode))
      {
        if (endswith(filename.c_str(), deleted ? DELEXT : RECEXT))
        {
          cRecording *r = new cRecording(filename.c_str());
          if (r->Name())
          {
            r->NumFrames(); // initializes the numFrames member XXX wtf
            r->FileSizeMB(); // initializes the fileSizeMB member XXX
            if (deleted)
              r->deleted = time(NULL);
            CThreadLock lock(this);
            Add(r);
            ChangeState();
          }
          else
          {
              delete r;
          }
        }
        else
        {
          ScanVideoDir(filename.c_str(), Foreground, LinkLevel + Link);
        }
      }
    }
  }
}

bool cRecordings::StateChanged(int &State)
{
  int NewState = state;
  bool Result = State != NewState;
  State = state;
  return Result;
}

void cRecordings::TouchUpdate(void)
{
  bool needsUpdate = NeedsUpdate();
  TouchFile(UpdateFileName());
  if (!needsUpdate)
     lastUpdate = time(NULL); // make sure we don't trigger ourselves
}

bool cRecordings::NeedsUpdate(void)
{
  time_t lastModified = LastModifiedTime(UpdateFileName());
  if (lastModified > time(NULL))
     return false; // somebody's clock isn't running correctly
  return lastUpdate < lastModified;
}

bool cRecordings::Update(bool Wait)
{
  if (Wait) {
     Refresh(true);
     return Count() > 0;
     }
  else
     CreateThread(false);
  return false;
}

cRecording *cRecordings::GetByName(const char *FileName)
{
  if (FileName) {
     for (cRecording *recording = First(); recording; recording = Next(recording)) {
         if (strcmp(recording->FileName(), FileName) == 0)
            return recording;
         }
     }
  return NULL;
}

void cRecordings::AddByName(const char *FileName, bool TriggerUpdate)
{
  LOCK_THREAD;
  cRecording *recording = GetByName(FileName);
  if (!recording) {
     recording = new cRecording(FileName);
     Add(recording);
     ChangeState();
     if (TriggerUpdate)
        TouchUpdate();
     }
}

cRecording* cRecordings::FindByUID(uint32_t uid)
{
  //XXX use a lookup table here, and protect member access
  for (cRecording *recording = First(); recording; recording = Next(recording))
  {
    if (recording->UID() == uid)
      return recording;
  }

  return NULL;
}

void cRecordings::DelByName(const char *FileName)
{
  LOCK_THREAD;
  cRecording *recording = GetByName(FileName);
  if (recording) {
     CThreadLock DeletedRecordingsLock(&DeletedRecordings);
     Del(recording, false);
     char *ext = strrchr(recording->fileName, '.');
     if (ext) {
        strncpy(ext, DELEXT, strlen(ext));
        if (CFile::Exists(recording->FileName())) {
           recording->deleted = time(NULL);
           DeletedRecordings.Add(recording);
           recording = NULL; // to prevent it from being deleted below
           }
        }
     delete recording;
     ChangeState();
     TouchUpdate();
     }
}

void cRecordings::UpdateByName(const char *FileName)
{
  LOCK_THREAD;
  cRecording *recording = GetByName(FileName);
  if (recording)
     recording->ReadInfo();
}

int cRecordings::TotalFileSizeMB(void)
{
  int size = 0;
  LOCK_THREAD;
  for (cRecording *recording = First(); recording; recording = Next(recording)) {
      int FileSizeMB = recording->FileSizeMB();
      if (FileSizeMB > 0 && recording->IsOnVideoDirectoryFileSystem())
         size += FileSizeMB;
      }
  return size;
}

double cRecordings::MBperMinute(void)
{
  int size = 0;
  int length = 0;
  LOCK_THREAD;
  for (cRecording *recording = First(); recording; recording = Next(recording)) {
      if (recording->IsOnVideoDirectoryFileSystem()) {
         int FileSizeMB = recording->FileSizeMB();
         if (FileSizeMB > 0) {
            int LengthInSeconds = recording->LengthInSeconds();
            if (LengthInSeconds > 0) {
               size += FileSizeMB;
               length += LengthInSeconds;
               }
            }
         }
      }
  return (size && length) ? double(size) * 60 / length : -1;
}

void cRecordings::ResetResume(const char *ResumeFileName)
{
  LOCK_THREAD;
  for (cRecording *recording = First(); recording; recording = Next(recording)) {
      if (!ResumeFileName || strncmp(ResumeFileName, recording->FileName(), strlen(recording->FileName())) == 0)
         recording->ResetResume();
      }
  ChangeState();
}

void cRecordings::ClearSortNames(void)
{
  LOCK_THREAD;
  for (cRecording *recording = First(); recording; recording = Next(recording))
      recording->ClearSortName();
}
