#pragma once
#include "../dlmalloc/malloc.h"
#define KOKORO_MALLOC(s)		dlmalloc(s)
#define KOKORO_FREE(p)			dlfree(p)
#define KOKORO_CALLOC(n, size)	dlcalloc(n, size)
#define KOKORO_REALLOC(p, size)	dlrealloc(p, size)
#define KOKORO_MALLOCA(size, a)	dlmemalign(a, size)
#define KOKORO_FREEN(p, n)		KOKORO_FREE(p)