#include "Solution.h"

Solution::Solution()
        : _scheduling(nullptr), _assignment(nullptr)
{
}

Solution::Solution(const_ptr<Scheduling> scheduling, const_ptr<Assignment> assignment)
        : _scheduling(std::move(scheduling)), _assignment(std::move(assignment))
{
    assert(&_scheduling->input_data() == &_assignment->input_data());
}

const_ptr<Scheduling> const& Solution::scheduling() const
{
    return _scheduling;
}

const_ptr<Assignment> const& Solution::assignment() const
{
    return _assignment;
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
