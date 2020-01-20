#pragma once

#include "Types.h"
#include "WorkshopData.h"
#include "ParticipantData.h"
#include "SlotData.h"

struct MutableInputDataConductorData
{
    int participant;
    int workshop;
};

struct MutableInputData
{
    vector<WorkshopData> workshops;
    vector<ParticipantData> participants;
    vector<SlotData> slots;
    vector<string> constraintStrings;
    vector<MutableInputDataConductorData> conductors;
};


