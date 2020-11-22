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
#include "Tagged.h"
#include "InputReader.h"

class InputReader;

class ChaiscriptInterface
{
private:
    ChaiscriptInterface() = default;

    template<typename T>
    static string find_by_name(string const& name, map<string, T>& values);

public:
    static void register_interface(InputReader& reader);

    static shared_ptr<InputSetData> set(InputReader& reader, string const& name);
    static shared_ptr<InputSetData> set(InputReader& reader, string const& name, vector<Tagged> const& t);

    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name);
    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name,
                                              Tagged const& t1);
    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name,
                                              Tagged const& t1, Tagged const& t2);
    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name,
                                              Tagged const& t1, Tagged const& t2, Tagged const& t3);
    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name,
                                              Tagged const& t1, Tagged const& t2, Tagged const& t3, Tagged const& t4);
    static shared_ptr<InputChoiceData> choice(InputReader& reader, string const& name,
                                              vector<Tagged> const& t);

    static shared_ptr<InputChooserData> chooser(InputReader& reader, string const& name);
    static shared_ptr<InputChooserData> chooser(InputReader& reader, string const& name,
                                                vector<int> const& preferences);
    static shared_ptr<InputChooserData> chooser(InputReader& reader, string const& name,
                                                vector<Tagged> const& t, vector<int> const& preferences);

    static shared_ptr<InputSetData> add(InputReader& reader, shared_ptr<InputSetData> set);
    static shared_ptr<InputChoiceData> add(InputReader& reader, shared_ptr<InputChoiceData> choice);
    static shared_ptr<InputChooserData> add(InputReader& reader, shared_ptr<InputChooserData> chooser);

    static Tagged min(int min);
    static Tagged max(int max);
    static Tagged bounds(int min, int max);
    static Tagged parts(int parts);
    static Tagged optional();
    static Tagged optional(bool optional);

    static void add_constraint(InputReader& reader, string const& constraintStr);
};

#include "ChaiscriptInterface.ipp"


