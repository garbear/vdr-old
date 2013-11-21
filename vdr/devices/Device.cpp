/*
 * device.c: The basic device interface
 *
 * See the main source file 'vdr.c' for copyright information and
 * how to reach the author.
 *
 * $Id: device.c 2.74.1.2 2013/08/22 10:35:30 kls Exp $
 */

#include "device.h"
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "audio.h"
#include "channels.h"
#include "i18n.h"
#include "player.h"
#include "receiver.h"
#include "status.h"
#include "transfer.h"

#include "vdr/utils/UTF8Utils.h"

// --- cLiveSubtitle ---------------------------------------------------------

class cLiveSubtitle : public cReceiver {
protected:
  virtual void Receive(uchar *Data, int Length);
public:
  cLiveSubtitle(int SPid);
  virtual ~cLiveSubtitle();
  };

cLiveSubtitle::cLiveSubtitle(int SPid)
{
  AddPid(SPid);
}

cLiveSubtitle::~cLiveSubtitle()
{
  cReceiver::Detach();
}

void cLiveSubtitle::Receive(uchar *Data, int Length)
{
  if (cDevice::PrimaryDevice())
     cDevice::PrimaryDevice()->PlayTs(Data, Length);
}

// --- cDeviceHook -----------------------------------------------------------

cDeviceHook::cDeviceHook(void)
{
  cDevice::deviceHooks.Add(this);
}

bool cDeviceHook::DeviceProvidesTransponder(const cDevice *Device, const cChannel *Channel) const
{
  return true;
}

// --- cDevice ---------------------------------------------------------------

// The minimum number of unknown PS1 packets to consider this a "pre 1.3.19 private stream":
#define MIN_PRE_1_3_19_PRIVATESTREAM 10

int cDevice::numDevices = 0;
int cDevice::useDevice = 0;
int cDevice::nextCardIndex = 0;
int cDevice::currentChannel = 1;
cDevice *cDevice::device[MAXDEVICES] = { NULL };
cDevice *cDevice::primaryDevice = NULL;
cList<cDeviceHook> cDevice::deviceHooks;

cDevice::cDevice(void)
:patPmtParser(true)
{
  cardIndex = nextCardIndex++;
  dsyslog("new device number %d", CardIndex() + 1);

  SetDescription("receiver on device %d", CardIndex() + 1);

  mute = false;
  volume = Setup.CurrentVolume;

  sectionHandler = NULL;
  eitFilter = NULL;
  patFilter = NULL;
  sdtFilter = NULL;
  nitFilter = NULL;

  camSlot = NULL;
  startScrambleDetection = 0;

  occupiedTimeout = 0;

  player = NULL;
  isPlayingVideo = false;
  keepTracks = false; // used in ClrAvailableTracks()!
  ClrAvailableTracks();
  currentAudioTrack = ttNone;
  currentAudioTrackMissingCount = 0;
  currentSubtitleTrack = ttNone;
  liveSubtitle = NULL;
  dvbSubtitleConverter = NULL;
  autoSelectPreferredSubtitleLanguage = true;

  for (int i = 0; i < MAXRECEIVERS; i++)
      receiver[i] = NULL;

  if (numDevices < MAXDEVICES)
     device[numDevices++] = this;
  else
     esyslog("ERROR: too many devices!");
}

cDevice::~cDevice()
{
  Detach(player);
  DetachAllReceivers();
  delete liveSubtitle;
  delete dvbSubtitleConverter;
  if (this == primaryDevice)
     primaryDevice = NULL;
}

bool cDevice::WaitForAllDevicesReady(int Timeout)
{
  for (time_t t0 = time(NULL); time(NULL) - t0 < Timeout; ) {
      bool ready = true;
      for (int i = 0; i < numDevices; i++) {
          if (device[i] && !device[i]->Ready()) {
             ready = false;
             cCondWait::SleepMs(100);
             }
          }
      if (ready)
         return true;
      }
  return false;
}

void cDevice::SetUseDevice(int n)
{
  if (n < MAXDEVICES)
     useDevice |= (1 << n);
}

