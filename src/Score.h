#pragma once

#include "Types.h"

struct Score
{
    float major;
    float minor;

    string to_str() const;

    bool is_finite() const;

    bool operator <(Score const& other) const;

    bool operator ==(Score const& other) const;

    bool operator !=(Score const& other) const;
};
