cmake_minimum_required(VERSION 2.6)
project(LIBPLATFORM)

set(LIBPLATFORM_SRCS
	threads/throttle.cpp
)

include_directories(${CMAKE_SOURCE_DIR})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

set(LIBPLATFORM_LIBRARY platform)
add_library(${LIBPLATFORM_LIBRARY} STATIC ${LIBPLATFORM_SRCS})
