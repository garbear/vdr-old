cmake_minimum_required(VERSION 2.6)
project(LIBPLATFORM)

set(LIBPLATFORM_SRCS
	threads/throttle.cpp
)

include_directories(${CMAKE_SOURCE_DIR})

set(LIBPLATFORM_LIBRARY libplatform)
add_library(${LIBPLATFORM_LIBRARY} STATIC ${LIBPLATFORM_SRCS})
