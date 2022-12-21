// 2019 Team AobaZero
// This source code is in the public domain.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#if defined(_WIN32)
#  include <io.h>
#  include <conio.h>
#else
#  include <sys/time.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif
#include "shogi.h"

#if defined(_MSC_VER)
#  include <Share.h>
#  define fopen( file