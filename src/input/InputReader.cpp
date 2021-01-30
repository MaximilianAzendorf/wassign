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
#include "InputReader.h"
#include "FuzzyMatch.h"
#include "../Util.h"
#include "../Status.h"

#include <utility>

InputReader::InputReader(shared_ptr<Options> options)
    : _options(std::move(options))
{
    ChaiscriptInterface::register_interface(*this);
}

const_ptr<InputData> InputReader::read_input(string const& input)
{
    _chai.eval(input);

    for(auto const& obj : _inputObjects)
    {
        if(!obj->registered)
            throw InputException(
                    "Newly created object not used. Did you try to access an existing one instead of creating a new one?");
    }

    if(_choices.empty())
    {
        throw InputException("No choices defined in input.");
    }
    if(_choosers.empty())
    {
        // TODO: We could actually accept this and just compute a scheduling without an assignment.
        throw InputException("No choosers defined in input.");
    }

    InputDataBuilder builder;
    builder.process_input_reader(*this);
    return builder.get_input_data();
}
