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

#include "../src/InputData.h"

#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(input_data)
{
    class_<InputData>("InputData")
        .function("choice", &InputData::choice)
        .function("chooser", &InputData::chooser)
        .function("set", &InputData::set)
        .property("maxPreference", &InputData::max_preference)
        .property("choiceCount", &InputData::choice_count)
        .property("chooserCount", &InputData::chooser_count)
        .property("setCount", &InputData::set_count)
}