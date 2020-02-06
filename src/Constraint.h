#pragma once

#include "Types.h"

const int CONSTRAINT_TYPE_DISCRIMINATION_LIMIT = 1u << 16u;

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

enum SlotSizeLimitOp
{
    Eq = 1,
    Gt = 2,
    Geq = 3,
    Neq = -Eq,
    Leq = -Gt,
    Lt = -Geq,
};

class Constraint
{
private:
    ConstraintType _type;
    int _left;
    int _right;
    int _extra;

public:
    Constraint() = default;

    Constraint(ConstraintType type, int left, int right, int extra = 0);

    Constraint negation();

    [[nodiscard]] ConstraintType type() const;

    [[nodiscard]] int left() const;

    [[nodiscard]] int right() const;

    [[nodiscard]] int extra() const;

    [[nodiscard]] bool is_valid() const;

    [[nodiscard]] bool is_scheduling_constraint() const;

    [[nodiscard]] bool is_assignment_constraint() const;

    bool operator ==(Constraint const& other) const;
};

size_t hash_value(Constraint const& c);