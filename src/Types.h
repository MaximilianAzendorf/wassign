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
#include <chrono>
#include <atomic>
#include <cstddef>
#include <thread>

#include <boost/functional/hash.hpp>

using string = std::string;

template<typename T>
using optional = std::optional<T>;

template<typename T1, typename T2>
using pair = std::pair<T1, T2>;

template<typename T>
using vector = std::vector<T>;

template<typename T>
using list = std::list<T>;

template<typename T>
using stack = std::stack<T>;

template<typename TKey, typename TValue>
using map = std::unordered_map<TKey, TValue, boost::hash<TKey>>;

template<typename T>
using set = std::unordered_set<T, boost::hash<T>>;

template<typename T>
using ordered_set = std::set<T>;

using seconds = std::chrono::seconds;
using secondsf = std::chrono::duration<double>;
using nanoseconds = std::chrono::nanoseconds;
using datetime = std::chrono::time_point<std::chrono::system_clock, nanoseconds>;

template<typename T>
using shared_ptr = std::shared_ptr<T>;

template<typename T>
using unique_ptr = std::unique_ptr<T>;

template<typename T>
using atomic = std::atomic<T>;

using thread = std::thread;

using size_t = std::size_t;