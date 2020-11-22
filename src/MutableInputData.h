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
#include "ChoiceData.h"
#include "ChooserData.h"
#include "SetData.h"
#include "Constraint.h"
#include "input/ProtoChoiceData.h"

struct MutableInputDataConductorData
{
    int chooser;
    int choice;
};

struct MutableInputData
{
    vector<ChoiceData> choices;
    vector<ProtoChoiceData> preChoices;
    vector<ChooserData> choosers;
    vector<SetData> sets;
    vector<string> constraintStrings;
    vector<Constraint> constraints;
    vector<MutableInputDataConductorData> conductors;
};


