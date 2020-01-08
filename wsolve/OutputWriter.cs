using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace WSolve
{
    public static class OutputWriter
    {
        public static void WriteSolution(Solution solution)
        {
            if (solution.InputData.SlotCount > 1)
            {
                WriteSchedulingSolution(solution);
            }

            WriteAssignmentSolution(solution);
        }

        private static void WriteSchedulingSolution(Solution solution)
        {
            (int ws, int slot)[] scheduling = solution.Scheduling.Select(kvp => (kvp.Key, kvp.Value)).ToArray();

            Console.WriteLine("Workshop Schedule:");

            var slotMin = new int[solution.InputData.Slots.Count];
            var slotMax = new int[solution.InputData.Slots.Count];
            var slotCnt = new int[solution.InputData.Slots.Count];

            foreach ((int ws, int slot) x in scheduling)
            {
                (string name, int min, int max, int? continuation) ws = solution.InputData.Workshops[x.ws];
                string slot = solution.InputData.Slots[x.slot];

                if (slot.StartsWith(InputData.NotScheduledSlotPrefix))
                {
                    slot = "not scheduled";
                }
                
                slotMin[x.slot] += ws.min;
                slotMax[x.slot] += ws.max;

                if (ws.name.StartsWith(InputData.HiddenWorkshopPrefix))
                {
                    continue;
                }
                
                slotCnt[x.slot]++;

                Console.WriteLine($"'{ws.name}' -> '{slot}'.");
            }

            Console.WriteLine();

            if (Options.CsvOutput)
            {
                var str = new StringBuilder();
                str.AppendLine("\"Workshop\",\"Slot\"");

                foreach ((int ws, int slot) x in scheduling)
                {
                    string workshop = solution.InputData.Workshops[x.ws].name;
                    if (workshop.StartsWith(InputData.HiddenWorkshopPrefix))
                    {
                        continue;
                    }
                    
                    string slot = solution.InputData.Slots[x.slot];
                    if (slot.StartsWith(InputData.NotScheduledSlotPrefix))
                    {
                        slot = "not scheduled";
                    }
                    
                    str.AppendLine(
                        $"\"{workshop}\",\"{slot}\"");
                }

                File.WriteAllText(Options.OutputFile + ".scheduling.csv", str.ToString());
            }

            if (!Options.NoStats)
            {
                Status.Info("Scheduling solution statistics:");
                for (int i = 0; i < solution.InputData.Slots.Count; i++)
                {
                    int maxSpan = slotMax[i] - slotMin[i];
                    int realSpan = solution.InputData.Participants.Count - slotMin[i];
                    if (!solution.InputData.Slots[i].StartsWith(InputData.NotScheduledSlotPrefix))
                    {
                        Status.Info(
                            $"    Slot '{solution.InputData.Slots[i]}': {slotCnt[i]} workshop(s), is {(float) realSpan / maxSpan * 100:0.0}% between min-max.");
                        foreach (int ws in scheduling.Where(x => x.slot == i).Select(x => x.ws))
                        {
                            Status.Info($"        {solution.InputData.Workshops[ws].name}");
                        }
                    }
                }

                int[] notScheduledSlots = Enumerable.Range(0, solution.InputData.SlotCount)
                    .Where(s => solution.InputData.Slots[s].StartsWith(InputData.NotScheduledSlotPrefix))
                    .ToArray();
                
                Status.Info($"    Not scheduled: {notScheduledSlots.Sum(s => slotCnt[s])} workshop(s)");                        
                foreach (int ws in scheduling.Where(x => notScheduledSlots.Contains(x.slot)).Select(x => x.ws))
                {
                    if (solution.InputData.Workshops[ws].name.StartsWith(InputData.HiddenWorkshopPrefix))
                    {
                        continue;
                    }
                    Status.Info($"        {solution.InputData.Workshops[ws].name}");
                }
            }
        }

        private static void WriteAssignmentSolution(Solution solution)
        {
            (int p, int ws)[] assignment =
                solution.FlatAssignment.Select(kvp => (kvp.participant, kvp.workshop)).ToArray();

            Console.WriteLine("Assignment:");

            var wsParts = new int[solution.InputData.Workshops.Count];
            var partCnt = new int[solution.InputData.Participants.Max(p => p.preferences.Max()) + 1];

            foreach ((int p, int ws) x in assignment)
            {
                if(solution.InputData.Slots[solution.Scheduling[x.ws]].StartsWith(InputData.NotScheduledSlotPrefix))
                    continue;
                
                (string name, IReadOnlyList<int> preferences) p = solution.InputData.Participants[x.p];
                (string name, int min, int max, int? continuation) ws = solution.InputData.Workshops[x.ws];

                wsParts[x.ws]++;
                partCnt[p.preferences[x.ws]]++;
                Console.WriteLine($"'{p.name}' -> '{ws.name}'");
            }

            if (Options.CsvOutput)
            {
                var str = new StringBuilder();
                str.Append("\"Workshop\"");

                foreach (string s in solution.InputData.Slots)
                {
                    if(s.StartsWith(InputData.NotScheduledSlotPrefix))
                        continue;
                    
                    str.Append($",\"{s}\"");
                }

                foreach (IGrouping<int, (int p, int ws)> g in assignment.GroupBy(x => x.p))
                {
                    int p = g.Key;
                    var workshops = new int[solution.InputData.Slots.Count];
                    for (int i = 0; i < workshops.Length; i++)
                    {
                        int ws = g.ElementAt(i).ws;

                        workshops[solution.Scheduling[ws]] = ws;
                    }

                    str.AppendLine();
                    str.Append($"\"{solution.InputData.Participants[p].name}\"");

                    for (int s = 0; s < solution.InputData.Slots.Count; s++)
                    {
                        if (solution.InputData.Slots[s].StartsWith(InputData.NotScheduledSlotPrefix))
                        {
                            continue;
                        }
                        str.Append($",{solution.InputData.Workshops[workshops[s]].name}");
                    }
                }

                File.WriteAllText(Options.OutputFile + ".assignment.csv", str.ToString());
            }

            if (!Options.NoStats)
            {
                Status.Info("Assignment solution statistics:");
                Status.Info("    Preference distribution: ");
                for (int i = solution.InputData.Participants.Min(p => p.preferences.Min()); i < partCnt.Length; i++)
                {
                    if (partCnt[i] > 0)
                    {
                        string prefStr = Options.RankedPreferences ? $"#{i + 1}" : $"{solution.InputData.MaxPreference - i}";
                        Status.Info($"        Preference {prefStr}: {partCnt[i]} participant(s).");
                    }
                }

                for (int i = 0; i < wsParts.Length; i++)
                {
                    if (solution.InputData.Workshops[i].name.StartsWith(InputData.HiddenWorkshopPrefix))
                    {
                        continue;
                    }
                    
                    Status.Info(
                        $"    Workshop '{solution.InputData.Workshops[i].name}': {wsParts[i]} participant(s) (of {solution.InputData.Workshops[i].max}).");
                }
            }
        }
    }
}