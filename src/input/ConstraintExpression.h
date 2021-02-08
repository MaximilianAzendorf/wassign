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
#pragma once

#include "../Constraint.h"

enum AccessorType
{
    NotSet,

    Chooser,
    Choice,
    Slot,
    Size,
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
    RNotContains = -RContains,

    // TODO: Implement support for sub- and superset relations
    RSubset,
    RSuperset,
};

struct ConstraintExpressionAccessor
{
    AccessorType type;
    AccessorType subType;
    string name;
    int part;
};

struct ConstraintExpressionRelation
{
    RelationType type;
};

struct ConstraintExpression
{
    ConstraintExpressionAccessor left;
    ConstraintExpressionRelation relation{};
    ConstraintExpressionAccessor right;
};