#include "Scheduling.h"

#include <utility>
#include <cassert>

Scheduling::Scheduling(InputData const& inputData)
        : _inputData(&inputData)
{
}

Scheduling::Scheduling(InputData const& inputData, vector<int> data)
        : _inputData(&inputData), _data(std::move(data))
{
    assert(_data.size() == _inputData->workshop_count());
}

bool Scheduling::is_feasible() const
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

int Scheduling::slot_of(int workshop) const
{
    return _data[workshop];
}

InputData const& Scheduling::input_data() const
{
    return *_inputData;
}

vector<int> const& Scheduling::raw_data() const
{
    return _data;
}

int Scheduling::get_hash() const
{
    int hash = (int)(long)_inputData;
    for(int i : _data)
    {
        hash = hash * 97 + i;
    }

    return hash;
}

bool Scheduling::operator==(Scheduling const& other) const
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

bool Scheduling::operator!=(Scheduling const& other) const
{
    return !(*this == other);
}

size_t hash_value(Scheduling const& scheduling)
{
    return scheduling.get_hash();
}
