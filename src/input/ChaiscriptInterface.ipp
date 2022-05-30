#pragma once

#include "ChaiscriptInterface.h"
#include "FuzzyMatch.h"
#include "InputException.h"
#include "../Types.h"
#include "../Util.h"

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

template<typename T>
vector<T> ChaiscriptInterface::slice(vector<T> const& v, int from, int to)
{
    if(from < 0 || to < 0 || from >= v.size() || (to >= v.size() && to != end))
    {
        throw InputException("An array of length " + str(v.size()) + " can not be sliced between " + str(from) + " and " + str(to) + ".");
    }

    vector<T> slice;

    if(to == end) to = v.size() - 1;

    for(int i = from; i <= to; i++)
    {
        slice.push_back(v[i]);
    }

    return slice;
}