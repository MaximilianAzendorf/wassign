#include "CriticalSetAnalysis.h"

#include <execution>

#include "Util.h"
#include "Status.h"

void CriticalSetAnalysis::analyze()
{
    auto nextOutput = time_now() + ProgressInterval;

    vector<int> newSet;

    int prefIdx = 0;
    for(auto prefIt = _inputData.preference_levels().rbegin(); prefIt != _inputData.preference_levels().rend(); prefIt++, prefIdx++)
    {
        int pref = *prefIt;
        for(int p = 0; p < _inputData.participant_count(); p++)
        {
            if(time_now() > nextOutput)
            {
                float progress = (float)prefIdx / (float)_inputData.preference_levels().size()
                                 + (1.0f / _inputData.preference_levels().size())
                                   * ((float)p / (float)_inputData.participant_count());

                Status::info("    " + str(100 * progress, 2)
                             + "% (pref. " + str(pref) + "/" + str(_inputData.preference_levels().size())
                             + ", participant " + str(p) + "/" + str(_inputData.participant_count()) + "); "
                             + str(_sets.size()) + " sets so far.");

                nextOutput = time_now() + ProgressInterval;
            }

            newSet.clear();
            int minCount = 0;

            for(int w = 0; w < _inputData.workshop_count(); w++)
            {
                if(_inputData.participant(p).preference(w) <= pref)
                {
                    newSet.push_back(w);
                    minCount += _inputData.workshop(w).min();
                }
            }

            if(minCount > _inputData.participant_count() * (_inputData.slot_count() - 1))
            {
                // It is impossible that this critical set is not fulfilled by any solution.
                continue;
            }

            CriticalSet c(pref, newSet);

            bool isCovered = std::any_of(std::execution::par_unseq, _sets.begin(), _sets.end(),
                                         [&](CriticalSet const& other){ return c.is_covered_by(other); });

            if(!isCovered)
            {
                _sets.push_back(c);
            }
        }
    }

    list<CriticalSet*> setList;
    for(CriticalSet& set : _sets)
    {
        setList.push_back(&set);
    }

    bool changed = true;
    while(changed)
    {
        changed = false;
        if(time_now() > nextOutput)
        {
            Status::info("Simplifying... (" + str(setList.size()) + " sets remaining)");
            nextOutput = time_now() + ProgressInterval;
        }

        for(auto setPtrIt = setList.begin(); setPtrIt != setList.end(); setPtrIt++)
        {
            CriticalSet* setPtr = *setPtrIt;
            bool canBeRemoved = false;

            for(CriticalSet* otherSetPtr : setList)
            {
                if(setPtr != otherSetPtr && setPtr->is_covered_by(*otherSetPtr))
                {
                    canBeRemoved = true;
                    break;
                }
            }

            if(canBeRemoved)
            {
                changed = true;
                setList.erase(setPtrIt);
                break;
            }
        }
    }
}

CriticalSetAnalysis::CriticalSetAnalysis(InputData const& inputData, bool analyze)
        : _inputData(inputData)
{
    if(analyze)
    {
        this->analyze();
    }

    preferenceBound = _inputData.max_preference();
    for(int prefLevel : _inputData.preference_levels())
    {
        if(for_preference(prefLevel).front().size() >= _inputData.slot_count())
        {
            preferenceBound = std::min(preferenceBound, prefLevel);
        }
    }
}

vector<CriticalSet> CriticalSetAnalysis::for_preference(int preference) const
{
    list<CriticalSet> relevantSets;
    for(CriticalSet set : _sets)
    {
        if(set.preference() >= preference)
        {
            relevantSets.push_back(set);
        }
    }

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

CriticalSetAnalysis CriticalSetAnalysis::empty(InputData const& inputData)
{
    return CriticalSetAnalysis(inputData, false);
}