int cDevice::NextCardIndex(int n)
{
  if (n > 0) {
     nextCardIndex += n;
     if (nextCardIndex >= MAXDEVICES)
        esyslog("ERROR: nextCardIndex too big (%d)", nextCardIndex);
     }
  else if (n < 0)
     esyslog("ERROR: invalid value in nextCardIndex(%d)", n);
  return nextCardIndex;
}

int cDevice::DeviceNumber(void) const
{
  for (int i = 0; i < numDevices; i++) {
      if (device[i] == this)
         return i;
      }
  return -1;
}

cString cDevice::DeviceType(void) const
{
  return "";
}

cString cDevice::DeviceName(void) const
{
  return "";
}

void cDevice::MakePrimaryDevice(bool On)
{
  if (!On) {
     DELETENULL(liveSubtitle);
     DELETENULL(dvbSubtitleConverter);
     }
}

bool cDevice::SetPrimaryDevice(int n)
{
  n--;
  if (0 <= n && n < numDevices && device[n]) {
     isyslog("setting primary device to %d", n + 1);
     if (primaryDevice)
        primaryDevice->MakePrimaryDevice(false);
     primaryDevice = device[n];
     primaryDevice->MakePrimaryDevice(true);
     primaryDevice->SetVideoFormat(Setup.VideoFormat);
     primaryDevice->SetVolumeDevice(Setup.CurrentVolume);
     return true;
     }
  esyslog("ERROR: invalid primary device number: %d", n + 1);
  return false;
}

bool cDevice::HasDecoder(void) const
{
  return false;
}

cSpuDecoder *cDevice::GetSpuDecoder(void)
{
  return NULL;
}

cDevice *cDevice::ActualDevice(void)
{
  cDevice *d = cTransferControl::ReceiverDevice();
  if (!d)
     d = PrimaryDevice();
  return d;
}

cDevice *cDevice::GetDevice(int Index)
{
  return (0 <= Index && Index < numDevices) ? device[Index] : NULL;
}

static int GetClippedNumProvidedSystems(int AvailableBits, cDevice *Device)
{
  int MaxNumProvidedSystems = (1 << AvailableBits) - 1;
  int NumProvidedSystems = Device->NumProvidedSystems();
  if (NumProvidedSystems > MaxNumProvidedSystems) {
     esyslog("ERROR: device %d supports %d modulation systems but cDevice::GetDevice() currently only supports %d delivery systems which should be fixed", Device->CardIndex() + 1, NumProvidedSystems, MaxNumProvidedSystems);
     NumProvidedSystems = MaxNumProvidedSystems;
     }
  else if (NumProvidedSystems <= 0) {
     esyslog("ERROR: device %d reported an invalid number (%d) of supported delivery systems - assuming 1", Device->CardIndex() + 1, NumProvidedSystems);
     NumProvidedSystems = 1;
     }
  return NumProvidedSystems;
}

