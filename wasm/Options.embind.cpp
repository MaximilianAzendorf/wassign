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

#include "../src/Options.h"

#include <emscripten/bind.h>
using namespace emscripten;

EMSCRIPTEN_BINDINGS(wassign_options)
{
    class_<Options>("Options")
            .smart_ptr_constructor("Options", &Options::default_options)
            .smart_ptr<const_ptr<Options>>("Options")
            //.property("verbosity", &Options::verbosity, &Options::set_verbosity)
            .property("timeout", &Options::timeout_seconds, &Options::set_timeout_seconds)
            .property("criticalSetTimeout",
                      &Options::critical_set_timeout_seconds, &Options::set_critical_set_timeout_seconds)
            .property("noCriticalSets", &Options::no_critical_sets, &Options::set_no_critical_sets)
            .property("preferenceExponent", &Options::preference_exponent, &Options::set_preference_exponent)
            .property("any", &Options::any, &Options::set_any)
            .property("threadCount", &Options::thread_count, &Options::set_thread_count)
            .property("greedy", &Options::greedy, &Options::set_greedy);
};