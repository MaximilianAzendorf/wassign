namespace WSolve
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Text;

    public static class OutputWriter
    {
        public static void WriteSolution(Solution solution)
        {
            WriteSchedulingSolution(solution);
            WriteAssignmentSolution(solution);
        }
        
        private static void WriteSchedulingSolution(Solution solution)
        {
            (int ws, int slot)[] scheduling = solution.Scheduling.Select(kvp => (kvp.Key, kvp.Value)).ToArray();
            
            Console.WriteLine("Workshop Schedule:");

            int[] slotMin = new int[solution.InputData.Slots.Count];
            int[] slotMax = new int[solution.InputData.Slots.Count];
            int[] slotCnt = new int[solution.InputData.Slots.Count];
            
            foreach (var x in scheduling)
            {
                var ws = solution.InputData.Workshops[x.ws];
                var slot = solution.InputData.Slots[x.slot];
                string name = ws.name;

                slotMin[x.slot] += ws.min;
                slotMax[x.slot] += ws.max;
                slotCnt[x.slot]++;
                
                Console.WriteLine($"'{ws.name}' -> '{slot}'.");
            }

            Console.WriteLine();

            if (Options.CsvOutputFile != null)
            {
                StringBuilder str = new StringBuilder();
                str.AppendLine("\"Workshop\",\"Slot\"");
                
                foreach (var x in scheduling)
                {
                    str.AppendLine($"\"{solution.InputData.Workshops[x.ws].name}\",\"{solution.InputData.Slots[x.slot]}\"");
                }
                
                File.WriteAllText(Options.CsvOutputFile + ".scheduling.csv", str.ToString());
            }

            if (!Options.NoStats)
            {
                Status.Info("Scheduling solution statistics:");
                for (int i = 0; i < solution.InputData.Slots.Count; i++)
                {
                    int maxSpan = slotMax[i] - slotMin[i];
                    int realSpan = solution.InputData.Participants.Count - slotMin[i];
                    if (solution.InputData.Slots[i] == "NULL")
                    {
                        Status.Info($"    Not scheduled: {slotCnt[i]} workshop(s)");
                    }
                    else
                    {
                        Status.Info(
                            $"    Slot '{solution.InputData.Slots[i]}': {slotCnt[i]} workshop(s), is {(float) realSpan / maxSpan * 100:0.0}% between min-max.");
                        foreach (var ws in scheduling.Where(x => x.slot == i).Select(x => x.ws))
                        {
                            Status.Info($"        {solution.InputData.Workshops[ws].name}");
                        }
                    }
                }
            }
        }

        private static void WriteAssignmentSolution(Solution solution)
        {
            (int p, int ws)[] assignment = solution.FlatAssignment.Select(kvp => (kvp.participant, kvp.workshop)).ToArray();
            
            Console.WriteLine("Assignment:");

            int[] wsParts = new int[solution.InputData.Workshops.Count];
            int[] partCnt = new int[solution.InputData.Participants.Max(p => p.preferences.Max()) + 1];

            foreach (var x in assignment)
            {
                var p = solution.InputData.Participants[x.p];
                var ws = solution.InputData.Workshops[x.ws];

                wsParts[x.ws]++;
                partCnt[p.preferences[x.ws]]++;
                Console.WriteLine($"'{p.name}' -> '{ws.name}'");
            }
            
            if (Options.CsvOutputFile != null)
            {
                StringBuilder str = new StringBuilder();
                str.Append($"\"Workshop\"");

                foreach (var s in solution.InputData.Slots)
                {
                    str.Append($",\"{s}\"");
                }

                foreach (var g in assignment.GroupBy(x => x.p))
                {
                    int p = g.Key;
                    int[] workshops = new int[solution.InputData.Slots.Count];
                    for (int i = 0; i < workshops.Length; i++)
                    {
                        int ws = g.ElementAt(i).ws;

                        workshops[solution.Scheduling[ws]] = ws;
                    }
                    
                    str.AppendLine();
                    str.Append($"\"{solution.InputData.Participants[p].name}\"");
                    
                    for (int s = 0; s < solution.InputData.Slots.Count; s++)
                    {
                        str.Append($",{solution.InputData.Workshops[workshops[s]].name}");
                    }
                }
                
                File.WriteAllText(Options.CsvOutputFile + ".assignment.csv", str.ToString());
            }

            if (!Options.NoStats)
            {
                Status.Info("Assignment solution statistics:");
                Status.Info("    Preference distribution: ");
                for (int i = solution.InputData.Participants.Min(p => p.preferences.Min()); i < partCnt.Length; i++)
                {
                    if (partCnt[i] > 0)
                    {
                        Status.Info($"        Preference {100 - i}: {partCnt[i]} participant(s).");
                    }
                }

                for (int i = 0; i < wsParts.Length; i++)
                {
                    Status.Info(
                        $"    Workshop '{solution.InputData.Workshops[i].name}': {wsParts[i] - solution.InputData.Workshops[i].conductors.Count()} participant(s) (of {solution.InputData.Workshops[i].max - solution.InputData.Workshops[i].conductors.Count()}).");
                }
            }
        }
    }
}