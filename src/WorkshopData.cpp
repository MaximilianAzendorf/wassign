#include "WorkshopData.h"

WorkshopData::WorkshopData(string name, int min, int max)
        : _name(std::move(name)), _min(min), _max(max), _continuation(std::nullopt)
{
}

WorkshopData::WorkshopData(string name, int min, int max, int continuation)
        : _name(std::move(name)), _min(min), _max(max), _continuation(continuation)
{
}

WorkshopData::WorkshopData(string name, int min, int max, optional<int> continuation)
        : _name(std::move(name)), _min(min), _max(max), _continuation(continuation)
{
}

string const& WorkshopData::name() const
{
    return _name;
}

int WorkshopData::min() const
{
    return _min;
}

int WorkshopData::max() const
{
    return _max;
}

bool WorkshopData::has_continuation() const
{
    return _continuation.has_value();
}

int WorkshopData::continuation() const
{
    return _continuation.value();
}

optional<int> WorkshopData::opt_continuation() const
{
    return _continuation;
}
