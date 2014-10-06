cmake_minimum_required(VERSION 2.6)
project(VDR)

include(CheckLibraryExists)

# Setup testing
ADD_SUBDIRECTORY(lib/gtest)
enable_testing()

# disable regex support for now
SET(USE_REGEX 0)

ADD_SUBDIRECTORY(lib/libsi)
ADD_SUBDIRECTORY(lib/libhts)
ADD_SUBDIRECTORY(lib/platform)

IF("${OS}" STREQUAL "android")
SET(IS_ANDROID 1)
ELSE()
SET(IS_ANDROID 0)
ENDIF()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindLibdl.cmake")

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/vdr)
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${gtest_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/libsi)
include_directories(${CMAKE_SOURCE_DIR}/lib/libhts)

CHECK_LIBRARY_EXISTS(rt clock_gettime "time.h" HAVE_RT)
CHECK_LIBRARY_EXISTS(pthread pthread_create "pthread.h" HAVE_PTHREAD)

CHECK_LIBRARY_EXISTS(fribidi fribidi_log2vis "fribidi/fribidi.h" HAVE_FRIBIDI) # TODO: Fails
CHECK_LIBRARY_EXISTS(iconv iconv_open "iconv.h" HAVE_ICONV)

if(USE_REGEX)
  CHECK_LIBRARY_EXISTS(pcre pcre_compile "pcre.h" HAVE_PCRE)

  if(NOT HAVE_PCRE)
    ADD_SUBDIRECTORY(lib/pcre)
    set(HAVE_PCRE 1)
  endif()

  add_definitions(-DHAVE_PCRE)
else()
  SET(HAVE_PCRE 0)
endif()

if(NOT IS_ANDROID)
  CHECK_LIBRARY_EXISTS(cap cap_set_proc "sys/capability.h" HAVE_CAP)
endif()

if(HAVE_CAP)
  list(APPEND LIBS cap)
  add_definitions(-DHAVE_CAP)
endif()
if(HAVE_RT)
  list(APPEND LIBS rt)
endif()
if(HAVE_PTHREAD)
  list(APPEND LIBS pthread)
endif()
if(HAVE_PCRE)
  list(APPEND LIBS pcre)
endif()
if(HAVE_FRIBIDI)
  list(APPEND LIBS fribidi)
else()                      # TODO
  list(APPEND LIBS fribidi) # TODO
endif()

list(APPEND LIBS tinyxml)

list(APPEND LIBS si)

list(APPEND LIBS hts)

list(APPEND LIBS platform)

# Enable gtest
enable_testing()

if(NOT IS_ANDROID)
    set(ANDROID_HELPER_SRCS "")
    add_definitions(-DTARGET_LINUX -DTARGET_POSIX)
else()
    #if(HAVE_ICONV)
        list(APPEND LIBS iconv)
    #endif()
    # TODO: CHECK_LIBRARY_EXISTS
    list(APPEND LIBS intl)

    add_definitions(-DTARGET_LINUX -DTARGET_POSIX -DTARGET_ANDROID)
    set(ANDROID_HELPER_SRCS
	    vdr/utils/android/sort.c
	    vdr/utils/android/strchrnul.c
    )
endif()

# Enable C++11
set(USE_CPP11 1)

if(USE_CPP11)
    ADD_DEFINITIONS(-std=gnu++11
                    -DHAVE_STD_SHARED_PTR
                    -DHAVE_STD__U16STRING
                    -DHAVE_STD__U32STRING)
endif()

set(VDR_FS_SRC_NATIVE
	vdr/filesystem/native/HDFile.cpp
	vdr/filesystem/native/HDDirectory.cpp
)

set(VDR_FS_SRC_XBMC
	vdr/filesystem/xbmc/VFSDirectory.cpp
	vdr/filesystem/xbmc/VFSFile.cpp
)

