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
    Neq = -1,
    Gt = 2,
    Leq = -2,
    Geq = 3,
    Lt = -3
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

    Constraint(ConstraintType type, int left, int right, int extra = 0)
            : _type(type), _left(left), _right(right), _extra(extra)
    {
    }

    Constraint negation()
    {
        Constraint neg = *this;
        switch(_type)
        {
            case WorkshopIsInSlot: neg._type = WorkshopIsNotInSlot; break;
            case WorkshopIsNotInSlot: neg._type = WorkshopIsInSlot; break;
            case WorkshopsAreInSameSlot: neg._type = WorkshopsAreNotInSameSlot; break;
            case WorkshopsAreNotInSameSlot: neg._type = WorkshopsAreInSameSlot; break;
            case SlotHasLimitedSize: neg._extra = -neg._extra; break;
            case SlotContainsWorkshop: neg._type = SlotNotContainsWorkshop; break;
            case SlotNotContainsWorkshop: neg._type = SlotContainsWorkshop; break;

            case ParticipantIsInWorkshop: neg._type = ParticipantIsNotInWorkshop; break;
            case ParticipantIsNotInWorkshop: neg._type = ParticipantIsInWorkshop; break;
            case WorkshopContainsParticipant: neg._type = WorkshopNotContainsParticipant; break;
            case WorkshopNotContainsParticipant: neg._type = WorkshopContainsParticipant; break;

            // Constraints with no valid negation
            case Invalid:
            case WorkshopsHaveOffset:
            case SlotsHaveSameWorkshops:
            case WorkshopsHaveSameParticipants:
            case ParticipantsHaveSameWorkshops:
                neg._type = Invalid;
                break;
        }

        return neg;
    }

    [[nodiscard]] ConstraintType type() const
    {
        return _type;
    }

    [[nodiscard]] int left() const
    {
        return _left;
    }

    [[nodiscard]] int right() const
    {
        return _right;
    }

    [[nodiscard]] int extra() const
    {
        return _extra;
    }

    [[nodiscard]] bool is_valid() const
    {
        return _type != Invalid;
    }

    [[nodiscard]] bool is_scheduling_constraint() const
    {
        return _type < CONSTRAINT_TYPE_DISCRIMINATION_LIMIT && _type != Invalid;
    }

    [[nodiscard]] bool is_assignment_constraint() const
    {
        return _type >= CONSTRAINT_TYPE_DISCRIMINATION_LIMIT && _type != Invalid;
    }

    bool operator ==(Constraint const& other) const
    {
        return _type == other._type && _left == other._left && _right == other._right && _extra == other._extra;
    }
};

size_t hash_value(Constraint const& c)
{
    int hash = 0;
    hash = hash * 101 + c.left();
    hash = hash * 101 + c.right();
    hash = hash * 101 + c.extra();
    hash = hash * 101 + c.type();
    return hash;
}