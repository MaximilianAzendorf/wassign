#pragma once

#include "Types.h"
#include "Util.h"
#include "SpiritUtil.h"
#include "Constraint.h"
#include "InputData.h"
#include "FuzzyMatch.h"

#include <boost/algorithm/string.hpp>
#include <boost/spirit/home/x3.hpp>

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

        RContains = 1U << 16,
        RSubset,
        RSuperset,

        RNotContains = -RContains,
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
    static bool parse_partial(Iterator& begin, Iterator end, Parser parser);

    template<typename Iterator>
    static Accessor parse_accessor(string const& constraint, Iterator& begin, Iterator end);

    template<typename Iterator>
    static Relation parse_relation(string const& constraint, Iterator& begin, Iterator end);

    template<typename Iterator>
    static ConstraintExpr parse_constraint_expr(string const& constraint, Iterator& begin, Iterator end);

    template<typename Data>
    static int find_name(string const& constraint, string name, vector<Data> const& dataVector);

    static void complete_accessor_types(string const& constraint, InputData const& data, Accessor& accessor);

    static void complete_accessor_types(string const& constraint, InputData const& data, ConstraintExpr& expr);

    static int resolve_accessor(InputData const& data, string const& constraint, Accessor const& accessor);

public:
    static vector<Constraint> parse(InputData const& data, string text);
};

#include "ConstraintParser.ipp"