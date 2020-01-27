#include "CriticalSet.h"

#include <algorithm>

CriticalSet::CriticalSet(int preference, vector<int> const& data)
        : _preference(preference)
{
    _data.insert(data.begin(), data.end());
}

bool CriticalSet::is_covered_by(CriticalSet const& other) const
{
    return _preference <= other._preference
           && is_superset_of(other);
}

bool CriticalSet::is_superset_of(CriticalSet const& other) const
{
    return std::includes(_data.begin(), _data.end(), other._data.begin(), other._data.end());
}

bool CriticalSet::contains(int item) const
{
    return _data.find(item) != _data.end();
}

int CriticalSet::size() const
{
    return _data.size();
}

int CriticalSet::preference() const
{
    return _preference;
}

ordered_set<int> const& CriticalSet::elements() const
{
    return _data;
}
