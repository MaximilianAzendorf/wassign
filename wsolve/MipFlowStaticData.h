#pragma once
#include "Types.h"
#include "MipFlow.h"
#include "Constraint.h"

#include <stdint.h>

typedef uint64_t flowid;

struct MipFlowStaticData
{
    MipFlow<flowid, flowid> baseFlow;
    vector<pair<int, int>> blockedEdges;
    vector<Constraint> constraints;
};


