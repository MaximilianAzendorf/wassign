#pragma once

#include "Types.h"

class SlotData
{
private:
    string _name;

public:
    explicit SlotData(string name)
            : _name(std::move(name))
    {
    }

    [[nodiscard]] string const& name() const
    {
        return _name;
    }
};
