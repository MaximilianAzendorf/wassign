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

#include "Types.h"

/**
 * ConstraintTypes that are >= this limit are assignment constraints; all others are scheduling constraints.
 */
const int CONSTRAINT_TYPE_DISCRIMINATION_LIMIT = 1u << 16u;

/**
 * All valid constraint types. Some types get reduced to other types in Constraints::reduce_and_optimize.
 */
enum ConstraintType
{
    Invalid = 0,

    ChoiceIsInSlot = 1,
    ChoiceIsNotInSlot,
    ChoicesAreInSameSlot,
    ChoicesAreNotInSameSlot,
    ChoicesHaveOffset,
    SlotHasLimitedSize,
    SlotContainsChoice,  // reduced
    SlotNotContainsChoice,  // reduced
    SlotsHaveSameChoices,  // reduced

    ChoosersOfChoicesRelation = CONSTRAINT_TYPE_DISCRIMINATION_LIMIT,
    ChooserIsInChoice,
    ChooserIsNotInChoice,
    ChoicesOfChoosersRelation,
    ChoiceContainsChooser,  // reduced
    ChoiceNotContainsChooser,  // reduced
};

/**
 * Relation operators supported by various constraint.
 */
enum RelationOp
{
    Eq = 1,
    Gt = 2,
    Geq = 3,
    Neq = -Eq,
    Leq = -Gt,
    Lt = -Geq,
    Subset,
    Superset,
};

/**
 * Represents a single constraint (constraining either scheduling or assignment solutions).
 */
class Constraint
{
private:
    static map<ConstraintType, string> _typeNames;

    ConstraintType _type;
    int _left;
    int _right;
    int _extra;

public:
    /**
     * Default constructor.
     */
    Constraint() = default;

    /**
     * Constructor.
     *
     * @param type The type of the constraint.
     * @param left The left operand of the constraint.
     * @param right The right operand of the constraint.
     * @param extra Extra data (may be irrelevant depending on the type).
     */
    Constraint(ConstraintType type, int left, int right, int extra = 0);

    /**
     * The constraint type.
     */
    [[nodiscard]] ConstraintType type() const;

    /**
     * The left operand of this constraint.
     */
    [[nodiscard]] int left() const;

    /**
     * The right operand of this constraint.
     */
    [[nodiscard]] int right() const;

    /**
     * The extra value of this constraint (may ba irrelevant depending on the type of this constraint).
     */
    [[nodiscard]] int extra() const;

    /**
     * Return false if the type is ConstraintType::Invalid.
     */
    [[nodiscard]] bool is_valid() const;

    /**
     * Return true if this constraint is a scheduling constraint.
     */
    [[nodiscard]] bool is_scheduling_constraint() const;

    /**
     * Return true if this constraint is an assignment constraint.
     */
    [[nodiscard]] bool is_assignment_constraint() const;

    [[nodiscard]] int get_hash() const;

    bool operator == (Constraint const& other) const;
    bool operator != (Constraint const& other) const;

    /**
     * Returns the name of the given constraint type
     */
     static string type_name(ConstraintType type);
};

namespace std
{
    template <>
    struct hash<Constraint>
    {
        size_t operator()(Constraint const& c) const
        {
            return c.get_hash();
        }
    };
}