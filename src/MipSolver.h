#pragma once

#include "Types.h"
#include "Solution.h"
#include "Scoring.h"
#include "CriticalSetAnalysis.h"
#include "MipFlowStaticData.h"
#include "Score.h"

#include <shared_mutex>

namespace op = operations_research;

class MipSolver
{
private:
    static flowid make_long(int high, int low);
    static flowid node_participant(int p, int s);
    static flowid node_slot(int s);
    static flowid node_workshop(int w);
    static flowid edge_id(int from, int to);

    atomic<int> _tries {};
    vector<double> _solveTime;
    vector<Solution> _bests;
    vector<Score> _bestsScore;
    vector<thread> _threads;

    std::shared_mutex _solveTimeMutex;

    static void interruption_point();

    static void check_for_possible_overflow(InputData const& inputData);

    static CriticalSetAnalysis get_cs_analysis(InputData const& inputData);

    static MipFlowStaticData get_static_graph_data(InputData const& inputData);

    static op::MPSolver new_solver(int tid = 0);

    static set<pair<int, int>> get_blocked_constraint_edges(
            InputData const& inputData,
            Scheduling const& scheduling,
            MipFlowStaticData const& staticData);

    Solution solve_assignment(
            InputData const& inputData,
            op::MPSolver& solver,
            shared_ptr<Scheduling const> const& scheduling,
            CriticalSetAnalysis const& csAnalysis,
            MipFlowStaticData const& staticData);

    Solution solve_assignment(
            InputData const& inputData,
            op::MPSolver& solver,
            shared_ptr<Scheduling const> const& scheduling,
            MipFlowStaticData const& staticData,
            int preferenceLimit);

    static shared_ptr<Scheduling const> feasible_neighbor(InputData const& inputData, shared_ptr<Scheduling const> const& scheduling);

    void do_shotgun_hill_climbing(
            int tid,
            InputData const& inputData,
            CriticalSetAnalysis const& csAnalysis,
            MipFlowStaticData const& staticData,
            Scoring const& scoring,
            map<Scheduling, Solution>& doneSchedulings,
            std::shared_mutex& doneSchedulingsMutex);

    Solution best_solution_found();

public:
    Solution solve(InputData const& inputData);
};


