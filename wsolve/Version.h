#pragma once

#include "Types.h"
#include "Util.h"

#define WSOLVE_VERSION_MAJOR "2"
#define WSOLVE_VERSION_MINOR "0"
#define WSOLVE_VERSION_PATCH "0"

#define WSOLVE_VERSION_HAS_META 1
#define WSOLVE_VERSION_META "alpha"

#if WSOLVE_VERSION_HAS_META
#define WSOLVE_VERSION WSOLVE_VERSION_MAJOR "." WSOLVE_VERSION_MINOR "." WSOLVE_VERSION_PATCH "-" WSOLVE_VERSION_META
#else
#define WSOLVE_VERSION WSOLVE_VERSION_MAJOR "." WSOLVE_VERSION_MINOR "." WSOLVE_VERSION_PATCH
#endif