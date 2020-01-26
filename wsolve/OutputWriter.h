#pragma once

#include "Solution.h"

class OutputWriter
{
private:
    OutputWriter() = default;

public:
    static string write_scheduling_solution(Solution const& solution)
    {
        using std::endl;
        std::stringstream str;

        str << R"("Workshop", "Slot")" << endl;

        for(int w = 0; w < solution.input_data().workshop_count(); w++)
        {
            int s = solution.scheduling().slot_of(w);
            string wName = solution.input_data().workshop(w).name();
            if(wName.rfind(InputData::HiddenWorkshopPrefix, 0) == 0)
            {
                continue;
            }

            string sName = solution.input_data().slot(s).name();
            if(sName.rfind(InputData::NotScheduledSlotPrefix, 0) == 0)
            {
                sName = "not scheduled";
            }

            str << '"' << wName << '"' << ", " << '"' << sName << '"' << endl;
        }

        return str.str();
    }

    static string write_assignment_solution(Solution const& solution)
    {
        using std::endl;
        std::stringstream str;

        str << "\"Workshop\"";

        for(int s = 0; s < solution.input_data().slot_count(); s++)
        {
            if(solution.input_data().slot(s).name().rfind(InputData::NotScheduledSlotPrefix, 0) == 0)
            {
                continue;
            }

            str << ", \"" << solution.input_data().slot(s).name() << '"';
        }

        for(int p = 0; p < solution.input_data().participant_count(); p++)
        {
            vector<int> workshops(solution.input_data().slot_count());
            for(int s = 0; s < solution.input_data().slot_count(); s++)
            {
                int ws = solution.assignment().workshop_of(p, s);
                workshops[solution.scheduling().slot_of(ws)] = ws;
            }

            str << endl << '"' << solution.input_data().participant(p).name() << '"';

            for(int s = 0; s < solution.input_data().slot_count(); s++)
            {
                if(solution.input_data().slot(s).name().rfind(InputData::NotScheduledSlotPrefix, 0) == 0)
                {
                    continue;
                }

                str << ", " << solution.input_data().workshop(workshops[s]).name();
            }
        }

        return str.str();
    }
};


