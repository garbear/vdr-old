cmake_minimum_required(VERSION 2.6)
project(VDR)

# Setup testing
ADD_SUBDIRECTORY(lib/gtest)
enable_testing()

add_definitions(-DTARGET_LINUX)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/vdr)
include_directories(${gtest_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/lib)

set(VDR_SRCS
	vdr/SignalHandler.cpp
	vdr/VDRDaemon.cpp
	vdr/channels/Channel.cpp
	vdr/channels/ChannelID.cpp
	vdr/channels/Channels.cpp
	vdr/devices/Device.cpp
	vdr/devices/DeviceManager.cpp
	vdr/devices/DeviceSubsystem.cpp
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
	vdr/filesystem/Directory.cpp
	vdr/filesystem/DirectoryFetchJob.cpp
	vdr/filesystem/File.cpp
	vdr/filesystem/HDFile.cpp
	vdr/filesystem/IDirectory.cpp
	vdr/filesystem/ReadDir.cpp
	vdr/filesystem/VFSDirectory.cpp
	vdr/filesystem/VFSFile.cpp
	vdr/sources/ISourceParams.cpp
	vdr/sources/Source.cpp
	vdr/sources/linux/DVBSourceParams.cpp
	vdr/utils/StringUtils.cpp
)

set(VDR_SRCS2 # Sources for testing
	vdr/SignalHandler.cpp
	vdr/VDRDaemon.cpp
	#vdr/channels/Channel.cpp
	vdr/channels/ChannelID.cpp
	#vdr/channels/Channels.cpp
	vdr/sources/ISourceParams.cpp
	vdr/sources/Source.cpp
	vdr/sources/linux/DVBSourceParams.cpp
	vdr/utils/StringUtils.cpp
)

set(VDR_TEST_SRCS
	vdr/test/TestVDRDaemon.cpp
	#vdr/channels/test/TestChannel.cpp
	vdr/channels/test/TestChannelID.cpp
	#vdr/channels/test/TestChannels.cpp
	vdr/sources/Source.cpp
	vdr/sources/linux/test/TestDVBSourceParams.cpp
)

set(VDR_MAIN vdr/main.cpp)
set(VDR_ADDON vdr/addon.cpp)

set(LIBS
	pthread # TODO: Not needed on Android
)

#set(VDR_EXECUTABLE vdr.bin)
#add_executable(${VDR_EXECUTABLE} ${VDR_MAIN} ${VDR_SRCS})
#target_link_libraries(${VDR_EXECUTABLE} ${LIBS})

#set(VDR_LIBRARY vdr)
#add_library(${VDR_LIBRARY} SHARED ${VDR_ADDON} ${VDR_SRCS})
#target_link_libraries(${VDR_LIBRARY} ${LIBS})

set(VDR_TEST_EXECUTABLE test-vdr.bin)
add_executable(${VDR_TEST_EXECUTABLE} ${VDR_SRCS2} ${VDR_TEST_SRCS})
target_link_libraries(${VDR_TEST_EXECUTABLE} gtest_main ${LIBS})

add_test(
	NAME ${VDR_TEST_EXECUTABLE}
	COMMAND ${VDR_TEST_EXECUTABLE}
)

