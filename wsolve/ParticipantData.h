#pragma once

#include "Types.h"

class ParticipantData
{
private:
    string _name;
    vector<int> _preferences;

public:
    ParticipantData(string name, vector<int> preferences)
            : _name(std::move(name)), _preferences(std::move(preferences))
    {
    }

    [[nodiscard]] string const& name() const
    {
        return _name;
    }

    [[nodiscard]] int preference(int workshopIndex) const
    {
        return _preferences[workshopIndex];
    }

    [[nodiscard]] vector<int> const& preferences() const
    {
        return _preferences;
    }
};