cDevice *cDevice::GetDevice(const cChannel *Channel, int Priority, bool LiveView, bool Query)
{
  // Collect the current priorities of all CAM slots that can decrypt the channel:
  int NumCamSlots = CamSlots.Count();
  int SlotPriority[NumCamSlots];
  int NumUsableSlots = 0;
  bool InternalCamNeeded = false;
  if (Channel->Ca() >= CA_ENCRYPTED_MIN) {
     for (cCamSlot *CamSlot = CamSlots.First(); CamSlot; CamSlot = CamSlots.Next(CamSlot)) {
         SlotPriority[CamSlot->Index()] = MAXPRIORITY + 1; // assumes it can't be used
         if (CamSlot->ModuleStatus() == msReady) {
            if (CamSlot->ProvidesCa(Channel->Caids())) {
               if (!ChannelCamRelations.CamChecked(Channel->GetChannelID(), CamSlot->SlotNumber())) {
                  SlotPriority[CamSlot->Index()] = CamSlot->Priority();
                  NumUsableSlots++;
                  }
               }
            }
         }
     if (!NumUsableSlots)
        InternalCamNeeded = true; // no CAM is able to decrypt this channel
     }

  bool NeedsDetachReceivers = false;
  cDevice *d = NULL;
  cCamSlot *s = NULL;

  uint32_t Impact = 0xFFFFFFFF; // we're looking for a device with the least impact
  for (int j = 0; j < NumCamSlots || !NumUsableSlots; j++) {
      if (NumUsableSlots && SlotPriority[j] > MAXPRIORITY)
         continue; // there is no CAM available in this slot
      for (int i = 0; i < numDevices; i++) {
          if (Channel->Ca() && Channel->Ca() <= CA_DVB_MAX && Channel->Ca() != device[i]->CardIndex() + 1)
             continue; // a specific card was requested, but not this one
          bool HasInternalCam = device[i]->HasInternalCam();
          if (InternalCamNeeded && !HasInternalCam)
             continue; // no CAM is able to decrypt this channel and the device uses vdr handled CAMs
          if (NumUsableSlots && !HasInternalCam && !CamSlots.Get(j)->Assign(device[i], true))
             continue; // CAM slot can't be used with this device
          bool ndr;
          if (device[i]->ProvidesChannel(Channel, Priority, &ndr)) { // this device is basically able to do the job
             if (NumUsableSlots && !HasInternalCam && device[i]->CamSlot() && device[i]->CamSlot() != CamSlots.Get(j))
                ndr = true; // using a different CAM slot requires detaching receivers
             // Put together an integer number that reflects the "impact" using
             // this device would have on the overall system. Each condition is represented
             // by one bit in the number (or several bits, if the condition is actually
             // a numeric value). The sequence in which the conditions are listed corresponds
             // to their individual severity, where the one listed first will make the most
             // difference, because it results in the most significant bit of the result.
             uint32_t imp = 0;
             imp <<= 1; imp |= LiveView ? !device[i]->IsPrimaryDevice() || ndr : 0;                                  // prefer the primary device for live viewing if we don't need to detach existing receivers
             imp <<= 1; imp |= !device[i]->Receiving() && (device[i] != cTransferControl::ReceiverDevice() || device[i]->IsPrimaryDevice()) || ndr; // use receiving devices if we don't need to detach existing receivers, but avoid primary device in local transfer mode
             imp <<= 1; imp |= device[i]->Receiving();                                                               // avoid devices that are receiving
             imp <<= 4; imp |= GetClippedNumProvidedSystems(4, device[i]) - 1;                                       // avoid cards which support multiple delivery systems
             imp <<= 1; imp |= device[i] == cTransferControl::ReceiverDevice();                                      // avoid the Transfer Mode receiver device
             imp <<= 8; imp |= device[i]->Priority() - IDLEPRIORITY;                                                 // use the device with the lowest priority (- IDLEPRIORITY to assure that values -100..99 can be used)
             imp <<= 8; imp |= ((NumUsableSlots && !HasInternalCam) ? SlotPriority[j] : IDLEPRIORITY) - IDLEPRIORITY;// use the CAM slot with the lowest priority (- IDLEPRIORITY to assure that values -100..99 can be used)
             imp <<= 1; imp |= ndr;                                                                                  // avoid devices if we need to detach existing receivers
             imp <<= 1; imp |= (NumUsableSlots || InternalCamNeeded) ? 0 : device[i]->HasCi();                       // avoid cards with Common Interface for FTA channels
             imp <<= 1; imp |= device[i]->AvoidRecording();                                                          // avoid SD full featured cards
             imp <<= 1; imp |= (NumUsableSlots && !HasInternalCam) ? !ChannelCamRelations.CamDecrypt(Channel->GetChannelID(), j + 1) : 0; // prefer CAMs that are known to decrypt this channel
             imp <<= 1; imp |= device[i]->IsPrimaryDevice();                                                         // avoid the primary device
             if (imp < Impact) {
                // This device has less impact than any previous one, so we take it.
                Impact = imp;
                d = device[i];
                NeedsDetachReceivers = ndr;
                if (NumUsableSlots && !HasInternalCam)
                   s = CamSlots.Get(j);
                }
             }
          }
      if (!NumUsableSlots)
         break; // no CAM necessary, so just one loop over the devices
      }
  if (d && !Query) {
     if (NeedsDetachReceivers)
        d->DetachAllReceivers();
     if (s) {
        if (s->Device() != d) {
           if (s->Device())
              s->Device()->DetachAllReceivers();
           if (d->CamSlot())
              d->CamSlot()->Assign(NULL);
           s->Assign(d);
           }
        }
     else if (d->CamSlot() && !d->CamSlot()->IsDecrypting())
        d->CamSlot()->Assign(NULL);
     }
  return d;
}

