/*
 *      Copyright (C) 2013 Garrett Brown
 *      Copyright (C) 2013 Lars Op den Kamp
 *      Portions Copyright (C) 2000, 2003, 2006, 2008, 2013 Klaus Schmidinger
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING. If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "devices/DeviceSubsystem.h"
//#include "osd.h" // for eTextAlignment
#include "devices/Remux.h"
//#include "tools.h"

#include <vector>
#include <sys/types.h>

enum ePlayMode
{
  pmNone,           // audio/video from decoder
  pmAudioVideo,     // audio/video from player
  pmAudioOnly,      // audio only from player, video from decoder
  pmAudioOnlyBlack, // audio only from player, no video (black screen)
  pmVideoOnly       // video only from player, audio from decoder
};

class cRect;
class cPlayer;

class cDevicePlayerSubsystem : protected cDeviceSubsystem
{
public:
  cDevicePlayerSubsystem(cDevice *device);
  virtual ~cDevicePlayerSubsystem();

  /*!
   * \brief Gets the current System Time Counter, which can be used to
   *        synchronize audio, video and subtitles
   *
   * If this device is able to replay, it must provide an STC. The value
   * returned doesn't need to be an actual "clock" value, it is sufficient if it
   * holds the PTS (Presentation Time Stamp) of the most recently presented
   * frame. A proper value must be returned in normal replay mode as well as in
   * any trick modes (like slow motion, fast forward/rewind). Only the lower
   * 32 bit of this value are actually used, since some devices can't handle the
   * msb correctly.
   */
  virtual int64_t GetSTC() { return -1; }

  /*!
   * \brief Returns true if the currently attached player has delivered any video packets
   */
  virtual bool IsPlayingVideo() const { return m_bIsPlayingVideo; }

  /*!
   * \brief Asks the output device whether it can scale the currently shown
   *        video in such a way that it fits into the given cRect, while
   *        retaining its proper aspect ratio
   * \param rect A cRect whose coordinates are in the range of the width and
   *        height returned by GetOsdSize()
   * \param alignment Used to determine how to align the actual rectable with
   *        the requested one if the scaled video doesn't exactly fit into rect
   * \return The rectangle that can actually be used when scaling the video. If
   *         this device can't scale the video, a Null rectangle is returned.
   *
   * The actual rectangle can be smaller, larger or the same size as the given
   * cRect, and its location may differ, depending on the capabilities of the
   * output device, which may not be able to display a scaled video at arbitrary
   * sizes and locations. The device shall, however, do its best to match the
   * requested cRect as closely as possible, preferring a size and location that
   * fits completely into the requested cRect if possible.
   *
   * A skin plugin using this function should rearrange its content according to
   * the rectangle returned from calling this function, and should especially be
   * prepared for cases where the returned rectangle is way off the requested
   * cRect, or even Null. In such cases, the skin may want to fall back to
   * working with full screen video.
   */
//  virtual cRect CanScaleVideo(const cRect &rect, int alignment = taCenter) { return cRect::Null; }

  /*!
   * \brief Scales the currently shown video in such a way that it fits into the
   *        given Rect
   * \param rect Should be one retrieved through a previous call to
   *        CanScaleVideo() (otherwise results may be undefined)
   *
   * Even if video output is scaled, the functions GetVideoSize() and
   * GetOsdSize() must still return the same values as if in full screen mode!
   * If this device can't scale the video, nothing happens. To restore full
   * screen video, call this function with a Null rectangle.
   */
