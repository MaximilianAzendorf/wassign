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

#include "ConstraintBuilder.h"
#include "ConstraintExpression.h"

int ConstraintBuilder::resolve_accessor(InputData const& data, ConstraintExpressionAccessor const& accessor)
{
    switch(accessor.type)
    {
        case Slot: return find_name(accessor.name, data.slots());
        case Chooser: return find_name(accessor.name, data.choosers());
        case Choice:
        {
            int w = find_name(accessor.name, data.choices());
            int part = accessor.part;
            while(part-- > 0)
            {
                if(!data.choice(w).has_continuation())
                {
                    throw InputException("The given choice doesn't have a part " + str(accessor.part) + ".");
                }
                w = data.choice(w).continuation_value();
            }
            return w;
        }
        case Integer: return std::stoi(accessor.name);

        default: throw std::logic_error("Unexpected accessor type.");
    }
}

vector<Constraint> ConstraintBuilder::build(InputData const& data, ConstraintExpression expression)
{
    // TODO: Add support for choice series constraints.

    vector<Constraint> res;
    auto add = [&](ConstraintType type, int extra = 0)
    {
        res.push_back(Constraint(type,
                                 resolve_accessor(data, expression.left),
                                 resolve_accessor(data, expression.right),
                                 extra));
    };

    // Try it two times, the second time we flip left and right of expr.
    //
    for(int i = 0; i < 2; i++)
    {
        switch (key(expression.left.type, expression.left.subType, expression.relation.type, expression.right.type, expression.right.subType))
        {
            case key(Choice, Slot, REq, Slot, NotSet): add(ChoiceIsInSlot); break;
            case key(Choice, Slot, RNeq, Slot, NotSet): add(ChoiceIsNotInSlot); break;
            case key(Choice, Slot, REq, Choice, Slot): add(ChoicesAreInSameSlot); break;
            case key(Choice, Slot, RNeq, Choice, Slot): add(ChoicesAreNotInSameSlot); break;
            case key(Slot, Choice, RContains, Choice, NotSet): add(SlotContainsChoice); break;
            case key(Slot, Choice, RNotContains, Choice, NotSet): add(SlotNotContainsChoice); break;
            case key(Slot, Choice, REq, Slot, Choice): add(SlotsHaveSameChoices); break;

            case key(Choice, Chooser, REq, Choice, Chooser): add(ChoicesHaveSameChoosers); break;
            case key(Chooser, Choice, RContains, Choice, NotSet): add(ChooserIsInChoice); break;
            case key(Chooser, Choice, RNotContains, Choice, NotSet): add(ChooserIsNotInChoice); break;
            case key(Chooser, Choice, REq, Chooser, Choice): add(ChoosersHaveSameChoices); break;
            case key(Choice, Chooser, RContains, Chooser, NotSet): add(ChoiceContainsChooser); break;
            case key(Choice, Chooser, RNotContains, Chooser, NotSet): add(ChoiceNotContainsChooser); break;

            case key(Slot, Size, REq, Integer, NotSet):
            case key(Slot, Size, RNeq, Integer, NotSet):
            case key(Slot, Size, RGt, Integer, NotSet):
            case key(Slot, Size, RLt, Integer, NotSet):
            case key(Slot, Size, RGeq, Integer, NotSet):
            case key(Slot, Size, RLeq, Integer, NotSet):
            {
                add(SlotHasLimitedSize, (SlotSizeLimitOp)expression.relation.type);
                break;
            }

            default:
            {
                if (i == 0)
                {
                    expression = {.left = expression.right, .relation = expression.relation, .right = expression.left};
                    continue;
                }
                throw InputException("Unsupported constraint.");
            }
        }

        break;
    }

    return res;
}
