cmake_minimum_required(VERSION 2.6)
project(VDR)

include(CheckLibraryExists)

# Setup testing
ADD_SUBDIRECTORY(lib/gtest)
enable_testing()

ADD_SUBDIRECTORY(lib/pcre)
ADD_SUBDIRECTORY(lib/libsi)

SET(IS_ANDROID $ENV{ANDROID})

add_definitions(-DTARGET_LINUX)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/vdr)
include_directories(${CMAKE_SOURCE_DIR}/lib)
include_directories(${gtest_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib/pcre)
include_directories(${CMAKE_SOURCE_DIR}/lib/libsi)

if ("${IS_ANDROID}" STREQUAL "")
set(ANDROID_HELPER_SRCS "")
else ("${IS_ANDROID}" STREQUAL "")
set(ANDROID_HELPER_SRCS
	vdr/utils/android/getline.c
	vdr/utils/android/sort.c
	vdr/utils/android/strchrnul.c
	vdr/utils/android/timegm.c
)
endif("${IS_ANDROID}" STREQUAL "")

set(VDR_FS_SRC_NATIVE
	vdr/filesystem/native/HDFile.cpp
)

set(VDR_FS_SRC_XBMC
	vdr/filesystem/xbmc/VFSDirectory.cpp
	vdr/filesystem/xbmc/VFSFile.cpp
)

set(VDR_SRCS
	vdr/Config.cpp
	vdr/SignalHandler.cpp
	vdr/VDRDaemon.cpp
	vdr/channels/Channel.cpp
	vdr/channels/ChannelID.cpp
	vdr/channels/ChannelManager.cpp
	vdr/devices/CI.cpp
	vdr/devices/Device.cpp
	vdr/devices/DeviceManager.cpp
	vdr/devices/DeviceSubsystem.cpp
	vdr/devices/Receiver.cpp
	vdr/devices/Remux.cpp
	vdr/devices/Transfer.cpp
	vdr/devices/linux/DVBDevice.cpp
	vdr/devices/linux/DVBDeviceProbe.cpp
	vdr/devices/linux/DVBTuner.cpp
	vdr/devices/linux/subsystems/DVBAudioSubsystem.cpp
	vdr/devices/linux/subsystems/DVBChannelSubsystem.cpp
	vdr/devices/linux/subsystems/DVBCommonInterfaceSubsystem.cpp
	vdr/devices/linux/subsystems/DVBPIDSubsystem.cpp
	vdr/devices/linux/subsystems/DVBReceiverSubsystem.cpp
	vdr/devices/linux/subsystems/DVBSectionFilterSubsystem.cpp
	vdr/devices/subsystems/DeviceAudioSubsystem.cpp
	vdr/devices/subsystems/DeviceChannelSubsystem.cpp
	vdr/devices/subsystems/DeviceCommonInterfaceSubsystem.cpp
	vdr/devices/subsystems/DeviceImageGrabSubsystem.cpp
	vdr/devices/subsystems/DevicePIDSubsystem.cpp
	vdr/devices/subsystems/DevicePlayerSubsystem.cpp
	vdr/devices/subsystems/DeviceReceiverSubsystem.cpp
	vdr/devices/subsystems/DeviceSectionFilterSubsystem.cpp
	vdr/devices/subsystems/DeviceSPUSubsystem.cpp
	vdr/devices/subsystems/DeviceTrackSubsystem.cpp
	vdr/devices/subsystems/DeviceVideoFormatSubsystem.cpp
	vdr/dvb/DiSEqC.cpp
	vdr/dvb/DVBCI.cpp
	vdr/dvb/Filter.cpp
	vdr/dvb/EIT.cpp
	vdr/dvb/EITScan.cpp
	vdr/dvb/NIT.cpp
	vdr/dvb/PAT.cpp
	vdr/dvb/SDT.cpp
	vdr/dvb/Sections.cpp
	vdr/epg/EPG.cpp
	vdr/filesystem/Directory.cpp
	vdr/filesystem/DirectoryFetchJob.cpp
	vdr/filesystem/File.cpp
	vdr/filesystem/IDirectory.cpp
	vdr/filesystem/ReadDir.cpp
	vdr/filesystem/Videodir.cpp
	vdr/recordings/Recorder.cpp
	vdr/recordings/Recording.cpp
	vdr/recordings/RecordingCutter.cpp
	vdr/recordings/Timers.cpp
	vdr/settings/Settings.cpp
	vdr/sources/ISourceParams.cpp
	vdr/sources/Source.cpp
	vdr/sources/linux/DVBSourceParams.cpp
	vdr/utils/CalendarUtils.cpp
	vdr/utils/CharsetConverter.cpp
	vdr/utils/CharSetConverterVDR.cpp
	vdr/utils/CharsetDetection.cpp
	vdr/utils/I18N.cpp
	vdr/utils/Observer.cpp
	vdr/utils/RegExp.cpp
	vdr/utils/Ringbuffer.cpp
	vdr/utils/StringUtils.cpp
	vdr/utils/Tools.cpp
	vdr/utils/XBMCTinyXML.cpp
	vdr/utils/XMLUtils.cpp
	vdr/utils/UTF8Utils.cpp
	vdr/utils/Status.cpp
	thread.cpp
)

set(VDR_SRCS2 # Sources for testing
	vdr/SignalHandler.cpp
	vdr/VDRDaemon.cpp
	vdr/channels/Channel.cpp
	vdr/channels/ChannelID.cpp
	vdr/channels/ChannelManager.cpp
	vdr/sources/ISourceParams.cpp
	vdr/sources/Source.cpp
	vdr/sources/linux/DVBSourceParams.cpp
	vdr/utils/StringUtils.cpp
)

set(VDR_TEST_SRCS
	vdr/test/TestVDRDaemon.cpp
	vdr/channels/test/TestChannel.cpp
	vdr/channels/test/TestChannelID.cpp
	vdr/channels/test/TestChannelManager.cpp
	vdr/filesystem/DirectoryFetchJob.cpp
	vdr/filesystem/File.cpp
	vdr/filesystem/IDirectory.cpp
	vdr/filesystem/ReadDir.cpp
	#vdr/filesystem/xbmc/VFSDirectory.cpp
	#vdr/filesystem/xbmc/VFSFile.cpp
	vdr/filesystem/Videodir.cpp
	vdr/sources/Source.cpp
	vdr/sources/linux/test/TestDVBSourceParams.cpp
	vdr/utils/CalendarUtils.cpp
	vdr/utils/CharsetConverter.cpp
	vdr/utils/CharSetConverterVDR.cpp
	vdr/utils/CharsetDetection.cpp
	vdr/utils/RegExp.cpp
	vdr/utils/StringUtils.cpp
	vdr/utils/Tools.cpp
	vdr/utils/XBMCTinyXML.cpp
	vdr/utils/XMLUtils.cpp
	vdr/utils/UTF8Utils.cpp
	vdr/utils/Status.cpp
	thread.cpp
)

set(VDR_MAIN vdr/main.cpp)
set(VDR_ADDON vdr/addon.cpp)

CHECK_LIBRARY_EXISTS(rt clock_gettime "time.h" HAVE_RT)
CHECK_LIBRARY_EXISTS(pthread pthread_create "pthread.h" HAVE_PTHREAD)
CHECK_LIBRARY_EXISTS(pcre pcre_compile "pcre.h" HAVE_PCRE)
CHECK_LIBRARY_EXISTS(fribidi fribidi_log2vis "fribidi/fribidi.h" HAVE_FRIBIDI)

set(LIBS
	pcre
	libsi
	fribidi
	tinyxml
)

if(HAVE_RT)
  set(LIBRT "rt")
endif()

if(HAVE_PTHREAD)
  set(LIBPTHREAD "pthread")
endif()

set(VDR_EXECUTABLE vdr.bin)
add_executable(${VDR_EXECUTABLE} ${VDR_MAIN} ${VDR_SRCS} ${VDR_FS_SRC_NATIVE} ${ANDROID_HELPER_SRCS})
target_link_libraries(${VDR_EXECUTABLE} ${LIBS} ${LIBRT} ${LIBPTHREAD})

set(VDR_LIBRARY vdr)
add_library(${VDR_LIBRARY} SHARED ${VDR_ADDON} ${VDR_SRCS} ${VDR_FS_SRC_XBMC} ${ANDROID_HELPER_SRCS})
set_target_properties(${VDR_LIBRARY} PROPERTIES DEFINE_SYMBOL TARGET_XBMC)
target_link_libraries(${VDR_LIBRARY} ${LIBS} ${LIBRT} ${LIBPTHREAD})

set(VDR_TEST_EXECUTABLE test-vdr.bin)
add_executable(${VDR_TEST_EXECUTABLE} ${VDR_SRCS2} ${VDR_TEST_SRCS} ${VDR_FS_SRC_NATIVE})
target_link_libraries(${VDR_TEST_EXECUTABLE} gtest_main ${LIBS} ${LIBRT} ${LIBPTHREAD})

add_test(
	NAME ${VDR_TEST_EXECUTABLE}
	COMMAND ${VDR_TEST_EXECUTABLE}
)

