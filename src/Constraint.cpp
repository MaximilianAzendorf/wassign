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
#include <magic_enum.hpp>

using namespace magic_enum;

auto Constraint::_typeNames = map<ConstraintType, string>();

Constraint::Constraint(ConstraintType type, int left, int right, int extra)
        : _type(type), _left(left), _right(right), _extra(extra)
{
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

string Constraint::type_name(ConstraintType type)
{
    if(_typeNames.empty())
    {
        for(auto entry : enum_entries<ConstraintType>())
        {
            _typeNames[entry.first] = string{entry.second};
        }
    }

    return _typeNames.at(type);
}


