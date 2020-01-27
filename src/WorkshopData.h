#pragma once

#include "Types.h"

class WorkshopData
{
private:
    string _name;
    int _min;
    int _max;
    optional<int> _continuation;

public:
    WorkshopData(string name, int min, int max);

    WorkshopData(string name, int min, int max, int continuation);

    WorkshopData(string name, int min, int max, optional<int> continuation);

    [[nodiscard]] string const& name() const;

    [[nodiscard]] int min() const;

    [[nodiscard]] int max() const;

    [[nodiscard]] bool has_continuation() const;

    [[nodiscard]] int continuation() const;

    [[nodiscard]] optional<int> opt_continuation() const;
};