set(VDR_SRCS
	vdr/SignalHandler.cpp
	vdr/VDRDaemon.cpp
	vdr/channels/Channel.cpp
	vdr/channels/ChannelFilter.cpp
	vdr/channels/ChannelGroup.cpp
	vdr/channels/ChannelID.cpp
	vdr/channels/ChannelManager.cpp
	vdr/devices/commoninterface/CI.cpp
	vdr/devices/Device.cpp
	vdr/devices/DeviceManager.cpp
	vdr/devices/DeviceSubsystem.cpp
	vdr/devices/Remux.cpp
	vdr/devices/Transfer.cpp
	vdr/devices/TunerHandle.cpp
	vdr/devices/linux/DVBDevice.cpp
	vdr/devices/linux/DVBTuner.cpp
	vdr/devices/linux/commoninterface/DVBCIAdapter.cpp
	vdr/devices/linux/subsystems/DVBChannelSubsystem.cpp
	vdr/devices/linux/subsystems/DVBCommonInterfaceSubsystem.cpp
	vdr/devices/linux/subsystems/DVBReceiverSubsystem.cpp
	vdr/devices/subsystems/DeviceChannelSubsystem.cpp
	vdr/devices/subsystems/DeviceCommonInterfaceSubsystem.cpp
	vdr/devices/subsystems/DeviceImageGrabSubsystem.cpp
	vdr/devices/subsystems/DevicePlayerSubsystem.cpp
	vdr/devices/subsystems/DeviceReceiverSubsystem.cpp
	vdr/devices/subsystems/DeviceScanSubsystem.cpp
	vdr/devices/subsystems/DeviceSPUSubsystem.cpp
	vdr/devices/subsystems/DeviceTrackSubsystem.cpp
	vdr/devices/subsystems/DeviceVideoFormatSubsystem.cpp
	vdr/dvb/CADescriptor.cpp
	vdr/dvb/CADescriptorHandler.cpp
	vdr/dvb/DiSEqC.cpp
	vdr/dvb/filters/EIT.cpp
	vdr/dvb/filters/Filter.cpp
	vdr/dvb/filters/NIT.cpp
	vdr/dvb/filters/PAT.cpp
	vdr/dvb/filters/PMT.cpp
	vdr/dvb/filters/PSIP_EIT.cpp
	vdr/dvb/filters/PSIP_MGT.cpp
	vdr/dvb/filters/PSIP_STT.cpp
	vdr/dvb/filters/PSIP_VCT.cpp
	vdr/dvb/filters/ScanReceiver.cpp
	vdr/dvb/filters/SDT.cpp
	vdr/dvb/filters/TDT.cpp
	vdr/epg/Component.cpp
	vdr/epg/EPGScanner.cpp
	vdr/epg/EPGStringifier.cpp
	vdr/epg/Event.cpp
	vdr/epg/Schedule.cpp
	vdr/epg/ScheduleManager.cpp
	vdr/filesystem/Directory.cpp
	vdr/filesystem/DirectoryFetchJob.cpp
	vdr/filesystem/File.cpp
	vdr/filesystem/IDirectory.cpp
	vdr/filesystem/LockFile.cpp
	vdr/filesystem/Poller.cpp
	vdr/filesystem/ReadDir.cpp
	vdr/filesystem/SpecialProtocol.cpp
	vdr/filesystem/Videodir.cpp
	vdr/filesystem/VideoFile.cpp
	vdr/recordings/filesystem/FileName.cpp
	vdr/recordings/filesystem/IndexFile.cpp
	vdr/recordings/filesystem/ResumeFile.cpp
	vdr/recordings/marks/Mark.cpp
	vdr/recordings/marks/Marks.cpp
	vdr/recordings/Recorder.cpp
	vdr/recordings/Recording.cpp
	vdr/recordings/RecordingCutter.cpp
	vdr/recordings/RecordingInfo.cpp
	vdr/recordings/RecordingUserCommand.cpp
	vdr/recordings/Recordings.cpp
	vdr/scan/CNICodes.cpp
	vdr/scan/ScanConfig.cpp
	vdr/scan/Scanner.cpp
	vdr/scan/SWReceiver.cpp
	vdr/settings/AllowedHosts.cpp
	vdr/settings/Settings.cpp
	vdr/timers/Timer.cpp
	vdr/timers/Timers.cpp
	vdr/timers/TimerTime.cpp
	vdr/transponders/CountryUtils.cpp
	vdr/transponders/dvb_wrapper.cpp
	vdr/transponders/SatelliteUtils.cpp
	vdr/transponders/Stringifier.cpp
	vdr/transponders/Transponder.cpp
	vdr/transponders/TransponderFactory.cpp
	vdr/transponders/TransponderParams.cpp
	vdr/utils/CalendarUtils.cpp
	vdr/utils/CharsetConverter.cpp
	vdr/utils/CharSetConverterVDR.cpp
	vdr/utils/CharsetDetection.cpp
	vdr/utils/DateTime.cpp
	vdr/utils/Hash.cpp
	vdr/utils/I18N.cpp
	vdr/utils/Observer.cpp
	vdr/utils/RegExp.cpp
	vdr/utils/Ringbuffer.cpp
	vdr/utils/StringUtils.cpp
	vdr/utils/TimeUtils.cpp
	vdr/utils/Tools.cpp
	vdr/utils/XBMCTinyXML.cpp
	vdr/utils/XMLUtils.cpp
	vdr/utils/UTF8Utils.cpp
	vdr/utils/SynchronousAbort.cpp
	vdr/utils/CRC32.cpp
	vdr/utils/List.cpp
	vdr/utils/log/Log.cpp
	vdr/utils/log/LogConsole.cpp
	vdr/utils/log/LogSyslog.cpp
	vdr/utils/url/URL.cpp
	vdr/utils/url/UrlOptions.cpp
	vdr/utils/url/URLUtils.cpp
	vdr/utils/Shutdown.cpp
	vdr/utils/Timer.cpp
	vdr/utils/XSocket.cpp
	vdr/utils/XTimeUtils.cpp
	vdr/utils/linux/ConvUtils.cpp
	vdr/utils/Version.cpp
	vdr/Player.cpp
	vdr/vnsi/net/RequestPacket.cpp
	vdr/vnsi/net/ResponsePacket.cpp
	vdr/vnsi/video/Demuxer.cpp
	vdr/vnsi/video/RecPlayer.cpp
	vdr/vnsi/video/Streamer.cpp
	vdr/vnsi/video/VideoBuffer.cpp
	vdr/vnsi/video/parser/Bitstream.cpp
	vdr/vnsi/video/parser/Parser.cpp
	vdr/vnsi/video/parser/ParserAAC.cpp
	vdr/vnsi/video/parser/ParserAC3.cpp
	vdr/vnsi/video/parser/ParserDTS.cpp
	vdr/vnsi/video/parser/ParserH264.cpp
	vdr/vnsi/video/parser/ParserMPEGAudio.cpp
	vdr/vnsi/video/parser/ParserMPEGVideo.cpp
	vdr/vnsi/video/parser/ParserSubtitle.cpp
	vdr/vnsi/video/parser/ParserTeletext.cpp
	vdr/vnsi/Client.cpp
	vdr/vnsi/Server.cpp
)

