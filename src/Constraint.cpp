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

#include "Constraint.h"

Constraint::Constraint(ConstraintType type, int left, int right, int extra)
        : _type(type), _left(left), _right(right), _extra(extra)
{
}

Constraint Constraint::negation()
{
    Constraint neg = *this;
    switch(_type)
    {
        case ChoiceIsInSet: neg._type = ChoiceIsNotInSet; break;
        case ChoiceIsNotInSet: neg._type = ChoiceIsInSet; break;
        case ChoicesAreInSameSet: neg._type = ChoicesAreNotInSameSet; break;
        case ChoicesAreNotInSameSet: neg._type = ChoicesAreInSameSet; break;
        case SetHasLimitedSize: neg._extra = -neg._extra; break;
        case SetContainsChoice: neg._type = SetNotContainsChoice; break;
        case SetNotContainsChoice: neg._type = SetContainsChoice; break;

        case ChooserIsInChoice: neg._type = ChooserIsNotInChoice; break;
        case ChooserIsNotInChoice: neg._type = ChooserIsInChoice; break;
        case ChoiceContainsChooser: neg._type = ChoiceNotContainsChooser; break;
        case ChoiceNotContainsChooser: neg._type = ChoiceContainsChooser; break;

            // Constraints with no valid negation
        case Invalid:
        case ChoicesHaveOffset:
        case SetsHaveSameChoices:
        case ChoicesHaveSameChoosers:
        case ChoosersHaveSameChoices:
            neg._type = Invalid;
            break;
    }

    return neg;
}

ConstraintType Constraint::type() const
{
    return _type;
}

int Constraint::left() const
{
    return _left;
}

int Constraint::right() const
{
    return _right;
}

int Constraint::extra() const
{
    return _extra;
}

bool Constraint::is_valid() const
{
    return _type != Invalid;
}

bool Constraint::is_scheduling_constraint() const
{
    return _type < CONSTRAINT_TYPE_DISCRIMINATION_LIMIT && _type != Invalid;
}

bool Constraint::is_assignment_constraint() const
{
    return _type >= CONSTRAINT_TYPE_DISCRIMINATION_LIMIT && _type != Invalid;
}

int Constraint::get_hash() const
{
    int hash = 0;
    hash = hash * 101 + _left;
    hash = hash * 101 + _right;
    hash = hash * 101 + _extra;
    hash = hash * 101 + _type;
    return hash;
}

bool Constraint::operator==(Constraint const& other) const
{
    return _type == other._type && _left == other._left && _right == other._right && _extra == other._extra;
}

bool Constraint::operator!=(Constraint const& other) const
{
    return !(*this == other);
}


