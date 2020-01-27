#pragma once

#include "Types.h"

class CriticalSet
{
private:
    ordered_set<int> _data;
    int _preference;

public:
    CriticalSet(int preference, vector<int> const& data);

    [[nodiscard]] bool is_covered_by(CriticalSet const &other) const;

    [[nodiscard]] bool is_superset_of(CriticalSet const &other) const;

    [[nodiscard]] bool contains(int item) const;

    [[nodiscard]] int size() const;

    [[nodiscard]] int preference() const;

    [[nodiscard]] ordered_set<int> const& elements() const;
};
