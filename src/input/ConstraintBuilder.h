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

#include "FuzzyMatch.h"
#include "../Types.h"
#include "../Util.h"
#include "../Constraint.h"
#include "../InputData.h"
#include "ConstraintExpression.h"

class ConstraintBuilder
{
private:
    ConstraintBuilder() = default;

    static constexpr unsigned key(AccessorType l, AccessorType lsub, RelationType rel, AccessorType r, AccessorType rsub)
    {
        return ((unsigned)l << 24U)
               | ((unsigned)lsub << 18U)
               | ((unsigned)rel << 12U)
               | ((unsigned)r << 6U)
               | ((unsigned)rsub << 0U);
    }

    template<typename Data>
    static int find_name(string const& name, vector<Data> const& dataVector);

    static int resolve_accessor(InputData const& data, ConstraintExpressionAccessor const& accessor);

public:
    static vector<Constraint> build(InputData const& data, ConstraintExpression expression);
};

#include "ConstraintBuilder.ipp"