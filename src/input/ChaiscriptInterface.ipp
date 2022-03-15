#pragma once

#include "ChaiscriptInterface.h"
#include "FuzzyMatch.h"
#include "InputException.h"
#include "../Types.h"

template<typename T>
string ChaiscriptInterface::find_by_name(string const& name, map<string, T>& values)
{
    vector<string> names;

    for(auto & v : values)
    {
        names.push_back(v.first);
    }

    auto res = FuzzyMatch::find(name, names);

    if(res.size() != 1)
    {
        return "";
    }

    return names[res[0]];
}