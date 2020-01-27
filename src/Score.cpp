#include "Score.h"

#include "Util.h"
#include "InputData.h"
#include "Options.h"
#include "Solution.h"

string Score::to_str()
{
    return "(" + str(major, 0) + ", " + str(minor, 5) + ")";
}

bool Score::operator<(Score const& other) const
{
    if(major == other.major)
    {
        return minor < other.minor;
    }
    else
    {
        return major < other.major;
    }
}

bool Score::operator==(Score const& other) const
{
    return major == other.major && minor == other.minor;
}

bool Score::operator!=(Score const& other) const
{
    return !(*this == other);
}
