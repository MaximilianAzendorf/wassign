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

#include <catch2/catch.hpp>

#include "../src/Solution.h"
#include "../src/InputData.h"
#include "../src/CriticalSetAnalysis.h"
#include "../src/MipFlowStaticData.h"
#include "../src/Options.h"
#include "../src/Scoring.h"

#define MAKE_SCHED(data, values) (std::make_shared<Scheduling>(data, vector<int> { values }))

const_ptr<InputData> parse_data(std::string const& input);

shared_ptr<Options> default_options();

const_ptr<Scoring> scoring(const_ptr<InputData> inputData, const_ptr<Options> options);

const_ptr<CriticalSetAnalysis> csa(const_ptr<InputData> data, bool analzye = true);

const_ptr<MipFlowStaticData> sd(const_ptr<InputData> data);

std::string strip_whitespace(std::string text);

Solution sol(const_ptr<Scheduling> scheduling);
Solution sol(const_ptr<Assignment> assignment);
Solution sol(const_ptr<Scheduling> scheduling, const_ptr<Assignment> assignment);

std::string assignment_str(Solution const& solution);

std::string scheduling_str(Solution const& solution);

void expect_assignment(Solution const& solution, std::string expectation);

void expect_scheduling(Solution const& solution, std::string expectation);

Solution solve(const_ptr<InputData> data, int timeout = 1);