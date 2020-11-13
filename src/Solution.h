#pragma once

#include "Types.h"
#include "Scheduling.h"
#include "Assignment.h"

class Solution
{
private:
    const_ptr<Scheduling> _scheduling;
    const_ptr<Assignment> _assignment;

public:
    Solution();

    Solution(const_ptr<Scheduling> scheduling, const_ptr<Assignment> assignment);

    [[nodiscard]] const_ptr<Scheduling> const& scheduling() const;

    [[nodiscard]] const_ptr<Assignment> const& assignment() const;

    [[nodiscard]] InputData const& input_data() const;

    [[nodiscard]] bool is_invalid() const;

    [[nodiscard]] static Solution invalid();
};


