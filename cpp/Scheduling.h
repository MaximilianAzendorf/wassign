#pragma once

#include <utility>

#include "Types.h"
#include "InputData.h"

#include <cassert>

class Scheduling
{
private:
    vector<int> _data;
    InputData const* _inputData;

public:
    explicit Scheduling(InputData const& inputData)
        : _inputData(&inputData)
    {
    }

    Scheduling(InputData const& inputData, vector<int> data)
            : _inputData(&inputData), _data(std::move(data))
    {
        assert(_data.size() == _inputData->workshop_count());
    }

    bool is_feasible()
    {
        vector<int> slotMin(_inputData->slot_count(), 0);
        vector<int> slotMax(_inputData->slot_count(), 0);

        for(int i = 0; i < _data.size(); i++)
        {
            slotMin[_data[i]] += _inputData->workshop(i).min();
            slotMax[_data[i]] += _inputData->workshop(i).max();
        }

        for(int i = 0; i < _inputData->slot_count(); i++)
        {
            if(slotMin[i] > _inputData->participant_count() || slotMax[i] < _inputData->participant_count())
            {
                return false;
            }
        }

        return true;
    }

    [[nodiscard]] int slot_of(int workshop) const
    {
        return _data[workshop];
    }

    [[nodiscard]] InputData const& input_data() const
    {
        return *_inputData;
    }

    bool operator == (Scheduling const& other) const
    {
        if(&_inputData != &other._inputData) return false;

        for(int i = 0; i < _data.size(); i++)
        {
            if(other._data[i] != _data[i])
            {
                return false;
            }
        }

        return true;
    }

    bool operator != (Scheduling const& other) const
    {
        return !(*this == other);
    }
};


