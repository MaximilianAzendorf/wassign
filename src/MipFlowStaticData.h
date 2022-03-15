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
#include "MipFlow.h"
#include "Constraint.h"
#include "InputData.h"

typedef uint64_t flowid;

/**
 * Contains some static data that is the same across all MipFlow instances and therefore do not have to be generated
 * every time.
 */
class MipFlowStaticData
{
private:
    const_ptr<InputData> _inputData;

public:
    MipFlow<flowid, flowid> baseFlow;
    vector<pair<int, int>> blockedEdges;
    vector<Constraint> constraints;

    static flowid make_long(int high, int low);
    static flowid node_chooser(int p, int s);
    static flowid node_slot(int s);
    static flowid node_choice(int w);
    static flowid edge_id(int from, int to);

    explicit MipFlowStaticData(const_ptr<InputData> inputData);
};