cDevice *cDevice::GetDeviceForTransponder(const cChannel *Channel, int Priority)
{
  cDevice *Device = NULL;
  for (int i = 0; i < cDevice::NumDevices(); i++) {
      if (cDevice *d = cDevice::GetDevice(i)) {
         if (d->IsTunedToTransponder(Channel))
            return d; // if any device is tuned to the transponder, we're done
         if (d->ProvidesTransponder(Channel)) {
            if (d->MaySwitchTransponder(Channel))
               Device = d; // this device may switch to the transponder without disturbing any receiver or live view
            else if (!d->Occupied() && d->MaySwitchTransponder(Channel)) { // MaySwitchTransponder() implicitly calls Occupied()
               if (d->Priority() < Priority && (!Device || d->Priority() < Device->Priority()))
                  Device = d; // use this one only if no other with less impact can be found
               }
            }
         }
      }
  return Device;
}

void cDevice::Shutdown(void)
{
  deviceHooks.Clear();
  for (int i = 0; i < numDevices; i++) {
      delete device[i];
      device[i] = NULL;
      }
}

void cDevice::SetVideoDisplayFormat(eVideoDisplayFormat VideoDisplayFormat)
{
  cSpuDecoder *spuDecoder = GetSpuDecoder();
  if (spuDecoder) {
     if (Setup.VideoFormat)
        spuDecoder->setScaleMode(cSpuDecoder::eSpuNormal);
     else {
        switch (VideoDisplayFormat) {
               case vdfPanAndScan:
                    spuDecoder->setScaleMode(cSpuDecoder::eSpuPanAndScan);
                    break;
               case vdfLetterBox:
                    spuDecoder->setScaleMode(cSpuDecoder::eSpuLetterBox);
                    break;
               case vdfCenterCutOut:
                    spuDecoder->setScaleMode(cSpuDecoder::eSpuNormal);
                    break;
               default: esyslog("ERROR: invalid value for VideoDisplayFormat '%d'", VideoDisplayFormat);
               }
        }
     }
}

void cDevice::SetVideoFormat(bool VideoFormat16_9)
{
}

eVideoSystem cDevice::GetVideoSystem(void)
{
  return vsPAL;
}

void cDevice::GetVideoSize(int &Width, int &Height, double &VideoAspect)
{
  Width = 0;
  Height = 0;
  VideoAspect = 1.0;
}

void cDevice::GetOsdSize(int &Width, int &Height, double &PixelAspect)
{
  Width = 720;
  Height = 480;
  PixelAspect = 1.0;
}

void cDevice::StartSectionHandler(void)
{
  if (!sectionHandler) {
     sectionHandler = new cSectionHandler(this);
     AttachFilter(eitFilter = new cEitFilter);
     AttachFilter(patFilter = new cPatFilter);
     AttachFilter(sdtFilter = new cSdtFilter(patFilter));
     AttachFilter(nitFilter = new cNitFilter);
     }
}

void cDevice::StopSectionHandler(void)
{
  if (sectionHandler) {
     delete nitFilter;
     delete sdtFilter;
     delete patFilter;
     delete eitFilter;
     delete sectionHandler;
     nitFilter = NULL;
     sdtFilter = NULL;
     patFilter = NULL;
     eitFilter = NULL;
     sectionHandler = NULL;
     }
}

int cDevice::OpenFilter(u_short Pid, u_char Tid, u_char Mask)
{
  return -1;
}

