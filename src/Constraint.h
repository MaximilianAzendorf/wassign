#pragma once

#include "Types.h"

/**
 * ConstraintTypes that are >= this limit are assignment constraints; all others are scheduling constraints.
 */
const int CONSTRAINT_TYPE_DISCRIMINATION_LIMIT = 1u << 16u;

/**
 * All valid constraint types.
 */
enum ConstraintType
{
    Invalid = 0,

    WorkshopIsInSlot = 1,
    WorkshopIsNotInSlot,
    WorkshopsAreInSameSlot,
    WorkshopsAreNotInSameSlot,
    WorkshopsHaveOffset,
    SlotHasLimitedSize,
    SlotContainsWorkshop,  // reduced
    SlotNotContainsWorkshop,  // reduced
    SlotsHaveSameWorkshops,  // reduced

    WorkshopsHaveSameParticipants = CONSTRAINT_TYPE_DISCRIMINATION_LIMIT,
    ParticipantIsInWorkshop,
    ParticipantIsNotInWorkshop,
    ParticipantsHaveSameWorkshops,
    WorkshopContainsParticipant,  // reduced
    WorkshopNotContainsParticipant,  // reduced
};

/**
 * Relation operators supported by the SlotHasLimitedSize constraint.
 */
enum SlotSizeLimitOp
{
    Eq = 1,
    Gt = 2,
    Geq = 3,
    Neq = -Eq,
    Leq = -Gt,
    Lt = -Geq,
};

/**
 * Represents a single constraint (constraining either scheduling or assignment solutions).
 */
class Constraint
{
private:
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
     * Returns the negation of this constraint.
     */
    Constraint negation();

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

    bool operator == (Constraint const& other) const;
    bool operator != (Constraint const& other) const;
};

size_t hash_value(Constraint const& c);