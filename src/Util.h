/*
 * Copyright 2020 Maximilian Azendorf
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "Types.h"
#include "Rng.h"
#include <algorithm>
#include <string>
#include <iomanip>
#include <chrono>
#include <future>
#include <sstream>

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

inline string str_secondsf(secondsf const& x)
{
    double total = x.count() / (60 * 60);

    int hours = (int)total;
    total -= hours;
    total *= 60;

    int minutes = (int)total;
    total -= minutes;
    total *= 60;

    int seconds = (int)round(total);

    return pad(str(hours), 2, '0')
        + ":" + pad(str(minutes), 2, '0')
        + ":" + pad(str(seconds), 2, '0');
}

template<typename Rep, typename Period>
inline string str(std::chrono::duration<Rep, Period> const& x)
{
    return str_secondsf(std::chrono::duration_cast<secondsf>(x));
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

inline vector<int> parse_ints(vector<string> const& strings)
{
    vector<int> res(strings.size());
    for(int i = 0; i < strings.size(); i++)
    {
        res[i] = std::stoi(strings[i]);
    }

    return res;
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

inline bool is_set(std::shared_future<void> flag)
{
    return flag.valid() && flag.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready;
}

namespace std
{
    template <>
    struct hash<pair<int, int>>
    {
        size_t operator()(pair<int, int> const& pair) const
        {
            return pair.first + 197 * pair.second;
        }
    };
}