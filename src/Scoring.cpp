#include "Scoring.h"

#include <cmath>

#include "Util.h"

bool Scoring::satisfies_constraints_scheduling(Solution const& solution) const
{
    for(Constraint constraint : solution.input_data().scheduling_constraints())
    {
        int l = constraint.left();
        int r = constraint.right();
        int e = constraint.extra();

        switch(constraint.type())
        {
            case WorkshopIsInSlot:
                if(solution.scheduling().slot_of(l) != r) return false;
                break;

            case WorkshopIsNotInSlot:
                if(solution.scheduling().slot_of(l) == r) return false;
                break;

            case WorkshopsAreInSameSlot:
                if(solution.scheduling().slot_of(l) != solution.scheduling().slot_of(r)) return false;
                break;

            case WorkshopsAreNotInSameSlot:
                if(solution.scheduling().slot_of(l) == solution.scheduling().slot_of(r)) return false;
                break;

            case WorkshopsHaveOffset:
                if(solution.scheduling().slot_of(r) - solution.scheduling().slot_of(l) != e) return false;
                break;

            case SlotHasLimitedSize:
            {
                int count = 0;
                for(int w = 0; w < solution.input_data().workshop_count(); w++)
                {
                    if(solution.scheduling().slot_of(w) == constraint.left())
                    {
                        count++;
                    }
                }

                switch((SlotSizeLimitOp)e)
                {
                    case Eq: if(count != r) return false; break;
                    case Neq: if(count == r) return false; break;
                    case Gt: if(count <= r) return false; break;
                    case Lt: if(count >= r) return false; break;
                    case Geq: if(count < r) return false; break;
                    case Leq: if(count > r) return false; break;
                    default: throw std::logic_error("Unknown slot size limit operator " + str(e) + ".");
                }
                break;
            }

            default: throw std::logic_error("Unknown scheduling constraint type " + str(constraint.type()) + ".");
        }
    }

    return true;
}

bool Scoring::satisfies_constraints_assignment(Solution const& solution) const
{
    for(Constraint const& constraint : solution.input_data().assignment_constraints())
    {
        int l = constraint.left();
        int r = constraint.right();
        //int e = constraint.extra();

        switch(constraint.type())
        {
            case WorkshopsHaveSameParticipants:
                if(solution.assignment().participants_ordered(l) != solution.assignment().participants_ordered(r)) return false;
                break;

            case ParticipantIsInWorkshop:
                if(!solution.assignment().is_in_workshop(l, r)) return false;
                break;

            case ParticipantIsNotInWorkshop:
                if(solution.assignment().is_in_workshop(l, r)) return false;
                break;

            case ParticipantsHaveSameWorkshops:
                if(solution.assignment().workshops_ordered(l) != solution.assignment().workshops_ordered(r)) return false;
                break;

            default: throw std::logic_error("Unknown assignment constraint type " + str(constraint.type()) + ".");
        }
    }

    return true;
}

bool Scoring::satisfies_constraints(Solution const& solution) const
{
    return satisfies_constraints_scheduling(solution) && satisfies_constraints_assignment(solution);
}

int Scoring::evaluate_major(Solution const& solution) const
{
    int m = 0;
    for(int i = 0; i < _inputData->participant_count() * _inputData->slot_count(); i++)
    {
        int p = i / _inputData->slot_count();
        int ws = solution.assignment().workshop_of(p, i % _inputData->slot_count());
        m = std::max(m, _inputData->participant(p).preference(ws));
    }

    return m;
}

float Scoring::evaluate_minor(Solution const& solution) const
{
    if(!is_feasible(solution))
    {
        return INFINITY;
    }

    vector<int> prefCount(_inputData->max_preference() + 1);

    for(int i = 0; i < _inputData->participant_count() * _inputData->slot_count(); i++)
    {
        int p = i / _inputData->slot_count();
        int ws = solution.assignment().workshop_of(p, i % _inputData->slot_count());
        prefCount[_inputData->participant(p).preference(ws)]++;
    }

    float sum = 0;
    for(int pref = 0; pref <= _inputData->max_preference(); pref++)
    {
        sum += prefCount[pref] * std::pow((float)pref, (float)_options.preference_exponent()) / _scaling;
    }

    return sum;
}

Scoring::Scoring(InputData const& inputData, Options const& options)
        : _inputData(&inputData),
        _options(options)
{
    _scaling = std::pow((float)_inputData->max_preference(), (float)_options.preference_exponent());
}

bool Scoring::is_feasible(Solution const& solution) const
{
    vector<int> partCounts(_inputData->workshop_count());
    vector<vector<bool>> isInSlot(_inputData->participant_count(), vector<bool>(_inputData->slot_count()));
    vector<int> slots(_inputData->workshop_count());

    if(!satisfies_constraints(solution))
    {
        return false;
    }

    for(int i = 0; i < _inputData->workshop_count(); i++)
    {
        slots[i] = solution.scheduling().slot_of(i);
    }

    for(int i = 0; i < _inputData->participant_count() * _inputData->slot_count(); i++)
    {
        int p = i / _inputData->slot_count();
        int ws = solution.assignment().workshop_of(p, i % _inputData->slot_count());
        if(isInSlot[p][slots[ws]])
        {
            return false;
        }

        isInSlot[p][slots[ws]] = true;
        partCounts[ws]++;
    }

    for(int i = 0; i < _inputData->workshop_count(); i++)
    {
        if(partCounts[i] < _inputData->workshop(i).min() || partCounts[i] > _inputData->workshop(i).max())
        {
            return false;
        }
    }

    return true;
}

Score Scoring::evaluate(Solution const& solution) const
{
    if(solution.is_invalid())
    {
        return {.major = INFINITY, .minor = INFINITY};
    }

    auto major = (float)evaluate_major(solution);
    auto minor = (float)evaluate_minor(solution);

    if(std::isfinite(major) && std::isfinite(minor))
    {
        return {.major = major, .minor = minor};
    }
    else
    {
        return {.major = INFINITY, .minor = INFINITY};
    }
}
