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

#include "ConstraintParser.h"

void ConstraintParser::complete_accessor_types(string const& constraint, InputData const& data,
                                               ConstraintParser::Accessor& accessor)
{
    if(accessor.type != NotSet) return;

    int hits = 0;

    if(find_name(constraint, accessor.name, data.sets()) >= 0)
    {
        accessor.type = Set;
        hits++;
    }

    if(find_name(constraint, accessor.name, data.choices()) >= 0)
    {
        accessor.type = Choice;
        hits++;
    }

    if(find_name(constraint, accessor.name, data.choosers()) >= 0)
    {
        accessor.type = Chooser;
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
        case Set: return find_name(constraint, accessor.name, data.sets());
        case Chooser: return find_name(constraint, accessor.name, data.choosers());
        case Choice:
        {
            int w = find_name(constraint, accessor.name, data.choices());
            int part = accessor.part;
            while(part-- > 0)
            {
                if(!data.choice(w).has_continuation())
                {
                    throw InputException("The given choice doesn't have a part " + str(accessor.part)
                                         + " given in constraint \"" + constraint + "\".");
                }
                w = data.choice(w).continuation();
            }
            return w;
        }

        default: throw std::logic_error("Unexpected accessor type.");
    }
}

vector<Constraint> ConstraintParser::parse(InputData const& data, string text)
{
    // TODO: Add support for choice series constraints.

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
            case key(Choice, Set, REq, Set, NotSet): add(ChoiceIsInSet); break;
            case key(Choice, Set, RNeq, Set, NotSet): add(ChoiceIsNotInSet); break;
            case key(Choice, Set, REq, Choice, Set): add(ChoicesAreInSameSet); break;
            case key(Choice, Set, RNeq, Choice, Set): add(ChoicesAreNotInSameSet); break;
            case key(Set, Choice, RContains, Choice, NotSet): add(SetContainsChoice); break;
            case key(Set, Choice, RNotContains, Choice, NotSet): add(SetNotContainsChoice); break;
            case key(Set, Choice, REq, Set, Choice): add(SetsHaveSameChoices); break;

            case key(Choice, Chooser, REq, Choice, Chooser): add(ChoicesHaveSameChoosers); break;
            case key(Chooser, Choice, RContains, Choice, NotSet): add(ChooserIsInChoice); break;
            case key(Chooser, Choice, RNotContains, Choice, NotSet): add(ChooserIsNotInChoice); break;
            case key(Chooser, Choice, REq, Chooser, Choice): add(ChoosersHaveSameChoices); break;
            case key(Choice, Chooser, RContains, Chooser, NotSet): add(ChoiceContainsChooser); break;
            case key(Choice, Chooser, RNotContains, Chooser, NotSet): add(ChoiceNotContainsChooser); break;

            case key(Set, Size, REq, Integer, NotSet):
            case key(Set, Size, RNeq, Integer, NotSet):
            case key(Set, Size, RGt, Integer, NotSet):
            case key(Set, Size, RLt, Integer, NotSet):
            case key(Set, Size, RGeq, Integer, NotSet):
            case key(Set, Size, RLeq, Integer, NotSet):
            {
                add(SetHasLimitedSize, (SetSizeLimitOp)expr.relation.type);
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