int cDevice::ReadFilter(int Handle, void *Buffer, size_t Length)
{
  return safe_read(Handle, Buffer, Length);
}

void cDevice::CloseFilter(int Handle)
{
  close(Handle);
}

void cDevice::AttachFilter(cFilter *Filter)
{
  if (sectionHandler)
     sectionHandler->Attach(Filter);
}

void cDevice::Detach(cFilter *Filter)
{
  if (sectionHandler)
     sectionHandler->Detach(Filter);
}

bool cDevice::DeviceHooksProvidesTransponder(const cChannel *Channel) const
{
  cDeviceHook *Hook = deviceHooks.First();
  while (Hook) {
        if (!Hook->DeviceProvidesTransponder(this, Channel))
           return false;
        Hook = deviceHooks.Next(Hook);
        }
  return true;
}

void cDevice::ClrAvailableTracks(bool DescriptionsOnly, bool IdsOnly)
{
  if (keepTracks)
     return;
  if (DescriptionsOnly) {
     for (int i = ttNone; i < ttMaxTrackTypes; i++)
         *availableTracks[i].description = 0;
     }
  else {
     if (IdsOnly) {
        for (int i = ttNone; i < ttMaxTrackTypes; i++)
            availableTracks[i].id = 0;
        }
     else
        memset(availableTracks, 0, sizeof(availableTracks));
     pre_1_3_19_PrivateStream = 0;
     SetAudioChannel(0); // fall back to stereo
     currentAudioTrackMissingCount = 0;
     currentAudioTrack = ttNone;
     currentSubtitleTrack = ttNone;
     }
}

bool cDevice::SetAvailableTrack(eTrackType Type, int Index, uint16_t Id, const char *Language, const char *Description)
{
  eTrackType t = eTrackType(Type + Index);
  if (Type == ttAudio && IS_AUDIO_TRACK(t) ||
      Type == ttDolby && IS_DOLBY_TRACK(t) ||
      Type == ttSubtitle && IS_SUBTITLE_TRACK(t)) {
     if (Language)
        strn0cpy(availableTracks[t].language, Language, sizeof(availableTracks[t].language));
     if (Description)
       cUtf8Utils::Utf8Strn0Cpy(availableTracks[t].description, Description, sizeof(availableTracks[t].description));
     if (Id) {
        availableTracks[t].id = Id; // setting 'id' last to avoid the need for extensive locking
        if (Type == ttAudio || Type == ttDolby) {
           int numAudioTracks = NumAudioTracks();
           if (!availableTracks[currentAudioTrack].id && numAudioTracks && currentAudioTrackMissingCount++ > numAudioTracks * 10)
              EnsureAudioTrack();
           else if (t == currentAudioTrack)
              currentAudioTrackMissingCount = 0;
           }
        else if (Type == ttSubtitle && autoSelectPreferredSubtitleLanguage)
           EnsureSubtitleTrack();
        }
     return true;
     }
  else
     esyslog("ERROR: SetAvailableTrack called with invalid Type/Index (%d/%d)", Type, Index);
  return false;
}

const tTrackId *cDevice::GetTrack(eTrackType Type)
{
  return (ttNone < Type && Type < ttMaxTrackTypes) ? &availableTracks[Type] : NULL;
}

int cDevice::NumTracks(eTrackType FirstTrack, eTrackType LastTrack) const
{
  int n = 0;
  for (int i = FirstTrack; i <= LastTrack; i++) {
      if (availableTracks[i].id)
         n++;
      }
  return n;
}

int cDevice::NumAudioTracks(void) const
{
  return NumTracks(ttAudioFirst, ttDolbyLast);
}

int cDevice::NumSubtitleTracks(void) const
{
  return NumTracks(ttSubtitleFirst, ttSubtitleLast);
}

bool cDevice::SetCurrentAudioTrack(eTrackType Type)
{
  if (ttNone < Type && Type <= ttDolbyLast) {
     cMutexLock MutexLock(&mutexCurrentAudioTrack);
     if (IS_DOLBY_TRACK(Type))
        SetDigitalAudioDevice(true);
     currentAudioTrack = Type;
     if (player)
        player->SetAudioTrack(currentAudioTrack, GetTrack(currentAudioTrack));
     else
        SetAudioTrackDevice(currentAudioTrack);
     if (IS_AUDIO_TRACK(Type))
        SetDigitalAudioDevice(false);
     return true;
     }
  return false;
}

