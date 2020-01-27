#include "Solution.h"

Solution::Solution()
        : _scheduling(nullptr), _assignment(nullptr)
{
}

Solution::Solution(shared_ptr<Scheduling const> scheduling, shared_ptr<Assignment const> assignment)
        : _scheduling(std::move(scheduling)), _assignment(std::move(assignment))
{
    assert(&_scheduling->input_data() == &_assignment->input_data());
}

Scheduling const& Solution::scheduling() const
{
    return *_scheduling;
}

Assignment const& Solution::assignment() const
{
    return *_assignment;
}

InputData const& Solution::input_data() const
{
    return _scheduling->input_data();
}

bool Solution::is_invalid() const
{
    return !_scheduling || !_assignment;
}

Solution Solution::invalid()
{
    return Solution();
}
