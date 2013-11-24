cmake_minimum_required(VERSION 2.6)
project(VDR)

# Setup testing
ADD_SUBDIRECTORY(lib/gtest)
enable_testing()

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/vdr)
include_directories(${gtest_SOURCE_DIR}/include)

set(VDR_SRCS
	vdr/SignalHandler.cpp
	vdr/VDRDaemon.cpp
	vdr/sources/ISourceParams.cpp
	vdr/sources/linux/DVBSourceParams.cpp
	vdr/utils/StringUtils.cpp
)

set(VDR_TEST_SRCS
	vdr/test/TestVDRDaemon.cpp
	vdr/sources/linux/test/TestDVBSourceParams.cpp
)

set(VDR_MAIN vdr/main.cpp)
set(VDR_ADDON vdr/addon.cpp)

set(LIBS
	pthread # TODO: Not needed on Android
)

set(VDR_EXECUTABLE vdr.bin)
add_executable(${VDR_EXECUTABLE} ${VDR_MAIN} ${VDR_SRCS})
target_link_libraries(${VDR_EXECUTABLE} ${LIBS})

set(VDR_LIBRARY vdr)
add_library(${VDR_LIBRARY} SHARED ${VDR_ADDON} ${VDR_SRCS})
target_link_libraries(${VDR_LIBRARY} ${LIBS})

set(VDR_TEST_EXECUTABLE test-vdr.bin)
add_executable(${VDR_TEST_EXECUTABLE} ${VDR_SRCS} ${VDR_TEST_SRCS})
target_link_libraries(${VDR_TEST_EXECUTABLE} gtest_main ${LIBS})

add_test(
	NAME ${VDR_TEST_EXECUTABLE}
	COMMAND ${VDR_TEST_EXECUTABLE}
)

