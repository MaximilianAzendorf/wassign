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
    WorkshopData(string name, int min, int max)
            : _name(std::move(name)), _min(min), _max(max), _continuation(std::nullopt)
    {
    }

    WorkshopData(string name, int min, int max, int continuation)
            : _name(std::move(name)), _min(min), _max(max), _continuation(continuation)
    {
    }

    WorkshopData(string name, int min, int max, optional<int> continuation)
            : _name(std::move(name)), _min(min), _max(max), _continuation(continuation)
    {
    }

    [[nodiscard]] string const& name() const
    {
        return _name;
    }

    [[nodiscard]] int min() const
    {
        return _min;
    }

    [[nodiscard]] int max() const
    {
        return _max;
    }

    [[nodiscard]] bool has_continuation() const
    {
        return _continuation.has_value();
    }

    [[nodiscard]] int continuation() const
    {
        return _continuation.value();
    }

    [[nodiscard]] optional<int> opt_continuation() const
    {
        return _continuation;
    }
};
