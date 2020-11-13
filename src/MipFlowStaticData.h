#pragma once
#include "Types.h"
#include "MipFlow.h"
#include "Constraint.h"
#include "InputData.h"

typedef uint64_t flowid;

class MipFlowStaticData
{
private:
    const_ptr<InputData> _inputData;

public:
    MipFlow<flowid, flowid> baseFlow;
    vector<pair<int, int>> blockedEdges;
    vector<Constraint> constraints;

    static flowid make_long(int high, int low);
    static flowid node_participant(int p, int s);
    static flowid node_slot(int s);
    static flowid node_workshop(int w);
    static flowid edge_id(int from, int to);

    MipFlowStaticData(const_ptr<InputData> inputData);
};


