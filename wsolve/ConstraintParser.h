#pragma once

#include "Types.h"
#include "Util.h"
#include "SpiritUtil.h"
#include "Constraint.h"
#include "InputData.h"
#include "FuzzyMatch.h"

#include <boost/algorithm/string.hpp>
#include <boost/spirit/home/x3.hpp>

namespace x3 = boost::spirit::x3;

using x3::char_;
using x3::uint_;
using x3::lit;
using x3::eoi;
using x3::lexeme;

class ConstraintParser
{
private:
    ConstraintParser() = default;

    enum AccessorType
    {
        NotSet,

        Participant,
        Workshop,
        Slot,
        Size,
        ParticipantsOfWorkshop,
        WorkshopsOfParticipant,
        WorkshopsOfSlot,
        SlotOfWorkshop,
        Integer,
    };

    enum RelationType
    {
        REq = Eq,
        RNeq = Neq,
        RGt = Gt,
        RLt = Lt,
        RGeq = Geq,
        RLeq = Leq,

        RContains = 1 << 16,
        RNotContains,
        RSubset,
        RSuperset,
    };

    struct Accessor
    {
        AccessorType type;
        AccessorType subType;
        string name;
        int part;
    };

    struct Relation
    {
        RelationType type;
    };

    struct ConstraintExpr
    {
        Accessor left;
        Relation relation{};
        Accessor right;
    };

    static constexpr unsigned key(AccessorType l, AccessorType lsub, RelationType rel, AccessorType r, AccessorType rsub)
    {
        return ((unsigned)l << 24U)
        | ((unsigned)lsub << 18U)
        | ((unsigned)rel << 12U)
        | ((unsigned)r << 6U)
        | ((unsigned)rsub << 0U);
    }

    template<typename Iterator, typename Parser>
    static bool parse_partial(Iterator& begin, Iterator end, Parser parser)
    {
        return x3::phrase_parse(begin, end, parser, x3::ascii::space);
    }

    template<typename Iterator>
    static Accessor parse_accessor(string const& constraint, Iterator& begin, Iterator end)
    {
        Accessor a{};

        auto baseAccessor =
                -(lit("event")[pset(a.type, Workshop)]
                    | lit("slot")[pset(a.type, Slot)]
                    | lit("person")[pset(a.type, Participant)]
                    )
                >> lit("[")
                >> lexeme[ (*(char_ - ']'))[pset(a.name)] ]
                >> lit("]")
                >> -(lit("part") >> uint_[pand(pset(a.part), pset(a.type, Workshop))]);

        auto accessor =
                -((lit("events")[pset(a.subType, Workshop)]
                    | lit("slot")[pset(a.subType, Slot)]
                    | lit("participants")[pset(a.subType, Participant)]
                    | lit("size")[pset(a.subType, Size)]
                    )
                    >> lit("of"))
                >> baseAccessor;

        auto accessorOrLiteral =
                accessor
                | (*(char_("0-9")))[pand(pset(a.name), pset(a.type, Integer))];

        if(!parse_partial(begin, end, accessorOrLiteral))
        {
            throw InputException("Could not parse accessor at position " + str(begin - constraint.begin())
                + " in constraint \"" + constraint + "\".");
        }

        return a;
    }

    template<typename Iterator>
    static Relation parse_relation(string const& constraint, Iterator& begin, Iterator end)
    {
        Relation r{};

        auto relation =
                lit("=")[pset(r.type, REq)]
                | lit("==")[pset(r.type, REq)]
                | lit("is")[pset(r.type, REq)]
                | lit("!=")[pset(r.type, RNeq)]
                | lit("is not")[pset(r.type, RNeq)]
                | lit("<>")[pset(r.type, RNeq)]
                | lit(">")[pset(r.type, RGt)]
                | lit("<")[pset(r.type, RLt)]
                | lit(">=")[pset(r.type, RGeq)]
                | lit("<=")[pset(r.type, RLeq)]
                | lit("contains")[pset(r.type, RContains)]
                | lit("contains not")[pset(r.type, RNotContains)]
                | lit("subset of")[pset(r.type, RSubset)]
                | lit("superset of")[pset(r.type, RSuperset)];

        if(!parse_partial(begin, end, relation))
        {
            throw InputException("Could not parse relation at position " + str(begin - constraint.begin())
                + " in constraint \"" + constraint + "\".");
        }

        return r;
    }

    template<typename Iterator>
    static ConstraintExpr parse_constraint_expr(string const& constraint, Iterator& begin, Iterator end)
    {
        return {
            .left = parse_accessor(constraint, begin, end),
            .relation = parse_relation(constraint, begin, end),
            .right = parse_accessor(constraint, begin, end),
        };
    }

    template<typename Data>
    static int find_name(string const& constraint, string name, vector<Data> const& dataVector)
    {
        vector<string> names;
        for(int i = 0; i < dataVector.size(); i++)
        {
            names.push_back(dataVector[i].name());
        }

        auto res = FuzzyMatch::find(name, names);

        if(res.size() > 1)
        {
            throw InputException("The name \"" + name + "\" is ambiguous in constraint \""
                                 + constraint + "\".");
        }
        else if(res.size() == 0)
        {
            return -1;
        }
        else
        {
            return res.front();
        }
    }

    static void complete_accessor_types(string const& constraint, InputData const& data, Accessor& accessor)
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

    static void complete_accessor_types(string const& constraint, InputData const& data, ConstraintExpr& expr)
    {
        complete_accessor_types(constraint, data, expr.left);
        complete_accessor_types(constraint, data, expr.right);
    }

    static int resolve_accessor(InputData const& data, string const& constraint, Accessor const& accessor)
    {
        switch(accessor.type)
        {
            case Slot: return find_name(constraint, accessor.name, data.slots());
            case Workshop: return find_name(constraint, accessor.name, data.workshops());
            case Participant: return find_name(constraint, accessor.name, data.participants());

            default: throw std::logic_error("Unexpected accessor type.");
        }
    }

public:
    static vector<Constraint> parse(InputData const& data, string text)
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
};