set(VDR_TEST_SRCS
	vdr/test/TestVDRDaemon.cpp
	vdr/channels/test/TestChannel.cpp
	vdr/channels/test/TestChannelID.cpp
	vdr/channels/test/TestChannelManager.cpp
	vdr/filesystem/test/TestSpecialProtocol.cpp
	vdr/filesystem/native/test/TestHDDirectory.cpp
	vdr/filesystem/native/test/TestHDFile.cpp
	vdr/scan/test/TestCNICodes.cpp
	vdr/scan/test/TestScanner.cpp
	vdr/test/gtest/TestBasicEnvironment.cpp
	vdr/test/gtest/TestUtils.cpp
	vdr/test/gtest/vdr-test.cpp
	vdr/utils/test/TestRegExp.cpp
	vdr/utils/test/TestStringUtils.cpp
	vdr/utils/test/TestSynchronousAbort.cpp
	vdr/utils/test/TestXBMCTinyXML.cpp
	vdr/utils/test/TestXMLUtils.cpp
	vdr/utils/url/test/TestURL.cpp
	vdr/utils/url/test/TestUrlOptions.cpp
	vdr/utils/url/test/TestURLUtils.cpp
)

set(VDR_MAIN vdr/main.cpp)
set(VDR_ADDON vdr/addon.cpp)

set(VDR_EXECUTABLE vdr.bin)
add_executable(${VDR_EXECUTABLE} ${VDR_MAIN} ${VDR_SRCS} ${VDR_FS_SRC_NATIVE} ${ANDROID_HELPER_SRCS})
target_link_libraries(${VDR_EXECUTABLE} ${LIBS})

set(VDR_LIBRARY XBMC_VDR)
add_library(${VDR_LIBRARY} SHARED ${VDR_ADDON} ${VDR_SRCS} ${VDR_FS_SRC_XBMC} ${VDR_FS_SRC_NATIVE} ${ANDROID_HELPER_SRCS})
set_target_properties(${VDR_LIBRARY} PROPERTIES DEFINE_SYMBOL TARGET_XBMC)
target_link_libraries(${VDR_LIBRARY} ${LIBS})

set(VDR_TEST_EXECUTABLE test-vdr.bin)
add_executable(${VDR_TEST_EXECUTABLE} ${VDR_SRCS} ${VDR_FS_SRC_NATIVE} ${VDR_TEST_SRCS})
target_link_libraries(${VDR_TEST_EXECUTABLE} gtest ${LIBS})

add_test(
	NAME ${VDR_TEST_EXECUTABLE}
	COMMAND ${VDR_TEST_EXECUTABLE}
)

