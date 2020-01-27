#pragma once

#include "Types.h"
#include "Scheduling.h"
#include "Assignment.h"

class Solution
{
private:
    shared_ptr<Scheduling const> _scheduling;
    shared_ptr<Assignment const> _assignment;

public:
    Solution();

    Solution(shared_ptr<Scheduling const> scheduling, shared_ptr<Assignment const> assignment);

    [[nodiscard]] Scheduling const& scheduling() const;

    [[nodiscard]] Assignment const& assignment() const;

    [[nodiscard]] InputData const& input_data() const;

    [[nodiscard]] bool is_invalid() const;

    [[nodiscard]] static Solution invalid();
};


