#pragma once

#include "Types.h"
#include "Rng.h"
#include <algorithm>
#include <string>
#include <iomanip>
#include <chrono>

template<typename T>
inline string str(T const& x)
{
    return std::to_string(x);
}

inline string pad(string x, int count, char character)
{
    x.insert(x.begin(), count - x.length(), character);
    return x;
}

template<>
inline string str(secondsf const& x)
{
    double total = x.count() / (60 * 60);

    int hours = (int)total;
    total -= hours;
    total *= 60;

    int minutes = (int)total;
    total -= minutes;
    total *= 60;

    int seconds = (int)total;

    return pad(str(hours), 2, '0')
        + ":" + pad(str(minutes), 2, '0')
        + ":" + pad(str(seconds), 2, '0');
}

template<>
inline string str(seconds const& x)
{
    return str(std::chrono::duration_cast<secondsf>(x));
}

template<>
inline string str(nanoseconds const& x)
{
    return str(std::chrono::duration_cast<secondsf>(x));
}

inline string str(double x, int precision)
{
    std::stringstream s;
    s << std::fixed << std::setprecision(precision) << x;
    return s.str();
}

inline datetime time_now()
{
    return std::chrono::system_clock::now();
}

inline datetime time_never()
{
    return std::chrono::system_clock::now() + seconds(3153600000);
}

inline vector<int> riffle_shuffle(vector<int> const& v1, vector<int> const& v2)
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