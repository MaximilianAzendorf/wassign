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

#pragma GCC diagnostic pop