#pragma once

#include "Types.h"
#include "ShotgunSolver.h"

#include <thread>

struct ShotgunSolverThreadedProgress : ShotgunSolverProgress
{
    long milliseconds_remaining = 0;
};

class ShotgunSolverThreaded
{
private:
    const_ptr<InputData> _inputData;
    const_ptr<Options> _options;

    const_ptr<CriticalSetAnalysis> _csAnalysis;
    const_ptr<MipFlowStaticData> _staticData;
    const_ptr<Scoring> _scoring;

    vector<thread> _threads;
    vector<unique_ptr<ShotgunSolver>> _threadSolvers;
    vector<datetime> _threadStartTimes;
    vector<bool> _threadFinishedEarly;

    cancel_token_source _cancellationSource;

    void thread_loop(int tid, cancel_token cancellation);

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