bool cDevice::SetCurrentSubtitleTrack(eTrackType Type, bool Manual)
{
  if (Type == ttNone || IS_SUBTITLE_TRACK(Type)) {
     currentSubtitleTrack = Type;
     autoSelectPreferredSubtitleLanguage = !Manual;
     if (dvbSubtitleConverter)
        dvbSubtitleConverter->Reset();
     if (Type == ttNone && dvbSubtitleConverter) {
        cMutexLock MutexLock(&mutexCurrentSubtitleTrack);
        DELETENULL(dvbSubtitleConverter);
        }
     DELETENULL(liveSubtitle);
     if (player)
        player->SetSubtitleTrack(currentSubtitleTrack, GetTrack(currentSubtitleTrack));
     else
        SetSubtitleTrackDevice(currentSubtitleTrack);
     if (currentSubtitleTrack != ttNone && !Replaying() && !Transferring()) {
        const tTrackId *TrackId = GetTrack(currentSubtitleTrack);
        if (TrackId && TrackId->id) {
           liveSubtitle = new cLiveSubtitle(TrackId->id);
           AttachReceiver(liveSubtitle);
           }
        }
     return true;
     }
  return false;
}

void cDevice::EnsureAudioTrack(bool Force)
{
  if (keepTracks)
     return;
  if (Force || !availableTracks[currentAudioTrack].id) {
     eTrackType PreferredTrack = ttAudioFirst;
     int PreferredAudioChannel = 0;
     int LanguagePreference = -1;
     int StartCheck = Setup.CurrentDolby ? ttDolbyFirst : ttAudioFirst;
     int EndCheck = ttDolbyLast;
     for (int i = StartCheck; i <= EndCheck; i++) {
         const tTrackId *TrackId = GetTrack(eTrackType(i));
         int pos = 0;
         if (TrackId && TrackId->id && I18nIsPreferredLanguage(Setup.AudioLanguages, TrackId->language, LanguagePreference, &pos)) {
            PreferredTrack = eTrackType(i);
            PreferredAudioChannel = pos;
            }
         if (Setup.CurrentDolby && i == ttDolbyLast) {
            i = ttAudioFirst - 1;
            EndCheck = ttAudioLast;
            }
         }
     // Make sure we're set to an available audio track:
     const tTrackId *Track = GetTrack(GetCurrentAudioTrack());
     if (Force || !Track || !Track->id || PreferredTrack != GetCurrentAudioTrack()) {
        if (!Force) // only log this for automatic changes
           dsyslog("setting audio track to %d (%d)", PreferredTrack, PreferredAudioChannel);
        SetCurrentAudioTrack(PreferredTrack);
        SetAudioChannel(PreferredAudioChannel);
        }
     }
}

void cDevice::EnsureSubtitleTrack(void)
{
  if (keepTracks)
     return;
  if (Setup.DisplaySubtitles) {
     eTrackType PreferredTrack = ttNone;
     int LanguagePreference = INT_MAX; // higher than the maximum possible value
     for (int i = ttSubtitleFirst; i <= ttSubtitleLast; i++) {
         const tTrackId *TrackId = GetTrack(eTrackType(i));
         if (TrackId && TrackId->id && (I18nIsPreferredLanguage(Setup.SubtitleLanguages, TrackId->language, LanguagePreference) ||
            (i == ttSubtitleFirst + 8 && !*TrackId->language && LanguagePreference == INT_MAX))) // compatibility mode for old subtitles plugin
            PreferredTrack = eTrackType(i);
         }
     // Make sure we're set to an available subtitle track:
     const tTrackId *Track = GetTrack(GetCurrentSubtitleTrack());
     if (!Track || !Track->id || PreferredTrack != GetCurrentSubtitleTrack())
        SetCurrentSubtitleTrack(PreferredTrack);
     }
  else
     SetCurrentSubtitleTrack(ttNone);
}

