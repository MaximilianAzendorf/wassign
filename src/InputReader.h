#pragma once

#include <utility>

#include "Types.h"
#include "InputData.h"
#include "Status.h"
#include "Util.h"
#include "SpiritUtil.h"

#include <boost/algorithm/string.hpp>
#include <boost/spirit/home/x3.hpp>

namespace x3 = boost::spirit::x3;

using x3::char_;
using x3::uint_;
using x3::lit;
using x3::eoi;
using x3::lexeme;

class InputReader
{
private:
    InputReader() = default;

    struct PreWorkshop
    {
        string name;
        vector<string> cond;
        int min, max, parts;
        bool optional;
    };

    template<typename Parser>
    static bool parse(string const& line, Parser parser)
    {
        return x3::phrase_parse(line.begin(), line.end(), parser >> eoi, x3::ascii::space);
    }

    static bool parse_line_slot(string const& line, MutableInputData& inputData)
    {
        string name;

        auto syntax = lit("(slot)") >> lexeme[ (*char_)[pset(name)] ];

        if(!parse(line, syntax)) return false;
        inputData.slots.push_back(SlotData(name));
        return true;
    }

    static bool parse_line_workshop(string const& line, vector<PreWorkshop>& preWorkshops)
    {
        PreWorkshop ws{};

        auto name = *(char_ - char_("+:,[]"));
        auto syntax =
                lit("(event)")
                >> lexeme[ name[pset(ws.name)] ] >> ':'
                >> -(lexeme[ name[padd(ws.cond)] ] % '+' >> ',')
                >> uint_[pset(ws.min)] >> '-' >> uint_[pset(ws.max)]
                >> -('['
                    >> ((uint_[pset(ws.parts)] >> "parts") | lit("optional")[pset(ws.optional, true)]) % ','
                    >> ']');

        if(!parse(line, syntax)) return false;
        preWorkshops.push_back(ws);
        return true;
    }

    static bool parse_line_participant(string const& line, MutableInputData& inputData)
    {
        string name;
        vector<int> pref;

        auto syntax =
                lit("(person)")
                >> lexeme[ (*(char_ - ':'))[pset(name)] ]
                >> ':'
                >> +(uint_[padd(pref)]);

        if(!parse(line, syntax)) return false;

        for(int i = 0; i < pref.size(); i++)
        {
            pref[i] = -pref[i];
        }

        inputData.participants.push_back(ParticipantData(name, pref));
        return true;
    }

    static bool parse_line_participant_csv(string const& line, MutableInputData& inputData)
    {
        string name;
        vector<int> pref;

        auto syntax =
                (*(char_ - ','))[pset(name)]
                >> uint_[padd(pref)] % ',';

        if(!parse(line, syntax)) return false;
        inputData.participants.push_back(ParticipantData(name, pref));
        return true;

    }

    static bool parse_line_constraint(string const& line, MutableInputData& inputData)
    {
        string constraint;

        auto syntax = lit("(constraint)") >> lexeme[ (*char_)[pset(constraint)] ];

        if(!parse(line, syntax)) return false;
        inputData.constraintStrings.push_back(constraint);
        return true;
    }

    static void parse_line(string& line, MutableInputData& inputData, vector<PreWorkshop>& preWorkshops)
    {
        boost::algorithm::trim(line);
        if(line.empty() || line.front() == '#')
        {
            return;
        }

        if(!parse_line_slot(line, inputData)
        && !parse_line_workshop(line, preWorkshops)
        && !parse_line_participant(line, inputData)
        && !parse_line_constraint(line, inputData)
        && !parse_line_participant_csv(line, inputData))
        {
            throw InputException("Could not parse line \"" + line + "\".");
        }
    }

