#include "Marks.h"
#include "Recording.h"
#include "IndexFile.h"
#include "RecordingUserCommand.h"

using namespace PLATFORM;

bool cMarks::Load(const char *RecordingFileName, double FramesPerSecond, bool IsPesRecording)
{
  recordingFileName = RecordingFileName;
  fileName = AddDirectory(RecordingFileName, IsPesRecording ? MARKSFILESUFFIX ".vdr" : MARKSFILESUFFIX);
  framesPerSecond = FramesPerSecond;
  isPesRecording = IsPesRecording;
  nextUpdate = 0;
  lastFileTime = -1; // the first call to Load() must take place!
  lastChange = 0;
  return Update();
}

bool cMarks::Update(void)
{
  time_t t = time(NULL);
  if (t > nextUpdate) {
     time_t LastModified = LastModifiedTime(fileName);
     if (LastModified != lastFileTime) // change detected, or first run
        lastChange = LastModified > 0 ? LastModified : t;
     int d = t - lastChange;
     if (d < 60)
        d = 1; // check frequently if the file has just been modified
     else if (d < 3600)
        d = 10; // older files are checked less frequently
     else
        d /= 360; // phase out checking for very old files
     nextUpdate = t + d;
     if (LastModified != lastFileTime) { // change detected, or first run
        lastFileTime = LastModified;
        if (lastFileTime == t)
           lastFileTime--; // make sure we don't miss updates in the remaining second
        CLockObject lock(MutexMarkFramesPerSecond);
        MarkFramesPerSecond = framesPerSecond;
        if (cConfig<cMark>::Load(fileName)) {
           Align();
           Sort();
           return true;
           }
        }
     }
  return false;
}

bool cMarks::Save(void)
{
  if (cConfig<cMark>::Save()) {
     lastFileTime = LastModifiedTime(fileName);
     return true;
     }
  return false;
}

void cMarks::Align(void)
{
  cIndexFile IndexFile(recordingFileName, false, isPesRecording);
  for (cMark *m = First(); m; m = Next(m)) {
      int p = IndexFile.GetClosestIFrame(m->Position());
      if (int d = m->Position() - p) {
         isyslog("aligned editing mark %s to %s (off by %d frame%s)", cMark::IndexToHMSF(m->Position(), true, framesPerSecond).c_str(), cMark::IndexToHMSF(p, true, framesPerSecond).c_str(), d, abs(d) > 1 ? "s" : "");
         m->SetPosition(p);
         }
      }
}

void cMarks::Sort(void)
{
  for (cMark *m1 = First(); m1; m1 = Next(m1)) {
      for (cMark *m2 = Next(m1); m2; m2 = Next(m2)) {
          if (m2->Position() < m1->Position()) {
             std::swap(m1->m_iPosition, m2->m_iPosition);
             std::swap(m1->m_strComment, m2->m_strComment);
             }
          }
      }
}

void cMarks::Add(int Position)
{
  cConfig<cMark>::Add(new cMark(Position, NULL, framesPerSecond));
  Sort();
}

cMark *cMarks::Get(int Position)
{
  for (cMark *mi = First(); mi; mi = Next(mi)) {
      if (mi->Position() == Position)
         return mi;
      }
  return NULL;
}

cMark *cMarks::GetPrev(int Position)
{
  for (cMark *mi = Last(); mi; mi = Prev(mi)) {
      if (mi->Position() < Position)
         return mi;
      }
  return NULL;
}

cMark *cMarks::GetNext(int Position)
{
  for (cMark *mi = First(); mi; mi = Next(mi)) {
      if (mi->Position() > Position)
         return mi;
      }
  return NULL;
}

cMark *cMarks::GetNextBegin(cMark *EndMark)
{
  cMark *BeginMark = EndMark ? Next(EndMark) : First();
  if (BeginMark) {
     while (cMark *NextMark = Next(BeginMark)) {
           if (BeginMark->Position() == NextMark->Position()) { // skip Begin/End at the same position
              if (!(BeginMark = Next(NextMark)))
                 break;
              }
           else
              break;
           }
     }
  return BeginMark;
}

cMark *cMarks::GetNextEnd(cMark *BeginMark)
{
  if (!BeginMark)
     return NULL;
  cMark *EndMark = Next(BeginMark);
  if (EndMark) {
     while (cMark *NextMark = Next(EndMark)) {
           if (EndMark->Position() == NextMark->Position()) { // skip End/Begin at the same position
              if (!(EndMark = Next(NextMark)))
                 break;
              }
           else
              break;
           }
     }
  return EndMark;
}

int cMarks::GetNumSequences(void)
{
  int NumSequences = 0;
  if (cMark *BeginMark = GetNextBegin()) {
     while (cMark *EndMark = GetNextEnd(BeginMark)) {
           NumSequences++;
           BeginMark = GetNextBegin(EndMark);
           }
     if (BeginMark) {
        NumSequences++; // the last sequence had no actual "end" mark
        if (NumSequences == 1 && BeginMark->Position() == 0)
           NumSequences = 0; // there is only one actual "begin" mark at offset zero, and no actual "end" mark
        }
     }
  return NumSequences;
}