//  virtual void ScaleVideo(const cRect &cect = cRect::Null) { }

  /*!
   * \brief Returns true if this device can handle all frames in 'fast forward'
   *        trick speeds
   */
  virtual bool HasIBPTrickSpeed() { return false; }

  /*!
   * \brief Sets the device into a mode where replay is done slower. Every
   *        single frame shall then be displayed the given number of times.
   *
   * The cDvbPlayer uses the following values for the various speeds:
   *                 1x   2x   3x
   * Fast Forward     6    3    1
   * Fast Reverse     6    3    1
   * Slow Forward     8    4    2
   * Slow Reverse    63   48   24
   */
  virtual void TrickSpeed(int speed) { }

  /*!
   * \brief Clears all video and audio data from the device.
   *
   * A derived class must call the base class function to make sure all
   * registered cAudio objects are notified.
   */
  virtual void Clear();

  /*!
   * \brief Sets the device into play mode (after a previous trick mode)
   */
  virtual void Play();

  /*!
   * \brief Puts the device into "freeze frame" mode
   */
  virtual void Freeze();

  /*!
   * \brief Turns off audio while replaying
   *
   * A derived class must call the base class function to make sure all
   * registered cAudio objects are notified.
   */
  virtual void Mute();

  /*!
   * \brief Displays the given I-frame as a still picture
   * \param data Points either to TS (first byte is 0x47) or PES (first byte is
   *        0x00) data of the given Length
   *
   * The default implementation converts TS to PES and calls itself again,
   * allowing a derived class to display PES if it can't handle TS directly.
   */
  virtual void StillPicture(const std::vector<uchar> &data);

  /*!
   * \brief Returns true if the device itself or any of the file handles in
   *        Poller is ready for further action
   * \param timeoutMs If > 0, the device will wait up to the given number of
   *        milliseconds before returning in case it can't accept any data
   */
  virtual bool Poll(cPoller &poller, unsigned int timeoutMs = 0) { return false; }

  /*!
   * \brief Returns true if the device's output buffers are empty, i. e. any
   *        data which was buffered so far has been processed
   * \param timeoutMs If > 0, the device will wait up to the given number of
   *        milliseconds before returning in case there is still data in the buffers
   */
  virtual bool Flush(unsigned int timeoutMs = 0) { return true; }

  /*!
   * \brief Plays all valid PES packets in Data with the given Length
   * \param data If NULL, any leftover data from a previous call will be
   *        discarded. Should point to a sequence of complete PES packets.
   * \param bVideoOnly If true, only the video will be displayed, which is
   *        necessary for trick modes like 'fast forward'
   *
   * If the last packet in Data is not complete, it will be copied and combined
   * to a complete packet with data from the next call to PlayPes(). That way
   * any functions called from within PlayPes() will be guaranteed to always
   * receive complete PES packets.
   */
  virtual int PlayPes(const std::vector<uchar> &data, bool bVideoOnly = false);

  /*!
   * \brief Plays the given TS packet
   * \param data Points to a single TS packet. If empty, any leftover data from a
   *        previous call will be discarded. TODO
   * \param length Always TS_SIZE (the total size of a single TS packet)
   * \param bVideoOnly If true, only the video will be displayed, which is necessary
   *        for trick modes like 'fast forward'
   * \return the number of processed bytes, or -1 in case of error. PlayTs()
   *         shall process the TS packets either as a whole (returning TS_SIZE)
   *         or not at all, returning 0 or -1 and setting 'errno' accordingly).
   *
   * A derived device can reimplement this function to handle the TS packets
   * itself. Any packets the derived function can't handle must be sent to the
   * base class function. This applies especially to the PAT/PMT packets.
   */
  virtual int PlayTs(const std::vector<uchar> &data, bool bVideoOnly = false);

  /*!
   * \brief Returns true if we are currently replaying
   */
  bool Replaying() const { return m_player != NULL; }

  /*!
   * \brief Returns true if we are currently in Transfer Mode
   */
  bool Transferring() const;

  /*!
   * \brief Stops the current replay session (if any)
   */
  void StopReplay();

  /*!
   * \brief Attaches the given player to this device
   */
  bool AttachPlayer(cPlayer *player);

  /*!
   * \brief Detaches the given player from this device
   */
  void Detach(cPlayer *player);

protected:
  /*!
   * \brief Returns a pointer to the patPmtParser, so that a derived device can
   *        use the stream information from it
   */
  const cPatPmtParser *PatPmtParser() const { return &m_patPmtParser; }

  /*!
   * \brief Returns true if this device can currently start a replay session
   */
