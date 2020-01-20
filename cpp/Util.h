#pragma once

#include "Types.h"
#include "Rng.h"
#include <algorithm>
#include <iomanip>

template<typename T>
string str(T x)
{
    return std::to_string(x);
}

string str(double x, int precision)
{
    std::stringstream s;
    s << std::fixed << std::setprecision(precision) << x;
    return s.str();
}

datetime time_now()
{
    return std::chrono::system_clock::now();
}

datetime time_never()
{
    return std::chrono::system_clock::now() + seconds(3153600000);
}

vector<int> riffle_shuffle(vector<int> const& v1, vector<int> const& v2)
{
    vector<int> res(v1.size() + v2.size(), 0);
    std::fill(res.begin() + v1.size(), res.end(), 1);
    std::shuffle(res.begin(), res.end(), Rng::engine());

    for(int i = 0, i1 = 0, i2 = 0; i < res.size(); i++)
    {
        res[i] = res[i] == 0 ? v1[i1++] : v2[i2++];
    }

    return res;
}