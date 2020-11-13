#pragma once

#include "Types.h"
#include "InputData.h"

class Scheduling
{
private:
    const_ptr<InputData> _inputData;
    vector<int> _data;

public:
    explicit Scheduling(const_ptr<InputData> inputData);

    Scheduling(const_ptr<InputData> inputData, vector<int> data);

    [[nodiscard]] bool is_feasible() const;

    [[nodiscard]] int slot_of(int workshop) const;

    [[nodiscard]] InputData const& input_data() const;

    [[nodiscard]] vector<int> const& raw_data() const;

    [[nodiscard]] int get_hash() const;

    bool operator == (Scheduling const& other) const;

    bool operator != (Scheduling const& other) const;
};

size_t hash_value(Scheduling const& scheduling);


