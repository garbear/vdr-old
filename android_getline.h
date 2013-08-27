#pragma once

#ifdef ANDROID
ssize_t getline(char **lineptr, size_t *n, FILE *stream);
#endif

