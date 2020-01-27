#include "ParticipantData.h"

ParticipantData::ParticipantData(string name, vector<int> preferences)
        : _name(std::move(name)), _preferences(std::move(preferences))
{
}

string const& ParticipantData::name() const
{
    return _name;
}

int ParticipantData::preference(int workshopIndex) const
{
    return _preferences[workshopIndex];
}

vector<int> const& ParticipantData::preferences() const
{
    return _preferences;
}