    static void compile_workshops(MutableInputData& inputData, vector<PreWorkshop>& preWorkshops)
    {
        int wsidx = 0;

        for(PreWorkshop const& pw : preWorkshops)
        {
            vector<int> conductors;
            for(string const& name : pw.cond)
            {
                bool found = false;
                for(int p = 0; p < inputData.participants.size(); p++)
                {
                    if(inputData.participants[p].name() == name)
                    {
                        conductors.push_back(p);
                        vector<int> newPref(inputData.participants[p].preferences());
                        newPref[wsidx] = InputData::MinPrefPlaceholder;
                        inputData.participants[p] = ParticipantData(inputData.participants[p].name(), newPref);
                        inputData.conductors.push_back({.participant = p, .workshop = wsidx});

                        found = true;
                        break;
                    }
                }

                if(!found) throw InputException("Unknown participant \"" + name + "\".");
            }

            inputData.workshops.push_back(WorkshopData(
                    pw.name,
                    pw.min,
                    pw.max,
                    pw.parts > 1 ? std::make_optional(wsidx + 1) : std::nullopt));

            wsidx++;
            if(pw.parts > 1)
            {
                for(int p = 0; p < inputData.participants.size(); p++)
                {
                    vector<int> newPrefs;
                    for(int i = 0; i < inputData.participants[p].preferences().size(); i++)
                    {
                        for(int j = 0; j < (i == wsidx - 1 ? pw.parts : 1); j++)
                        {
                            newPrefs.push_back(inputData.participants[p].preference(i));
                        }
                    }

                    inputData.participants[p] = ParticipantData(inputData.participants[p].name(), newPrefs);
                }

                for(int i = 1; i < pw.parts; i++)
                {
                    string name = InputData::GeneratedPrefix + "[" + str(i + 1) + "] " + pw.name;
                    inputData.workshops.push_back(WorkshopData(
                            name,
                            pw.min,
                            pw.max,
                            i == pw.parts - 1 ? std::nullopt : std::make_optional(wsidx + 1)));
                    wsidx++;
                }
            }
        }
    }

    static void generate_extra_slots(MutableInputData& inputData, vector<PreWorkshop>& preWorkshops)
    {
        int optMin = 0;
        for(PreWorkshop pw : preWorkshops)
        {
            if(!pw.optional) continue;
            optMin += pw.min;
        }

        int numExtraSlots = (int)std::ceil((double)optMin / (double)inputData.participants.size());

        for(int i = 0; i < numExtraSlots; i++)
        {
            string extraSlot = InputData::NotScheduledSlotPrefix + str(i);
            string extraWorkshop = InputData::HiddenWorkshopPrefix + "unassigned_" + str(i);

            int s = inputData.slots.size();

            inputData.slots.push_back(SlotData(extraSlot));
            inputData.workshops.push_back(WorkshopData(extraWorkshop, 0, inputData.participants.size() + 1));
            inputData.constraints.push_back(Constraint(WorkshopIsInSlot, inputData.workshops.size() - 1, s));

            for(int p = 0; p < inputData.participants.size(); p++)
            {
                vector<int> newPref(inputData.participants[p].preferences());
                newPref.push_back(InputData::MinPrefPlaceholder);
                inputData.participants[p] = ParticipantData(inputData.participants[p].name(), newPref);
            }

            for(PreWorkshop pw : preWorkshops)
            {
                if(pw.optional) continue;

                int w = 0;
                for(; w < inputData.workshops.size(); w++)
                {
                    if(inputData.workshops[w].name() == pw.name) break;
                }

                inputData.constraints.push_back(Constraint(WorkshopIsNotInSlot, w, s));
            }
        }
    }

    static InputData parse(string const& input)
    {
        std::stringstream inputStream(input);
        string line;

        MutableInputData inputData;
        vector<PreWorkshop> preWorkshops;

        while(std::getline(inputStream, line, '\n'))
        {
            if(line.back() == '\r') line.resize(line.size() - 1);
            parse_line(line, inputData, preWorkshops);
        }

        compile_workshops(inputData, preWorkshops);
        generate_extra_slots(inputData, preWorkshops);

        return InputData(inputData);
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
        + str(wsCount) + " event(s) and "
        + str(partCount) + " participant(s).");

        return res;
    }
};


