#pragma once
#include "Types.h"
#include "MipFlow.h"
#include "Constraint.h"

struct MipFlowStaticData
{
    MipFlow<int, int> baseFlow;
    vector<pair<int, int>> blockedEdges;
    vector<Constraint> constraints;
};


