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

#include "MipFlowStaticData.h"
#include "InputData.h"

#define SLOT_ID_HIGH (INT_MAX / 2)
#define WORKSHOP_ID_HIGH (INT_MAX / 2 - 1)

/*
 * Some helper functions to construct node ids.
 */
flowid MipFlowStaticData::make_long(int high, int low) { return ((flowid)high << 32U) | ((flowid)low & 0xFFFFFFFFUL); }
flowid MipFlowStaticData::node_participant(int p, int s) { return make_long(p, s); }
flowid MipFlowStaticData::node_slot(int s) { return make_long(SLOT_ID_HIGH, s); }
flowid MipFlowStaticData::node_workshop(int w) { return make_long(WORKSHOP_ID_HIGH, w); }
flowid MipFlowStaticData::edge_id(int from, int to) { return make_long(from, to); }

MipFlowStaticData::MipFlowStaticData(const_ptr<InputData> inputData)
{
    for(int p = 0; p < inputData->participant_count(); p++)
    {
        for(int s = 0; s < inputData->slot_count(); s++)
        {
            baseFlow.add_node(node_participant(p, s));
        }
    }

    for(int w = 0; w < inputData->workshop_count(); w++)
    {
        baseFlow.add_node(node_workshop(w));
    }

    for(int s = 0; s < inputData->slot_count(); s++)
    {
        baseFlow.add_node(node_slot(s));
    }

    constraints = inputData->assignment_constraints();
}