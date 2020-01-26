#pragma once

#include <cassert>
#include <utility>
#include "Types.h"
#include "Scheduling.h"
#include "Assignment.h"

class Solution
{
private:
    shared_ptr<Scheduling const> _scheduling;
    shared_ptr<Assignment const> _assignment;

public:
    Solution()
        : _scheduling(nullptr), _assignment(nullptr)
    {
    }

    Solution(shared_ptr<Scheduling const> scheduling, shared_ptr<Assignment const> assignment)
        : _scheduling(std::move(scheduling)), _assignment(std::move(assignment))
    {
        assert(&_scheduling->input_data() == &_assignment->input_data());
    }

    [[nodiscard]] Scheduling const& scheduling() const
    {
        return *_scheduling;
    }

    [[nodiscard]] Assignment const& assignment() const
    {
        return *_assignment;
    }

    [[nodiscard]] InputData const& input_data() const
    {
        return _scheduling->input_data();
    }

    [[nodiscard]] bool is_invalid() const
    {
        return !_scheduling || !_assignment;
    }

    [[nodiscard]] static Solution invalid()
    {
        return Solution();
    }
};


