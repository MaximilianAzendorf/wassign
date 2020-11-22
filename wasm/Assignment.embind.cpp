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

#include "../src/Assignment.h"

#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(wassign_assignment)
{
    class_<Assignment>("Assignment")
            .function("choiceOf", &Assignment::choice_of)
            .function("choosersOrdered", &Assignment::choosers_ordered)
            .function("choicesOrdered", &Assignment::choices_ordered)
            .function("isInChoice", &Assignment::is_in_choice)
            .function("maxUsedPreference", &Assignment::max_used_preference)
            .function("inputData", &Assignment::input_data);
};