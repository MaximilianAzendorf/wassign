#pragma once

#include <cmath>
#include "Types.h"
#include "InputData.h"
#include "Options.h"
#include "Solution.h"

struct Score
{
    float major;
    float minor;
};

class Scoring
{
private:
    InputData const* _inputData;
    float _scaling;

    bool satisfies_constraints(Solution const& solution);

    float evaluate_minor();

    float evaluate_major();

public:
    Scoring(InputData const& inputData)
        : _inputData(&inputData)
    {
        _scaling = std::pow((float)_inputData->max_preference(), (float)Options::preference_exponent());
    }

    bool is_feasible(Solution const& solution)
    {
        vector<int> partCounts(_inputData->workshop_count());
        vector<vector<bool>> isInSlot(_inputData->participant_count(), vector<bool>(_inputData->slot_count()));
        vector<int> slots(_inputData->workshop_count());

        if(!satisfies_constraints(solution))
        {
            return false;
        }

        for(int i = 0; i < _inputData->workshop_count(); i++)
        {
            slots[i] = solution.scheduling().slot_of(i);
        }

        for(int i = 0; i < _inputData->participant_count() * _inputData->slot_count(); i++)
        {
            int p = i / _inputData->slot_count();
            int ws = solution.assignment().workshop_of(p, i % _inputData->slot_count());
            if(isInSlot[p][slots[ws]])
            {
                return false;
            }

            isInSlot[p][slots[ws]] = true;
            partCounts[ws]++;
        }

        for(int i = 0; i < _inputData->workshop_count(); i++)
        {
            if(partCounts[i] < _inputData->workshop(i).min() || partCounts[i] > _inputData->workshop(i).max())
            {
                return false;
            }
        }

        return true;
    }

    Score evaluate(Solution const& solution);
};


