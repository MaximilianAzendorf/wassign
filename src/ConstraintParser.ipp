#pragma once

#include "ConstraintParser.h"
#include "InputException.h"

namespace x3 = boost::spirit::x3;

using x3::char_;
using x3::uint_;
using x3::lit;
using x3::eoi;
using x3::lexeme;

template<typename Iterator, typename Parser>
bool ConstraintParser::parse_partial(Iterator& begin, Iterator end, Parser parser)
{
    return x3::phrase_parse(begin, end, parser, x3::ascii::space);
}

template<typename Iterator>
ConstraintParser::Accessor ConstraintParser::parse_accessor(string const& constraint, Iterator& begin, Iterator end)
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
ConstraintParser::Relation ConstraintParser::parse_relation(string const& constraint, Iterator& begin, Iterator end)
{
    Relation r{};
    bool negated = false;
    bool canBeNegated = true;

    auto relation =
            lit("=")[pset(r.type, REq)]
            | lit("==")[pset(r.type, REq)]
            | lit("is")[pset(r.type, REq)]
            | lit("!=")[pset(r.type, RNeq)]
            | lit("<>")[pset(r.type, RNeq)]
            | lit(">")[pset(r.type, RGt)]
            | lit("<")[pset(r.type, RLt)]
            | lit(">=")[pset(r.type, RGeq)]
            | lit("<=")[pset(r.type, RLeq)]
            | lit("contains")[pset(r.type, RContains)]
            | lit("subset")[pand(pset(r.type, RSubset), pset(canBeNegated, false))]
            | lit("superset")[pand(pset(r.type, RSuperset), pset(canBeNegated, false))];

    auto negRelation =
            relation
            >> -(lit("not")[pset(negated, true)]);

    if(!parse_partial(begin, end, negRelation) || (negated && !canBeNegated))
    {
        throw InputException("Could not parse relation at position " + str(begin - constraint.begin())
                             + " in constraint \"" + constraint + "\".");
    }

    if(negated)
    {
        r.type = (RelationType)-r.type;
    }

    return r;
}

template<typename Iterator>
ConstraintParser::ConstraintExpr
ConstraintParser::parse_constraint_expr(string const& constraint, Iterator& begin, Iterator end)
{
    return {
            .left = parse_accessor(constraint, begin, end),
            .relation = parse_relation(constraint, begin, end),
            .right = parse_accessor(constraint, begin, end),
    };
}

template<typename Data>
int ConstraintParser::find_name(string const& constraint, string name, vector<Data> const& dataVector)
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