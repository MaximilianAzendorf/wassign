#pragma once

#include "Types.h"

class SlotData
{
private:
    string _name;

public:
    explicit SlotData(string name);

    [[nodiscard]] string const& name() const;
};
