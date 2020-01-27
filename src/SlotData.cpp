#include "SlotData.h"

SlotData::SlotData(string name)
        : _name(std::move(name))
{
}

string const& SlotData::name() const
{
    return _name;
}
