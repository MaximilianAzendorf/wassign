/*
 * Copyright 2020 Maximilian Azendorf
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Types.h"
#include "Util.h"

#define WSOLVE_VERSION_MAJOR "2"
#define WSOLVE_VERSION_MINOR "0"
#define WSOLVE_VERSION_PATCH "0"

#define WSOLVE_VERSION_HAS_META 0
#define WSOLVE_VERSION_META ""

#if WSOLVE_VERSION_HAS_META
#define WSOLVE_VERSION WSOLVE_VERSION_MAJOR "." WSOLVE_VERSION_MINOR "." WSOLVE_VERSION_PATCH "-" WSOLVE_VERSION_META
#else
#define WSOLVE_VERSION WSOLVE_VERSION_MAJOR "." WSOLVE_VERSION_MINOR "." WSOLVE_VERSION_PATCH
#endif