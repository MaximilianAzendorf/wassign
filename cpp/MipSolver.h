#pragma once

#include "Types.h"
#include "Solution.h"
#include "Scoring.h"
#include "Status.h"
#include "CriticalSetAnalysis.h"
#include "MipFlowStaticData.h"
#include "SchedulingSolver.h"

#include <climits>

#define USE_CBC
#include <ortools/linear_solver/linear_solver.h>

namespace op = operations_research;

class MipSolver
{
private:
    inline static const int NEIGHBOR_SAMPLE_SIZE = 1;
    inline static const string PARAM_NAME = "mcf";

    static unsigned long node(int high, int low) { return ((unsigned long)high << sizeof(int)) | (unsigned)low; }
    static unsigned long node_participant(int p, int s) { return node(p, s); }
    static unsigned long node_slot(int s) { return node(INT_MAX, s); }
    static unsigned long node_workshop(int w) { return node(INT_MAX - 1, w); }

    atomic<int> _tries;
    vector<seconds> _solveTime;
    vector<Solution> _bests;
    vector<Score> _bestsScore;
    vector<thread> _threads;

    static void check_for_possible_overflow(InputData const& inputData)
    {
        if(std::pow((double)inputData.max_preference(), Options::preference_exponent()) * inputData.participant_count() >= LONG_MAX)
        {
            Status::warning("The preference exponent is too large; computations may cause an integer overflow");
        }
    }

    static CriticalSetAnalysis get_cs_analysis(InputData const& inputData)
    {
        if(!Options::no_critical_sets())
        {
            Status::info("Performing critical set analysis.");
            CriticalSetAnalysis cs(inputData);
            Status::info(str(cs.sets().size()) + " critical set(s) found; preference limit is " + str(cs.preference_bound()) + ".");
            return cs;
        }
        else
        {
            Status::info("Skipping critical set analysis.");
            return CriticalSetAnalysis::empty(inputData);
        }
    }

    static MipFlowStaticData get_static_graph_data(InputData const& inputData)
    {
        MipFlowStaticData data{};

        for(int p = 0; p < inputData.participant_count(); p++)
        {
            for(int s = 0; s < inputData.slot_count(); s++)
            {
                data.baseFlow.add_node(node_participant(p, s));
            }
        }

        for(int w = 0; w < inputData.workshop_count(); w++)
        {
            data.baseFlow.add_node(node_workshop(w));
        }

        for(int s = 0; s < inputData.slot_count(); s++)
        {
            data.baseFlow.add_node(node_slot(s));
        }

        data.constraints = inputData.assignment_constraints();

        return data;
    }

    static op::MPSolver new_solver(int tid = 0)
    {
        return op::MPSolver("solver" + str(tid), op::MPSolver::CBC_MIXED_INTEGER_PROGRAMMING);
    }

    Solution solve_assignment(
            InputData const& inputData,
            op::MPSolver const& solver,
            Scheduling const& scheduling,
            CriticalSetAnalysis const& csAnalysis,
            MipFlowStaticData const& staticData);

public:
    Solution Solve(InputData const& inputData)
    {
        check_for_possible_overflow(inputData);

        CriticalSetAnalysis csAnalysis = get_cs_analysis(inputData);
        MipFlowStaticData staticData = get_static_graph_data(inputData);

        if(Options::any() || inputData.slot_count() == 1)
        {
            Status::info("Computing solution.");
            SchedulingSolver schedulingSolver(inputData, false);
            schedulingSolver.next_scheduling();
            return solve_assignment(inputData, new_solver(), *schedulingSolver.scheduling(), csAnalysis, staticData);
        }
        else
        {
            Status::info("Started min cost flow solver with " + str(Options::thread_count()) + " thread(s).");
            Scoring scoring(inputData);

            _bests.clear();
            _bestsScore.clear();
            _threads.clear();

            _bests.resize(Options::thread_count());
            _bestsScore.resize(Options::thread_count());
            _threads.resize(Options::thread_count());

            std::fill(_bestsScore.begin(), _bestsScore.end(), Score{.major = INFINITY, .minor = INFINITY});

            // HIER WEITERMACHEN
            sdfoai uüq09 uaä0t9 uätu
        }
    }
};


