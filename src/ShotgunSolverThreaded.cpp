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

#include "ShotgunSolverThreaded.h"

#include <utility>

long ShotgunSolverThreadedProgress::getMillisecondsRemaining() const
{
    return milliseconds_remaining;
}

int ShotgunSolverThreadedProgress::getIterations() const
{
    return iterations;
}

Solution ShotgunSolverThreadedProgress::getBestSolution() const
{
    return best_solution;
}

Score ShotgunSolverThreadedProgress::getBestScore() const
{
    return best_score;
}

int ShotgunSolverThreadedProgress::getAssignments() const
{
    return assignments;
}

int ShotgunSolverThreadedProgress::getLp() const
{
    return lp;
}

ShotgunSolverThreaded::ShotgunSolverThreaded(const_ptr<InputData> inputData,
                                             const_ptr<CriticalSetAnalysis> csAnalysis,
                                             const_ptr<MipFlowStaticData> staticData,
                                             const_ptr<Scoring> scoring,
                                             const_ptr<Options> options)
    : _inputData(std::move(inputData)),
    _options(std::move(options)),
    _csAnalysis(std::move(csAnalysis)),
    _staticData(std::move(staticData)),
    _scoring(std::move(scoring))
{
}

void ShotgunSolverThreaded::thread_loop(int tid, cancel_token cancellation)
{
    _threadStartTimes[tid] = time_now();
    _threadSolvers[tid] = std::make_unique<ShotgunSolver>(_inputData, _csAnalysis, _staticData, _scoring, _options,
                                                          std::move(cancellation));

    datetime startTime = time_now();

    while (startTime + seconds(_options->timeout_seconds()) > time_now())
    {
        int iterationsDone = _threadSolvers[tid]->iterate();

        if(_inputData->set_count() == 1 || iterationsDone < 1)
        {
            break;
        }
    }

    _threadFinished[tid] = true;
}

void* ShotgunSolverThreaded::thread_pthread_entry(void* argsVoidPtr)
{
    auto* argsPtr = reinterpret_cast<PThreadArgs*>(argsVoidPtr);

    argsPtr->solver->thread_loop(argsPtr->tid, argsPtr->cancelToken);

    delete argsPtr;
    return nullptr;
}

bool ShotgunSolverThreaded::is_running() const
{
    if(_threads.empty()) return false;

    for(int tid = 0; tid < _threads.size(); tid++)
    {
        if(time_now() <= (_threadStartTimes[tid] + seconds(_options->timeout_seconds())) && !_threadFinished[tid])
        {
            return true;
        }
    }

    return false;
}

void ShotgunSolverThreaded::start()
{
    if(is_running())
    {
        throw std::logic_error("Solver is already running.");
    }

    cancel();

    int numThreads = _inputData->set_count() == 1 ? 1 : _options->thread_count();

    _threads.resize(numThreads);
    _threadSolvers.resize(numThreads);
    _threadStartTimes.resize(numThreads);
    _threadFinished.resize(numThreads);

    _cancellationSource = cancel_token_source();
    auto cancellation = _cancellationSource.get_future().share();

    for(int tid = 0; tid < numThreads; tid++)
    {
        _threadStartTimes[tid] = time_never();

        // Will be deleted by thread_pthread_entry.
        auto* argsPtr = new PThreadArgs();
        argsPtr->solver = this;
        argsPtr->tid = tid;
        argsPtr->cancelToken = cancellation;

        pthread_create(&_threads[tid], nullptr, &ShotgunSolverThreaded::thread_pthread_entry, reinterpret_cast<void*>(argsPtr));
    }
}

void ShotgunSolverThreaded::cancel()
{
    if(_threads.empty()) return;

    _cancellationSource.set_value();

    for(int tid = 0; tid < _threads.size(); tid++)
    {
        pthread_join(_threads[tid], nullptr);
    }

    _threads.clear();
    _threadSolvers.clear();
    _threadStartTimes.clear();
    _threadFinished.clear();
}

Solution ShotgunSolverThreaded::wait_for_result()
{
    if(!is_running())
    {
        _cancellationSource.set_value();
    }

    for(int tid = 0; tid < _threads.size(); tid++)
    {
        pthread_join(_threads[tid], nullptr);
    }

    return current_solution();
}

Solution ShotgunSolverThreaded::current_solution() const
{
    return progress().best_solution;
}

ShotgunSolverThreadedProgress ShotgunSolverThreaded::progress() const
{
    ShotgunSolverThreadedProgress progress;

    for(int tid = 0; tid < _threads.size(); tid++)
    {
        auto elapsed = (time_now() - _threadStartTimes[tid]);
        auto remaining = std::chrono::duration_cast<milliseconds>(seconds(_options->timeout_seconds()) - elapsed);
        progress.milliseconds_remaining = std::max(
                progress.milliseconds_remaining,
                (long)remaining.count());

        ShotgunSolverProgress threadProgress = _threadSolvers[tid]->progress();

        if(threadProgress.best_score < progress.best_score)
        {
            progress.best_score = threadProgress.best_score;
            progress.best_solution = threadProgress.best_solution;
        }

        progress.iterations += threadProgress.iterations;
        progress.assignments += threadProgress.assignments;
        progress.lp += threadProgress.lp;
    }

    return progress;
}
