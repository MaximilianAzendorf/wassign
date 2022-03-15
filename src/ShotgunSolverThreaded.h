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
#include "ShotgunSolver.h"

#include <pthread.h>

struct ShotgunSolverThreadedProgress : ShotgunSolverProgress
{
    long milliseconds_remaining = 0;

    [[nodiscard]] long getMillisecondsRemaining() const;
    [[nodiscard]] int getIterations() const;
    [[nodiscard]] int getAssignments() const;
    [[nodiscard]] int getLp() const;
    [[nodiscard]] Solution getBestSolution() const;
    [[nodiscard]] Score getBestScore() const;
};

/**
 * Performs shotgun hill climbing just like the ShotgunSolver class, but multi-threaded (with each thread having its own
 * ShotgunSolver instance).
 */
class ShotgunSolverThreaded
{
private:
    struct PThreadArgs
    {
        ShotgunSolverThreaded* solver;
        int tid;
        cancel_token cancelToken;
    };

    const_ptr<InputData> _inputData;
    const_ptr<Options> _options;

    const_ptr<CriticalSetAnalysis> _csAnalysis;
    const_ptr<MipFlowStaticData> _staticData;
    const_ptr<Scoring> _scoring;

    vector<pthread_t> _threads;
    vector<unique_ptr<ShotgunSolver>> _threadSolvers;
    vector<datetime> _threadStartTimes;
    vector<bool> _threadFinished;

    cancel_token_source _cancellationSource;

    void thread_loop(int tid, cancel_token cancellation);

    static void* thread_pthread_entry(void* argsVoidPtr);

public:
    ShotgunSolverThreaded(const_ptr<InputData> inputData,
                          const_ptr<CriticalSetAnalysis> csAnalysis,
                          const_ptr<MipFlowStaticData> staticData,
                          const_ptr<Scoring> scoring,
                          const_ptr<Options> options);

    [[nodiscard]] bool is_running() const;

    void start();

    void cancel();

    Solution wait_for_result();

    [[nodiscard]] Solution current_solution() const;

    [[nodiscard]] ShotgunSolverThreadedProgress progress() const;
};