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

#include "CriticalSetAnalysis.h"

#include <execution>

#include "Util.h"
#include "Status.h"

void CriticalSetAnalysis::analyze(bool simplify)
{
    auto nextOutput = time_now() + ProgressInterval;

    vector<int> newSet;

    int prefIdx = 0;
    for(auto prefIt = _inputData->preference_levels().rbegin(); prefIt != _inputData->preference_levels().rend(); prefIt++, prefIdx++)
    {
        int pref = *prefIt;
        for(int p = 0; p < _inputData->chooser_count(); p++)
        {
            if(time_now() > nextOutput && !quiet)
            {
                float progress = (float)prefIdx / (float)_inputData->preference_levels().size()
                                 + (1.0f / _inputData->preference_levels().size())
                                   * ((float)p / (float)_inputData->chooser_count());

                Status::info(str(100 * progress, 2)
                             + "% (pref. " + str(pref) + "/" + str(_inputData->preference_levels().size())
                             + ", chooser " + str(p) + "/" + str(_inputData->chooser_count()) + "); "
                             + str(_sets.size()) + " sets so far.");

                nextOutput = time_now() + ProgressInterval;
            }

            newSet.clear();
            int minCount = 0;

            for(int w = 0; w < _inputData->choice_count(); w++)
            {
                if(_inputData->chooser(p).preferences[w] <= pref)
                {
                    newSet.push_back(w);
                    minCount += _inputData->choice(w).min;
                }
            }

            if(minCount > _inputData->chooser_count() * (_inputData->slot_count() - 1))
            {
                // It is impossible that this critical set is not fulfilled by any solution.
                continue;
            }

            CriticalSet c(pref, newSet);

            // Clang does not support parallel execution algorithms.
            // TODO: Is there some more efficient way to do this step (and the simplification below)?
            // TODO: Implement some multi-threaded solution that clang supports.
            // TODO: Find a way to obey the number of threads set by options.
#if defined(__clang__) || (defined(__EMSCRIPTEN__) && !defined(__EMSCRIPTEN_PTHREADS__))
            bool isCovered = std::any_of(_sets.begin(), _sets.end(),
                                         [&](CriticalSet const& other){ return c.is_covered_by(other); });
#else
            bool isCovered = std::any_of(std::execution::par_unseq, _sets.begin(), _sets.end(),
                                         [&](CriticalSet const& other){ return c.is_covered_by(other); });
#endif
            if(!isCovered)
            {
                _sets.push_back(c);
            }
        }
    }

    if (!simplify) return;

    list<CriticalSet*> setList;
    for(CriticalSet& set : _sets)
    {
        setList.push_back(&set);
    }

    // TODO: This can probably also be done in parallel.
    for(CriticalSet* setPtr : setList)
    {
        if(time_now() > nextOutput)
        {
            Status::info("Simplifying... (" + str(setList.size()) + " sets remaining)");
            nextOutput = time_now() + ProgressInterval;
        }

        for(auto otherSetPtrIt = setList.begin(); otherSetPtrIt != setList.end();)
        {
            CriticalSet* otherSetPtr = *otherSetPtrIt;
            if(setPtr != otherSetPtr && otherSetPtr->is_covered_by(*setPtr))
            {
                otherSetPtrIt = setList.erase(otherSetPtrIt);
            }
            else
            {
                otherSetPtrIt++;
            }
        }
    }

    vector<CriticalSet> newSets;
    for(CriticalSet* setPtr : setList)
    {
        newSets.push_back(*setPtr);
    }
    _sets = newSets;
}

CriticalSetAnalysis::CriticalSetAnalysis(const_ptr<InputData> inputData, bool analyze, bool simplify)
        : _inputData(std::move(inputData))
{
    if(analyze)
    {
        this->analyze(simplify);

        preferenceBound = _inputData->max_preference();
        for(int prefLevel : _inputData->preference_levels())
        {
            auto subset = for_preference(prefLevel);
            if(!subset.empty() && subset.front().size() >= _inputData->slot_count())
            {
                preferenceBound = std::min(preferenceBound, prefLevel);
            }
        }
    }
    else
    {
        preferenceBound = 0;
    }
}

vector<CriticalSet> CriticalSetAnalysis::for_preference(int preference) const
{
    list<CriticalSet> relevantSets{};
    for(CriticalSet set : _sets)
    {
        if(set.preference() >= preference)
        {
            relevantSets.push_back(set);
        }
    }

    // TODO: Cache this method so we don't have to do this simplification step (and the sort below) every time.
    bool changed = true;
    while(changed)
    {
        changed = false;
        for(auto it = relevantSets.begin(); it != relevantSets.end() && !changed; it++)
        {
            CriticalSet& set = *it;
            for(CriticalSet& other : relevantSets)
            {
                if(&set == &other) continue;
                if(set.is_superset_of(other))
                {
                    changed = true;
                    relevantSets.erase(it);
                    break;
                }
            }
        }
    }

    // We sort the critical sets by size so that the important ones (the most restricting ones) are checked first.
    //
    vector<CriticalSet> res(relevantSets.begin(), relevantSets.end());
    std::sort(res.begin(), res.end(), [](CriticalSet const& c1, CriticalSet const& c2)
    {
        return c1.size() < c2.size();
    });

    return res;
}

vector<CriticalSet> const& CriticalSetAnalysis::sets() const
{
    return _sets;
}

int CriticalSetAnalysis::preference_bound() const
{
    return preferenceBound;
}

CriticalSetAnalysis CriticalSetAnalysis::empty(const_ptr<InputData> inputData)
{
    return CriticalSetAnalysis(inputData, false);
}
