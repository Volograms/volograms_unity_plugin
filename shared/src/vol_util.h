#pragma once 

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdint.h> // include uint types
#include <stddef.h>/* size_t */
#include <string.h> // include memcpy()
#ifdef _WIN32
#include <malloc.h> // include alloca()
#include <windows.h> /* for backtraces and timers */
#else
#include <unistd.h> // Added only for debugging, should be removed for builds
#include <alloca.h> // include alloca()
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#include "vol_av.h"
#include "vol_geom.h"
#include "vol_util.h"

void apg_time_init( void );
double apg_time_s( void );