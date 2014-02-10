#include "Recordings.h"
#include "utils/Tools.h"
#include "filesystem/Directory.h"
#include "filesystem/Videodir.h"
#include "platform/threads/throttle.h"

using namespace PLATFORM;

cRecordings Recordings;

char *cRecordings::updateFileName = NULL;

class cRemoveDeletedRecordingsThread : public CThread {
public:
  cRemoveDeletedRecordingsThread(cRecordings* recordings) :
    m_recordings(recordings) {}

protected:
  virtual void* Process(void)
  {
    m_recordings->RemoveDeletedRecordings();
    return NULL;
  }

private:
  cRecordings* m_recordings;
};

cRecordings::cRecordings(bool Deleted)
{
  deleted = Deleted;
  lastUpdate = 0;
  state = 0;
  m_LastFreeDiskCheck = 0;
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
              r->m_deleted = time(NULL);
            CThreadLock lock(this);
            m_recordings.push_back(r);
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
  if (Wait)
  {
    Refresh(true);
    return !m_recordings.empty();
  }
  else
    CreateThread(false);
  return false;
}

cRecording *cRecordings::GetByName(const char *FileName)
{
  if (FileName)
  {
    for (std::vector<cRecording*>::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
      if (strcmp((*it)->FileName(), FileName) == 0)
        return (*it);
  }
  return NULL;
}

void cRecordings::AddByName(const char *FileName, bool TriggerUpdate)
{
  CThreadLock lock(this);
  cRecording *recording = GetByName(FileName);
  if (!recording) {
     recording = new cRecording(FileName);
     m_recordings.push_back(recording);
     ChangeState();
     if (TriggerUpdate)
        TouchUpdate();
     }
}

cRecording* cRecordings::FindByUID(uint32_t uid)
{
  //XXX use a lookup table here, and protect member access
  for (std::vector<cRecording*>::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    if ((*it)->UID() == uid)
      return (*it);
  }

  return NULL;
}

void cRecordings::DelByName(const char *FileName)
{
  CThreadLock lock(this);
  cRecording *recording = GetByName(FileName);
  if (recording)
  {
    for (std::vector<cRecording*>::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
    {
      if (!strcmp((*it)->FileName(), FileName))
      {
        m_recordings.erase(it);
        break;
      }
    }

    char *ext = strrchr(recording->m_strFileName, '.');
    if (ext)
    {
      strncpy(ext, DELEXT, strlen(ext));
      if (CFile::Exists(recording->FileName()))
      {
        recording->m_deleted = time(NULL);
        m_deletedRecordings.push_back(recording);
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
  CThreadLock lock(this);
  cRecording *recording = GetByName(FileName);
  if (recording)
     recording->ReadInfo();
}

int cRecordings::TotalFileSizeMB(bool bDeletedRecordings /* = false */)
{
  int size = 0;
  CThreadLock lock(this);
  std::vector<cRecording*>& recordings = bDeletedRecordings ? m_deletedRecordings : m_recordings;
  for (std::vector<cRecording*>::const_iterator it = recordings.begin(); it != recordings.end(); ++it)
  {
    int FileSizeMB = (*it)->FileSizeMB();
    if (FileSizeMB > 0 && (*it)->IsOnVideoDirectoryFileSystem())
      size += FileSizeMB;
  }
  return size;
}

double cRecordings::MBperMinute(void)
{
  int size = 0;
  int length = 0;
  CThreadLock lock(this);
  for (std::vector<cRecording*>::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    if ((*it)->IsOnVideoDirectoryFileSystem())
    {
      int FileSizeMB = (*it)->FileSizeMB();
      if (FileSizeMB > 0)
      {
        int LengthInSeconds = (*it)->LengthInSeconds();
        if (LengthInSeconds > 0)
        {
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
  CThreadLock lock(this);
  for (std::vector<cRecording*>::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
  {
    if (!ResumeFileName || strncmp(ResumeFileName, (*it)->FileName(), strlen((*it)->FileName())) == 0)
      (*it)->ResetResume();
  }
  ChangeState();
}

void cRecordings::Add(cRecording* recording)
{
  CThreadLock lock(this);
  m_recordings.push_back(recording);
}

void cRecordings::Clear(void)
{
  CThreadLock lock(this);
  for (std::vector<cRecording*>::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
    delete *it;
  m_recordings.clear();
}

bool cRecordings::RemoveDeletedRecordings(void)
{
  bool bReturn(false);
  // Make sure only one instance of VDR does this:
  cLockFile LockFile(VideoDirectory);
  if (LockFile.Lock())
  {
    CThreadLock lock(this);
    for (std::vector<cRecording*>::iterator it = m_deletedRecordings.begin(); it != m_deletedRecordings.end(); ++it)
    {
      if (PLATFORM::cIoThrottle::Engaged())
        return bReturn;

      if ((*it)->Deleted() && time(NULL) - (*it)->Deleted() > DELETEDLIFETIME)
      {
        (*it)->Remove();
        bReturn = true;
        it = m_deletedRecordings.erase(it);
      }
    }

    if (bReturn)
    {
      const char *IgnoreFiles[] = { SORTMODEFILE, NULL };
      RemoveEmptyVideoDirectories(IgnoreFiles);
    }
  }
  return bReturn;
}

bool cRecordings::RemoveDeletedRecording(time_t& LastFreeDiskCheck, int Factor)
{
  std::vector<cRecording*>::iterator deleteRecording = m_deletedRecordings.end();
  for (std::vector<cRecording*>::iterator it = m_deletedRecordings.begin(); it != m_deletedRecordings.end(); ++it)
  {
    if ((*it)->IsOnVideoDirectoryFileSystem())
    { // only remove recordings that will actually increase the free video disk space
      if (deleteRecording == m_deletedRecordings.end() || (*it)->Start() < (*deleteRecording)->Start())
        deleteRecording = it;
    }
  }
  if (deleteRecording != m_deletedRecordings.end())
  {
    if ((*deleteRecording)->Remove())
      LastFreeDiskCheck += REMOVELATENCY / Factor;

    delete *deleteRecording;
    m_deletedRecordings.erase(deleteRecording);
    return true;
  }

  return false;
}

bool cRecordings::RemoveOldestRecording(int Priority)
{
  CThreadLock RecordingsLock(this);
  if (!m_recordings.empty())
  {
    std::vector<cRecording*>::iterator deleteRecording = m_recordings.end();
    for (std::vector<cRecording*>::iterator it = m_recordings.begin(); it != m_recordings.end(); ++it)
    {
      if ((*it)->IsOnVideoDirectoryFileSystem())
      { // only delete recordings that will actually increase the free video disk space
        if (!(*it)->IsEdited() && (*it)->Lifetime() < MAXLIFETIME)
        { // edited recordings and recordings with MAXLIFETIME live forever
          if (((*it)->Lifetime() == 0 && Priority > (*it)->Priority()) || // the recording has no guaranteed lifetime and the new recording has higher priority
              ((*it)->Lifetime() > 0
                  && (time(NULL) - (*it)->Start()) / SECSINDAY
                      >= (*it)->Lifetime()))
          { // the recording's guaranteed lifetime has expired
            if (deleteRecording != m_recordings.end())
            {
              if ((*it)->Priority() < (*deleteRecording)->Priority()
                  || ((*it)->Priority() == (*deleteRecording)->Priority()
                      && (*it)->Start() < (*deleteRecording)->Start()))
                deleteRecording = it; // in any case we delete the one with the lowest priority (or the older one in case of equal priorities)
            }
            else
            {
              deleteRecording = it;
            }
          }
        }
      }
    }

    if (deleteRecording != m_recordings.end() && (*deleteRecording)->Delete())
    {
      delete *deleteRecording;
      m_recordings.erase(deleteRecording);
      return true;
    }
  }

  return false;
}

void cRecordings::AssertFreeDiskSpace(int Priority, bool Force)
{
  static CMutex Mutex;
  CLockObject lock(Mutex);

  // With every call to this function we try to actually remove
  // a file, or mark a file for removal ("delete" it), so that
  // it will get removed during the next call.
  int Factor = (Priority == -1) ? 10 : 1;
  if (Force || time(NULL) - m_LastFreeDiskCheck > DISKCHECKDELTA / Factor)
  {
    if (!VideoFileSpaceAvailable(MINDISKSPACE))
    {
      // Make sure only one instance of VDR does this:
      cLockFile LockFile(VideoDirectory);
      if (!LockFile.Lock())
        return;

      // Remove the oldest file that has been "deleted":
      isyslog("low disk space while recording, trying to remove a deleted recording...");

      CThreadLock DeletedRecordingsLock(this);
      if (!m_deletedRecordings.empty() && RemoveDeletedRecording(m_LastFreeDiskCheck, Factor))
        return;

      // No "deleted" files to remove, so let's see if we can delete a recording:
      if (Priority > 0)
      {
        isyslog("...no deleted recording found, trying to delete an old recording...");
        if (RemoveOldestRecording(Priority))
          return;

        // Unable to free disk space, but there's nothing we can do about that...
        isyslog("...no old recording found, giving up");
      }
      else
      {
        isyslog("...no deleted recording found, priority %d too low to trigger deleting an old recording", Priority);
      }
      esyslog(tr("Low disk space!"));
    }
    m_LastFreeDiskCheck = time(NULL);
  }
}

size_t cRecordings::Size(void) const
{
  return m_recordings.size();
}

std::vector<cRecording*> cRecordings::Recordings(void) const
{
  return m_recordings;
}
