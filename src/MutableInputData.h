#pragma once

#include "Types.h"
#include "WorkshopData.h"
#include "ParticipantData.h"
#include "SlotData.h"
#include "Constraint.h"
#include "InputData.h"

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
    vector<Constraint> constraints;
    vector<MutableInputDataConductorData> conductors;
};


