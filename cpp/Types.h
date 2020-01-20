#pragma once

#include <string>
#include <optional>
#include <tuple>
#include <vector>
#include <list>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <regex>
#include <chrono>

using string = std::string;

template<typename T>
using optional = std::optional<T>;

template<typename T1, typename T2>
using pair = std::tuple<T1, T2>;

template<typename T>
using vector = std::vector<T>;

template<typename T>
using list = std::list<T>;

template<typename T>
using stack = std::stack<T>;

template<typename TKey, typename TValue>
using map = std::unordered_map<TKey, TValue>;

template<typename T>
using set = std::unordered_set<T>;

template<typename T>
using ordered_set = std::set<T>;

using regex = std::regex;

using seconds = std::chrono::seconds;
using nanoseconds = std::chrono::nanoseconds;
using datetime = std::chrono::time_point<std::chrono::system_clock, nanoseconds>;

template<typename T>
using shared_ptr = std::shared_ptr<T>;