bool cDevice::Ready(void)
{
  return true;
}

#define TS_SCRAMBLING_TIMEOUT     3 // seconds to wait until a TS becomes unscrambled
#define TS_SCRAMBLING_TIME_OK    10 // seconds before a Channel/CAM combination is marked as known to decrypt

void cDevice::Action(void)
{
  if (Running() && OpenDvr()) {
     while (Running()) {
           // Read data from the DVR device:
           uchar *b = NULL;
           if (GetTSPacket(b)) {
              if (b) {
                 int Pid = TsPid(b);
                 // Check whether the TS packets are scrambled:
                 bool DetachReceivers = false;
                 bool DescramblingOk = false;
                 int CamSlotNumber = 0;
                 if (startScrambleDetection) {
                    cCamSlot *cs = CamSlot();
                    CamSlotNumber = cs ? cs->SlotNumber() : 0;
                    if (CamSlotNumber) {
                       bool Scrambled = b[3] & TS_SCRAMBLING_CONTROL;
                       int t = time(NULL) - startScrambleDetection;
                       if (Scrambled) {
                          if (t > TS_SCRAMBLING_TIMEOUT)
                             DetachReceivers = true;
                          }
                       else if (t > TS_SCRAMBLING_TIME_OK) {
                          DescramblingOk = true;
                          startScrambleDetection = 0;
                          }
                       }
                    }
                 // Distribute the packet to all attached receivers:
                 Lock();
                 for (int i = 0; i < MAXRECEIVERS; i++) {
                     if (receiver[i] && receiver[i]->WantsPid(Pid)) {
                        if (DetachReceivers) {
                           ChannelCamRelations.SetChecked(receiver[i]->ChannelID(), CamSlotNumber);
                           Detach(receiver[i]);
                           }
                        else
                           receiver[i]->Receive(b, TS_SIZE);
                        if (DescramblingOk)
                           ChannelCamRelations.SetDecrypt(receiver[i]->ChannelID(), CamSlotNumber);
                        }
                     }
                 Unlock();
                 }
              }
           else
              break;
           }
     CloseDvr();
     }
}

// --- cTSBuffer -------------------------------------------------------------

cTSBuffer::cTSBuffer(int File, int Size, int CardIndex)
{
  SetDescription("TS buffer on device %d", CardIndex);
  f = File;
  cardIndex = CardIndex;
  delivered = false;
  ringBuffer = new cRingBufferLinear(Size, TS_SIZE, true, "TS");
  ringBuffer->SetTimeouts(100, 100);
  ringBuffer->SetIoThrottle();
  Start();
}

cTSBuffer::~cTSBuffer()
{
  Cancel(3);
  delete ringBuffer;
}

void cTSBuffer::Action(void)
{
  if (ringBuffer) {
     bool firstRead = true;
     cPoller Poller(f);
     while (Running()) {
           if (firstRead || Poller.Poll(100)) {
              firstRead = false;
              int r = ringBuffer->Read(f);
              if (r < 0 && FATALERRNO) {
                 if (errno == EOVERFLOW)
                    esyslog("ERROR: driver buffer overflow on device %d", cardIndex);
                 else {
                    LOG_ERROR;
                    break;
                    }
                 }
              }
           }
     }
}

uchar *cTSBuffer::Get(void)
{
  int Count = 0;
  if (delivered) {
     ringBuffer->Del(TS_SIZE);
     delivered = false;
     }
  uchar *p = ringBuffer->Get(Count);
  if (p && Count >= TS_SIZE) {
     if (*p != TS_SYNC_BYTE) {
        for (int i = 1; i < Count; i++) {
            if (p[i] == TS_SYNC_BYTE) {
               Count = i;
               break;
               }
            }
        ringBuffer->Del(Count);
        esyslog("ERROR: skipped %d bytes to sync on TS packet on device %d", Count, cardIndex);
        return NULL;
        }
     delivered = true;
     return p;
     }
  return NULL;
}