public: // TODO
  virtual bool CanReplay() const;

  /*!
   * \brief Sets the device into the given play mode
   * \return True if the operation was successful
   */
  virtual bool SetPlayMode(ePlayMode playMode) { return false; }

  /*!
   * \brief Plays the given data block as video
   * \param data Points to exactly one complete PES packet of the given length
   * \param length The length of a PES packet
   * \return The number of bytes actually taken from data, or -1 in case of an
   *         error. PlayVideo() shall process the packet either as a whole
   *         (returning length) or not at all (returning 0 or -1 and setting
   *         'errno' accordingly).
   */
  virtual int PlayVideo(const std::vector<uchar> &data) { return -1; }

  /*!
   * \brief Plays the given data block as audio
   * \param data Points to exactly one complete PES packet of the given length
   * \param length The length of data
   * \param id Indicates the type of audio data this packet holds
   * \return The number of bytes actually taken from data. or -1 in case of an
   *         error. PlayAudio() shall process the packet either as a whole
   *         (returning length) or not at all (returning 0 or -1 and setting
   *         'errno' accordingly).
   */
  virtual int PlayAudio(const std::vector<uchar> &data, uchar id) { return -1; }

  /*!
   * \brief Plays the given data block as a subtitle
   * \param data Points to exactly one complete PES packet of the given length
   * \param length The length of the packet
   * \return The number of bytes actually taken from data, or -1 in case of an
   *         error. PlaySubtitle() shall process the packet either as a whole
   *         (returning length) or not at all (returning 0 or -1 and setting
   *         'errno' accordingly).
   */
  virtual int PlaySubtitle(const std::vector<uchar> &data);

  /*!
   * \brief Plays the single PES packet in data with the given length
   * \param data Must point to one single, complete PES packet
   * \param length The length of the packet
   * \param bVideoOnly If true, only the video will be displayed, which is
   *        necessary for trick modes like 'fast forward'
   */
  virtual unsigned int PlayPesPacket(const std::vector<uchar> &data, bool bVideoOnly = false);

  /*!
   * \brief Plays the given data block as video
   * \param data Points to exactly one complete TS packet of the given length
   *        (which is always TS_SIZE)
   * \return The number of bytes actually taken from data, or -1 in case of an
   *         error. PlayTsVideo() shall process the packet either as a whole
   *         (returning length) or not at all (returning 0 or -1 and setting
   *         'errno' accordingly).
   *
   * The default implementation collects all incoming TS payload belonging to
   * one PES packet and calls PlayVideo() with the resulting packet.
   */
  virtual int PlayTsVideo(const std::vector<uchar> &data);

  /*!
   * \brief Plays the given data block as audio
   * \param data Points to exactly one complete TS packet of the given length
   *        (which is always TS_SIZE)
   * \param length The length of the packet (always equal to TS_SIZE)
   * \return The number of bytes actually taken from data, or -1 in case of an
   *         error. PlayTsAudio() shall process the packet either as a whole
   *         (returning length) or not at all (returning 0 or -1 and setting
   *         'errno' accordingly).
   *
   * The default implementation collects all incoming TS payload belonging to
   * one PES packet and calls PlayAudio() with the resulting packet.
   */
  virtual int PlayTsAudio(const std::vector<uchar> &data);

  /*!
   * \brief Plays the given data block as a subtitle
   * \param data Points to exactly one complete TS packet of the given length
   *        (which is always TS_SIZE)
   * \param length The length of the packet (always equal to TS_SIZE)
   * \return The number of bytes actually taken from data, or -1 in case of an
   *         error. PlayTsSubtitle() shall process the packet either as a whole
   *         (returning length) or not at all (returning 0 or -1 and setting
   *         'errno' accordingly).
   *
   * The default implementation collects all incoming TS payload belonging to
   * one PES packet and displays the resulting subtitle via the OSD.
   */
  virtual int PlayTsSubtitle(const std::vector<uchar> &data);

private:
public: // TODO
  cPlayer       *m_player;
private:
  cPatPmtParser  m_patPmtParser;
  cTsToPes       m_tsToPesVideo;
  cTsToPes       m_tsToPesAudio;
  cTsToPes       m_tsToPesSubtitle;
  bool           m_bIsPlayingVideo;
};
