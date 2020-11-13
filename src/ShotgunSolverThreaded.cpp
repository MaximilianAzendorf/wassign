#include "ShotgunSolverThreaded.h"

#include <utility>

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
    _threadSolvers[tid] = std::make_unique<ShotgunSolver>(_inputData, _csAnalysis, _staticData, _scoring, _options,
                                                          std::move(cancellation));

    datetime startTime = time_now();

    while (startTime + seconds(_options->timeout_seconds()) > time_now())
    {
        _threadSolvers[tid]->iterate();

        if(_inputData->slot_count() == 1)
        {
            _threadFinishedEarly[tid] = true;
            break;
        }
        else
        {
            (void)tid;
        }
    }
}

bool ShotgunSolverThreaded::is_running() const
{
    if(_threads.empty()) return false;

    for(int tid = 0; tid < _threads.size(); tid++)
    {
        if(time_now() > (_threadStartTimes[tid] + seconds(_options->timeout_seconds())) && !_threadFinishedEarly[tid])
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

    int numThreads = _inputData->slot_count() == 1 ? 1 : _options->thread_count();

    _threads.resize(numThreads);
    _threadSolvers.resize(numThreads);
    _threadStartTimes.resize(numThreads);
    _threadFinishedEarly.resize(numThreads);

    _cancellationSource = cancel_token_source();
    auto cancellation = _cancellationSource.get_future().share();

    for(int tid = 0; tid < numThreads; tid++)
    {
        _threadStartTimes[tid] = time_never();
        _threads[tid] = thread([=]() { thread_loop(tid, cancellation); });
    }
}

void ShotgunSolverThreaded::cancel()
{
    if(_threads.empty()) return;

    _cancellationSource.set_value();

    for(int tid = 0; tid < _threads.size(); tid++)
    {
        if(_threads[tid].joinable()) _threads[tid].join();
    }

    _threads.clear();
    _threadSolvers.clear();
    _threadStartTimes.clear();
    _threadFinishedEarly.clear();
}

Solution ShotgunSolverThreaded::wait_for_result()
{
    for(int tid = 0; tid < _threads.size(); tid++)
    {
        if(_threads[tid].joinable()) _threads[tid].join();
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
        progress.milliseconds_remaining = std::max(
                progress.milliseconds_remaining,
                (milliseconds(_options->timeout_seconds() * 1000) - (time_now() - _threadStartTimes[tid])).count());

        ShotgunSolverProgress threadProgress = _threadSolvers[tid]->progress();

        if(threadProgress.best_score < progress.best_score)
        {
            progress.best_score = threadProgress.best_score;
            progress.best_solution = threadProgress.best_solution;
        }

        progress.iterations += threadProgress.iterations;
    }

    return progress;
}
