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

#define WASSIGN_VERSION_MAJOR "2"
#define WASSIGN_VERSION_MINOR "0"
#define WASSIGN_VERSION_PATCH "0"

#define WASSIGN_VERSION_HAS_META 0
#define WASSIGN_VERSION_META ""

#if WASSIGN_VERSION_HAS_META
#define WASSIGN_VERSION WASSIGN_VERSION_MAJOR "." WASSIGN_VERSION_MINOR "." WASSIGN_VERSION_PATCH "-" WASSIGN_VERSION_META
#else
#define WASSIGN_VERSION WASSIGN_VERSION_MAJOR "." WASSIGN_VERSION_MINOR "." WASSIGN_VERSION_PATCH
#endif
