#pragma once

#include "Types.h"
#include "InputData.h"

class Assignment
{
private:
    InputData const* _inputData;
    vector<vector<int>> _data;

public:
    Assignment(InputData const& inputData, vector<vector<int>> data);

    [[nodiscard]] int workshop_of(int participant, int slot) const;

    [[nodiscard]] vector<int> participants_ordered(int workshop) const;

    [[nodiscard]] vector<int> workshops_ordered(int participant) const;

    [[nodiscard]] bool is_in_workshop(int p, int w) const;

    [[nodiscard]] int max_used_preference() const;

    [[nodiscard]] InputData const& input_data() const;

    bool operator == (Assignment const& other) const;

    bool operator != (Assignment const& other) const;
};


