#include "ConstraintParser.h"

void ConstraintParser::complete_accessor_types(string const& constraint, InputData const& data,
                                               ConstraintParser::Accessor& accessor)
{
    if(accessor.type != NotSet) return;

    int hits = 0;

    if(find_name(constraint, accessor.name, data.slots()) >= 0)
    {
        accessor.type = Slot;
        hits++;
    }

    if(find_name(constraint, accessor.name, data.workshops()) >= 0)
    {
        accessor.type = Workshop;
        hits++;
    }

    if(find_name(constraint, accessor.name, data.participants()) >= 0)
    {
        accessor.type = Participant;
        hits++;
    }

    if(hits == 0) throw InputException("Could not find anything with name \""
                                       + accessor.name + "\" in constraint \"" + constraint + "\".");

    if(hits > 1) throw InputException("The name \"" + accessor.name + "\" is ambiguous in constraint \""
                                      + constraint + "\".");
}

void ConstraintParser::complete_accessor_types(string const& constraint, InputData const& data,
                                               ConstraintParser::ConstraintExpr& expr)
{
    complete_accessor_types(constraint, data, expr.left);
    complete_accessor_types(constraint, data, expr.right);
}

int ConstraintParser::resolve_accessor(InputData const& data, string const& constraint,
                                       ConstraintParser::Accessor const& accessor)
{
    switch(accessor.type)
    {
        case Slot: return find_name(constraint, accessor.name, data.slots());
        case Workshop: return find_name(constraint, accessor.name, data.workshops());
        case Participant: return find_name(constraint, accessor.name, data.participants());

        default: throw std::logic_error("Unexpected accessor type.");
    }
}

vector<Constraint> ConstraintParser::parse(InputData const& data, string text)
{
    // TODO: Add support for event series constraints.

    auto begin = text.begin();
    ConstraintExpr expr = parse_constraint_expr(text, begin, text.end());

    complete_accessor_types(text, data, expr);

    if(begin != text.end())
    {
        throw InputException("Could not parse remaining text at position " + str(begin - text.begin())
                             + " in constraint \"" + text + "\".");
    }

    vector<Constraint> res;
    auto add = [&](ConstraintType type, int extra = 0)
    {
        res.push_back(Constraint(type,
                                 resolve_accessor(data, text, expr.left),
                                 resolve_accessor(data, text, expr.right),
                                 extra));
    };

    // Try it two times, the second time we flip left and right of expr.
    //
    for(int i = 0; i < 2; i++)
    {
        switch (key(expr.left.type, expr.left.subType, expr.relation.type, expr.right.type, expr.right.subType))
        {
            case key(Workshop, Slot, REq, Slot, NotSet): add(WorkshopIsInSlot); break;
            case key(Workshop, Slot, RNeq, Slot, NotSet): add(WorkshopIsNotInSlot); break;
            case key(Workshop, Slot, REq, Workshop, Slot): add(WorkshopsAreInSameSlot); break;
            case key(Workshop, Slot, RNeq, Workshop, Slot): add(WorkshopsAreNotInSameSlot); break;
            case key(Slot, Workshop, RContains, Workshop, NotSet): add(SlotContainsWorkshop); break;
            case key(Slot, Workshop, RNotContains, Workshop, NotSet): add(SlotNotContainsWorkshop); break;
            case key(Slot, Workshop, REq, Slot, Workshop): add(SlotsHaveSameWorkshops); break;

            case key(Workshop, Participant, REq, Workshop, Participant): add(WorkshopsHaveSameParticipants); break;
            case key(Participant, Workshop, RContains, Workshop, NotSet): add(ParticipantIsInWorkshop); break;
            case key(Participant, Workshop, RNotContains, Workshop, NotSet): add(ParticipantIsNotInWorkshop); break;
            case key(Participant, Workshop, REq, Participant, Workshop): add(ParticipantsHaveSameWorkshops); break;
            case key(Workshop, Participant, RContains, Participant, NotSet): add(WorkshopContainsParticipant); break;
            case key(Workshop, Participant, RNotContains, Participant, NotSet): add(WorkshopNotContainsParticipant); break;

            case key(Slot, Size, REq, Integer, NotSet):
            case key(Slot, Size, RNeq, Integer, NotSet):
            case key(Slot, Size, RGt, Integer, NotSet):
            case key(Slot, Size, RLt, Integer, NotSet):
            case key(Slot, Size, RGeq, Integer, NotSet):
            case key(Slot, Size, RLeq, Integer, NotSet):
            {
                add(SlotHasLimitedSize, (SlotSizeLimitOp)expr.relation.type);
                break;
            }

            default:
            {
                if (i == 0)
                {
                    expr = {.left = expr.right, .relation = expr.relation, .right = expr.left};
                    continue;
                }
                throw InputException("Constraints like \"" + text + "\" are not supported.");
            }
        }

        break;
    }

    return res;
}
