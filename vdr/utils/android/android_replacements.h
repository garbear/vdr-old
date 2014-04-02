#pragma once

#define POSIX_FADV_RANDOM   1
#define POSIX_FADV_DONTNEED 1
#define POSIX_FADV_WILLNEED 1
#define posix_fadvise(a,b,c,d) (-1)

// Android does not declare mkdtemp even though it's implemented.
extern "C" char *mkdtemp(char *path);

#include "sort.h"
#include "strchrnul.h"
