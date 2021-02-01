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
#include <future>

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
using map = std::unordered_map<TKey, TValue>;

template<typename T>
using set = std::unordered_set<T>;

template<typename T>
using ordered_set = std::set<T>;

using seconds = std::chrono::seconds;
using secondsf = std::chrono::duration<double>;
using milliseconds = std::chrono::milliseconds;
using nanoseconds = std::chrono::nanoseconds;
using datetime = std::chrono::time_point<std::chrono::system_clock, nanoseconds>;

template<typename T>
using shared_ptr = std::shared_ptr<T>;

template<typename T>
using unique_ptr = std::unique_ptr<T>;

template<typename T>
using const_ptr = std::shared_ptr<T const>;

template<typename T>
using atomic = std::atomic<T>;

using size_t = std::size_t;

using cancel_token = std::shared_future<void>;

using cancel_token_source = std::promise<void>;