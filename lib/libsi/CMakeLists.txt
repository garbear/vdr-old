cmake_minimum_required(VERSION 2.6)
project(LIBSI)

set(LIBSI_SRCS
	util.cpp
	si.cpp
	section.cpp
	descriptor.cpp
	dish.cpp
	multiple_string.cpp
)
IF("${OS}" STREQUAL "android")
add_definitions(-DTARGET_ANDROID)
ENDIF()

set(LIBSI_LIBRARY si)
add_library(${LIBSI_LIBRARY} STATIC ${LIBSI_SRCS})
