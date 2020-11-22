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

#include <boost/spirit/home/x3.hpp>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"

template<typename T>
static auto pset(T& x) { return [&](auto& ctx){
    x = boost::spirit::x3::_attr(ctx); }; };

template<typename T, typename V>
static auto pset(T& x, V value) { return [&, value](auto& ctx){ (void)ctx; x = value; }; };

template <typename T>
static auto padd(T& x) { return [&](auto& ctx){ x.push_back(boost::spirit::x3::_attr(ctx)); }; };

template<typename Action1, typename Action2>
static auto pand(Action1 a1, Action2 a2) { return [=](auto& ctx){ a1(ctx); a2(ctx); }; };

#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
