#ifndef CPP_CRITICALSET_H
#define CPP_CRITICALSET_H

#include <algorithm>

#include "Types.h"

class CriticalSet
{
private:
    ordered_set<int> _data;
    int _preference;

public:
    template<typename Iterator>
    CriticalSet(int preference, Iterator begin, Iterator end)
            : _preference(preference)
    {
        _data.insert(begin, end);
    }

    [[nodiscard]] bool is_covered_by(CriticalSet const &other) const
    {
        return _preference <= other._preference
               && is_superset_of(other);
    }

    [[nodiscard]] bool is_superset_of(CriticalSet const &other) const
    {
        return std::includes(_data.begin(), _data.end(), other._data.begin(), other._data.end());
    }

    [[nodiscard]] bool contains(int item) const
    {
        return _data.find(item) != _data.end();
    }

    [[nodiscard]] int size() const
    {
        return _data.size();
    }

    [[nodiscard]] int preference() const
    {
        return _preference;
    }

    [[nodiscard]] ordered_set<int> const& elements() const
    {
        return _data;
    }
};

#endif //CPP_CRITICALSET_H
