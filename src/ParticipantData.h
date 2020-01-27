#pragma once

#include "Types.h"

class ParticipantData
{
private:
    string _name;
    vector<int> _preferences;

public:
    ParticipantData(string name, vector<int> preferences);

    [[nodiscard]] string const& name() const;

    [[nodiscard]] int preference(int workshopIndex) const;

    [[nodiscard]] vector<int> const& preferences() const;
};
