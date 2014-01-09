cmake_minimum_required(VERSION 2.6)
project(LIBHTS)

set(LIBHTS_SRCS
	htsbuf.c
	htsmsg_binary.c
	htsmsg.c
	htsstr.c
)

set(LIBHTS_LIBRARY libhts)
add_library(${LIBHTS_LIBRARY} SHARED ${LIBHTS_SRCS})
