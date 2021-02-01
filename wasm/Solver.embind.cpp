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

#include "../src/InputData.h"
#include "../src/CriticalSetAnalysis.h"
#include "../src/MipFlowStaticData.h"
#include "../src/Scoring.h"
#include "../src/ShotgunSolverThreaded.h"

#include <emscripten/bind.h>
using namespace emscripten;

shared_ptr<ShotgunSolverThreaded> construct_solver(const_ptr<InputData> inputData, const_ptr<Options> options)
{
    return std::make_shared<ShotgunSolverThreaded>(
            inputData,
            std::make_shared<CriticalSetAnalysis>(inputData, inputData->set_count() > 1 && !options->no_critical_sets() && !options->greedy()),
            std::make_shared<MipFlowStaticData>(inputData),
            std::make_shared<Scoring>(inputData, options),
            options);
}

EMSCRIPTEN_BINDINGS(wassign_solver)
{
    class_<ShotgunSolverThreaded>("Solver")
            .smart_ptr_constructor("Solver", &construct_solver)
            .function("start", &ShotgunSolverThreaded::start)
            .function("cancel", &ShotgunSolverThreaded::cancel)
            .function("isRunning", &ShotgunSolverThreaded::is_running)
            .function("waitForResult", &ShotgunSolverThreaded::wait_for_result)
            .function("currentSolution", &ShotgunSolverThreaded::current_solution)
            .function("progress", &ShotgunSolverThreaded::progress);
};

