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

#include "../Types.h"
#include "../Options.h"
#include "../MutableInputData.h"
#include "../InputData.h"
#include "InputSetData.h"
#include "InputChoiceData.h"
#include "InputChooserData.h"
#include "ChaiscriptInterface.h"
#include "InputDataBuilder.h"
#include "ConstraintExpression.h"

#include <chaiscript/chaiscript.hpp>

namespace cs = chaiscript;

class ChaiscriptInterface;
class InputDataBuilder;

class InputReader
{
    friend class ChaiscriptInterface;
    friend class InputDataBuilder;

private:
    cs::ChaiScript _chai;

    shared_ptr<Options> _options;

    map<string, shared_ptr<InputSetData>> _setMap;
    map<string, shared_ptr<InputChoiceData>> _choiceMap;
    map<string, shared_ptr<InputChooserData>> _chooserMap;

    vector<shared_ptr<InputSetData>> _sets;
    vector<shared_ptr<InputChoiceData>> _choices;
    vector<shared_ptr<InputChooserData>> _choosers;
    vector<shared_ptr<InputObject>> _inputObjects;
    vector<string> _constraintStrings;
    vector<ConstraintExpression> _constraintExpressions;

public:
    explicit InputReader(shared_ptr<Options> options);

    /**
     * Parses a string containing the input and prints status information.
     *
     * @param input The input.
     * @return The resulting InputData instance.
     */
    const_ptr<InputData> read_input(string const& input);
};

