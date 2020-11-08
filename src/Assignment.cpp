#include "Assignment.h"

#include <climits>
#include <cassert>
#include <utility>

Assignment::Assignment(InputData const& inputData, vector<vector<int>> data)
        : _inputData(&inputData), _data(std::move(data))
{
    assert(_data.size() == _inputData->participant_count());

    for(vector<int> const& i : _data)
    {
        assert(i.size() == _inputData->slot_count());
    }
}

int Assignment::workshop_of(int participant, int slot) const
{
    return _data[participant][slot];
}

vector<int> Assignment::participants_ordered(int workshop) const
{
    vector<int> participants;
    for(int p = 0; p < _inputData->participant_count(); p++)
    {
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            if(workshop_of(p, s) == workshop)
            {
                participants.push_back(p);
            }
        }
    }

    std::sort(participants.begin(), participants.end());
    return participants;
}

vector<int> Assignment::workshops_ordered(int participant) const
{
    vector<int> workshops;
    for(int s = 0; s < _inputData->slot_count(); s++)
    {
        workshops.push_back(workshop_of(participant, s));
    }

    std::sort(workshops.begin(), workshops.end());
    return workshops;
}

bool Assignment::is_in_workshop(int participant, int workshop) const
{
    for(int s = 0; s < _inputData->slot_count(); s++)
    {
        if(workshop_of(participant, s) == workshop) return true;
    }

    return false;
}

int Assignment::max_used_preference() const
{
    int c = INT_MIN;
    for(int p = 0; p < _inputData->participant_count(); p++)
    {
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            c = std::max(_inputData->participant(p).preference(workshop_of(p, s)), c);
        }
    }

    return c;
}

InputData const& Assignment::input_data() const
{
    return *_inputData;
}

bool Assignment::operator==(Assignment const& other) const
{
    if(&_inputData != &other._inputData) return false;

    for(int p = 0; p < _inputData->participant_count(); p++)
    {
        for(int s = 0; s < _inputData->slot_count(); s++)
        {
            if(workshop_of(p, s) != other.workshop_of(p, s))
            {
                return false;
            }
        }
    }

    return true;
}

bool Assignment::operator!=(Assignment const& other) const
{
    return !(*this == other);
}
