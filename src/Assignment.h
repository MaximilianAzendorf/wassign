#pragma once

#include "Types.h"
#include "InputData.h"

/**
 * Represents a single assignment solution for the given input data.
 */
class Assignment
{
private:
    const_ptr<InputData> _inputData;
    vector<vector<int>> _data;

public:
    /**
     * Constructor.
     *
     * @param inputData The input data this instance is an assignment solution for.
     * @param data A vector of vectors so that participant x is assigned to workshop data[x][y] at slot y.
     */
    Assignment(const_ptr<InputData> inputData, vector<vector<int>> data);

    /**
     * Returns the workshop the given participant is assigned to at the given slot.
     */
    [[nodiscard]] int workshop_of(int participant, int slot) const;

    /**
     *
     * Returns an ordered vector of all participants that are assigned to the given workshop.
     */
    [[nodiscard]] vector<int> participants_ordered(int workshop) const;

    /**
     * Returns an ordered vector of all workshops to which the given participant is assigned.
     */
    [[nodiscard]] vector<int> workshops_ordered(int participant) const;

    /**
     * Returns true if the given participant is assigned to the given workshop.
     */
    [[nodiscard]] bool is_in_workshop(int participant, int workshop) const;

    /**
     * Returns the maximum preference that any participant has for any of their assigned to workshop.
     */
    [[nodiscard]] int max_used_preference() const;

    /**
     * Returns the input data this instance is an assignment solution for.
     */
    [[nodiscard]] InputData const& input_data() const;

    bool operator == (Assignment const& other) const;
    bool operator != (Assignment const& other) const;
};


