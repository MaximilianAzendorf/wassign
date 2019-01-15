using System;
using System.Collections.Generic;
using System.Linq;

namespace wsolve
{
    public static class OutputWriter
    {
        public static void WriteSolution(Input input, Output output)
        {
            WriteSchedulingSolution(input, output.Scheduling.Select(kvp => (kvp.Key, kvp.Value)));
            WriteAssignmentSolution(input, output.FlatAssignment.Select(kvp => (kvp.participant, kvp.workshop)));
        }
        
        private static void WriteSchedulingSolution(Input input, IEnumerable<(int ws, int slot)> solution)
        {
            Console.WriteLine("Workshop Schedule:");

            int[] slotMin = new int[input.Slots.Count];
            int[] slotMax = new int[input.Slots.Count];
            int[] slotCnt = new int[input.Slots.Count];
            
            foreach(var x in solution)
            {
                var ws = input.Workshops[x.ws];
                var slot = input.Workshops[x.slot];
                string name = ws.name;

                slotMin[x.slot] += ws.min;
                slotMax[x.slot] += ws.max;
                slotCnt[x.slot]++;
                
                Console.WriteLine($"'{ws.name}' -> '{slot}'.");
            }
            
            Status.Info("Scheduling solution statistics:");
            for (int i = 0; i < input.Slots.Count; i++)
            {
                int maxSpan = slotMax[i] - slotMin[i];
                int realSpan = input.Participants.Count - slotMin[i];
                if (input.Slots[i] == "NULL")
                {
                    Status.Info($"    Not scheduled: {slotCnt[i]} workshop(s)");
                }
                else
                {
                    Status.Info($"    Slot '{input.Slots[i]}': {slotCnt[i]} workshop(s), is {(float)realSpan / maxSpan*100:0.0}% between min-max.");
                }
            }
            
            Console.WriteLine();
        }

        private static void WriteAssignmentSolution(Input input, IEnumerable<(int p, int ws)> solution)
        {
            Console.WriteLine("Assignment:");

            int[] wsParts = new int[input.Workshops.Count];
            int[] partCnt = new int[input.Participants.Max(p => p.preferences.Max()) + 1];

            foreach(var x in solution)
            {
                var p = input.Participants[x.p];
                var ws = input.Workshops[x.ws];

                wsParts[x.ws]++;
                partCnt[p.preferences[x.ws]]++;
                Console.WriteLine($"'{p.name}' -> '{ws.name}'");
            }

            Status.Info("Assignment solution statistics:");
            Status.Info("    Preference distribution: ");
            for (int i = input.Participants.Min(p => p.preferences.Min()); i < partCnt.Length; i++)
            {
                Status.Info($"     - Preference {i}: {partCnt[i]} participant(s).");
            }

            for (int i = 0; i < wsParts.Length; i++)
            {
                Status.Info($"    Workshop '{input.Workshops[i].name}': {wsParts[i]} participant(s).");
            };
        }
    }
}