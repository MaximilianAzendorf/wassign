#pragma once

#include "Types.h"
#include "CriticalSet.h"
#include "InputData.h"

class CriticalSetAnalysis
{
private:
    vector<CriticalSet> _sets;
    const_ptr<InputData> _inputData;
    int preferenceBound;

    void analyze();

public:
    inline static const seconds ProgressInterval = seconds(3);

    CriticalSetAnalysis(const_ptr<InputData> inputData, bool analyze = true);

    [[nodiscard]] vector<CriticalSet> for_preference(int preference) const;

    [[nodiscard]] vector<CriticalSet> const& sets() const;

    [[nodiscard]] int preference_bound() const;

    [[nodiscard]] static CriticalSetAnalysis empty(const_ptr<InputData> inputData);
};


