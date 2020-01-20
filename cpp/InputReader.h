#pragma once

#include <utility>

#include "Types.h"
#include "InputData.h"
#include "Status.h"
#include "Util.h"

class InputReader
{
private:
    inline static const regex workshopRegex = regex(R"(^\(event\)\s+((?<name>[a-zA-Z0-9_\- ]+)\s*:\s*(?:(?<conductor>[^,\s][^,]*)\s*,\s*)?(?<min>[0-9]+)\s*\-\s*(?<max>[0-9]+)\s*)*(?:\s*\[(?:(?:(?<parts>[1-9][0-9]*) parts|(?<optional>optional))(?:,|(?=\]))\s*)+\])?$)");
    inline static const regex slotRegex = regex(R"(^\(slot\)\s+(?<name>[a-zA-Z0-9_\- ]+))");
    inline static const regex participantRegex = regex(R"(^\(person\)\s+(?<name>[a-zA-Z0-9_\- ]+)\s*:(?:\s*(?<pref>[0-9]+))+)");
    inline static const regex constraintRegex = regex(R"(^\(constraint\)\s+(?<constraint>.+))");

    InputReader() = default;

    struct PreWorkshop
    {
        string name;
        string cond;
        int min, max, parts;
        bool optional;
    };

    static void parse_line(string const& line, MutableInputData& inputData, vector<PreWorkshop>& preWorkshops)
    {
        if(line.front() == '#')
        {
            return;
        }

        // TODO: Implement.
    }

    static InputData parse(string const& input)
    {
        std::stringstream inputStream(input);
        string line;

        MutableInputData inputData;
        vector<PreWorkshop> preWorkshops;

        while(std::getline(inputStream, line, '\n'))
        {
            parse_line(line, inputData, preWorkshops);
        }
    }

public:
    static InputData read_input(string const& input)
    {
        Status::info("Begin parsing input.");

        InputData res = parse(input);
        int slotCount = res.slot_count();
        int wsCount = res.workshop_count();
        int partCount = res.participant_count();

        for(int i = 0; i < res.slot_count(); i++)
        {
            if(res.slot(i).name().rfind(InputData::GeneratedPrefix, 0) == 0)
            {
                slotCount--;
            }
        }

        for(int i = 0; i < res.workshop_count(); i++)
        {
            if(res.workshop(i).name().rfind(InputData::GeneratedPrefix, 0) == 0)
            {
                wsCount--;
            }
        }

        for(int i = 0; i < res.participant_count(); i++)
        {
            if(res.participant(i).name().rfind(InputData::GeneratedPrefix, 0) == 0)
            {
                partCount--;
            }
        }

        Status::info("Read "
        + str(slotCount) + " slot(s), "
        + str(wsCount) + "event(s), "
        + str(partCount) + " participant(s) and "
        + str(res.scheduling_constraints().size()) + "+" + str(res.assignment_constraints().size()) + " constraints.");

        return res;
    }
};


