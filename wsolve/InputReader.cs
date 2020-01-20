using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace WSolve
{
    public static class InputReader
    {
        private static readonly Regex WorkshopRegex =
            new Regex(
                @"^\(event\)\s+((?<name>[a-zA-Z0-9_\- ]+)\s*:\s*(?:(?<conductor>[^,\s][^,]*)\s*,\s*)?(?<min>[0-9]+)\s*\-\s*(?<max>[0-9]+)\s*)*(?:\s*\[(?:(?:(?<parts>[1-9][0-9]*) parts|(?<optional>optional))(?:,|(?=\]))\s*)+\])?$",
                RegexOptions.Compiled);

        private static readonly Regex SlotRegex =
            new Regex(@"^\(slot\)\s+(?<name>[a-zA-Z0-9_\- ]+)", RegexOptions.Compiled);

        private static readonly Regex ParticipantRegex =
            new Regex(@"^\(person\)\s+(?<name>[a-zA-Z0-9_\- ]+)\s*:(?:\s*(?<pref>[0-9]+))+", RegexOptions.Compiled);

        private static readonly Regex ConstraintRegex =
            new Regex(@"^\(constraint\)\s+(?<constraint>.+)", RegexOptions.Compiled);
        
        private static readonly Regex FilterRegex =
            new Regex(@"^\(filter\)(?:\s+(?<filter>.+))?", RegexOptions.Compiled);

        public static InputData ReadInput()
        {
            try
            {
                Status.Info("Begin parsing input.");
                string inputString = !Options.InputFiles.Any()
                    ? Console.In.ReadToEnd()
                    : string.Join('\n', Options.InputFiles.Select(File.ReadAllText));
                
                var res = Parse(inputString);
                int slotCount = res.SlotCount - res.Slots.Count(s => s.StartsWith(InputData.GeneratedPrefix));
                int wsCount = res.WorkshopCount - res.Workshops.Count(w => w.name.StartsWith(InputData.GeneratedPrefix));
                int partCount = res.ParticipantCount - res.Participants.Count(p => p.name.StartsWith(InputData.GeneratedPrefix));
                Status.Info($"Read {slotCount} slot(s), {wsCount} event(s), {partCount} participant(s), and {res.SchedulingConstraints.Count}+{res.AssignmentConstraints.Count} constraint(s).");
                return res;
            }
            catch (FileNotFoundException)
            {
                Status.Error("Input file not found.");
                Environment.Exit(Exit.INPUT_FILE_NOT_FOUND);
            }

            return null;
        }

        private static InputData Parse(string textInput)
        {
            string[] lines = textInput.Split('\n');
            var inputData = new MutableInputData();
            var preWorkshops = new List<(string name, string cond, int min, int max, int parts, bool optional)>();

            bool canContinueFilter = false;
            for (int i = 0; i < lines.Length; i++)
            {
                if (!string.IsNullOrWhiteSpace(lines[i]))
                {
                    ParseLine(inputData, preWorkshops, lines, i, ref canContinueFilter);
                }
            }

            if (!inputData.Slots.Any())
            {
                inputData.Slots.Add("Single Slot");
            }

            int minPref = inputData.Participants.Min(p => p.preferences.Min());

            foreach(var p in inputData.Participants)
            {
                for (int i = 0; i < p.preferences.Length; i++)
                {
                    p.preferences[i] -= minPref;
                }
            }

            int wsidx = 0;
            
            foreach ((string name, string cond, int min, int max, int parts, bool optional) in preWorkshops)
            {
                string[] conductorNames = cond.Split('+', StringSplitOptions.RemoveEmptyEntries);
                int[] conductors = conductorNames.Select(c => inputData.Participants.FindIndex(0, x => x.name == c))
                    .ToArray();

                foreach (int c in conductors)
                {
                    inputData.Participants[c].preferences[wsidx] = 0;
                    inputData.Conductors.Add((c, inputData.Workshops.Count));
                }

                inputData.Workshops.Add((name, min, max, parts > 1 ? wsidx + 1 : (int?)null));
                wsidx++;
                
                if (parts > 1)
                {
                    for (int i = 0; i < inputData.Participants.Count; i++)
                    {
                        var newPref = inputData.Participants[i].preferences
                            .SelectMany((p, idx) => idx == wsidx - 1 ? Enumerable.Repeat(p, parts) : new[] {p})
                            .ToArray();

                        inputData.Participants[i] = (inputData.Participants[i].name, newPref);
                    }

                    for (int i = 1; i < parts; i++)
                    {
                        inputData.Workshops.Add((InputData.GeneratedPrefix + $"[{i + 1}] " + name, min, max, i == parts - 1 ? (int?)null : wsidx + 1));
                        wsidx++;
                    }
                }
            }

            int numExtraSlots = (int)Math.Ceiling(preWorkshops.Where(w => w.optional).Sum(w => w.min) / (float)inputData.Participants.Count);
            
            for (int i = 0; i < numExtraSlots; i++)
            {
                string extraSlot = InputData.NotScheduledSlotPrefix + i;
                string extraWorkshop = InputData.HiddenWorkshopPrefix + "unassigned_" + i;
                
                inputData.Slots.Add(extraSlot);
                inputData.Workshops.Add((extraWorkshop, 0, inputData.Participants.Count + 1, null));

                inputData.Constraints.Add($"Workshop(\"{extraWorkshop}\").Slot == Slot(\"{extraSlot}\")");

                for (int p = 0; p < inputData.Participants.Count; p++)
                {
                    var newPref = inputData.Participants[p].preferences.Concat(new[] {0}).ToArray();
                    inputData.Participants[p] = (inputData.Participants[p].name, newPref);
                }

                foreach (var w in preWorkshops.Where(w => !w.optional))
                {
                    inputData.Constraints.Add(
                        $"Workshop(@\"{w.name.Replace("\"", "\"\"")}\").Slot != Slot(\"{extraSlot}\")");
                }
            }

            return inputData.ToImmutableInputData();
        }

        private static void ParseLine(
            MutableInputData inputData,
            List<(string name, string conductor, int min, int max, int parts, bool optional)> preWorkshops,
            IReadOnlyList<string> lines,
            int index,
            ref bool canContinueFilter)
        {
            if (lines[index].StartsWith("#"))
            {
                return;
            }
            
            Match m;
            try
            {
                if ((m = WorkshopRegex.Match(lines[index])).Success)
                {
                    canContinueFilter = false;
                    preWorkshops.Add((
                        m.Groups["name"].Value,
                        m.Groups["conductor"].Length > 0 ? m.Groups["conductor"].Value : "",
                        int.Parse(m.Groups["min"].Value),
                        int.Parse(m.Groups["max"].Value),
                        m.Groups["parts"].Length > 0 ? int.Parse(m.Groups["parts"].Value) : 1,
                        m.Groups["optional"].Length > 0));
                }
                else if ((m = SlotRegex.Match(lines[index])).Success)
                {
                    canContinueFilter = false;
                    inputData.Slots.Add(m.Groups["name"].Value);
                }
                else if ((m = ParticipantRegex.Match(lines[index])).Success)
                {
                    canContinueFilter = false;
                    int[] pref = m.Groups["pref"].Captures.Select(x => -int.Parse(x.Value)).ToArray();
                    if (Options.RankedPreferences)
                    {
                        pref = pref
                            .Select((e, i) => (e, i))
                            .GroupBy(x => x.e)
                            .OrderBy(g => g.Key)
                            .SelectMany((g, i) => g.Select(x => (r:i, x.i)))
                            .OrderBy(x => x.i)
                            .Select(x => x.r)
                            .ToArray();
                    }
                    inputData.Participants.Add((m.Groups["name"].Value, pref));
                }
                else if ((m = ConstraintRegex.Match(lines[index])).Success)
                {
                    canContinueFilter = false;
                    string constraint = m.Groups["constraint"].Value;
                    inputData.Constraints.Add(constraint);
                }
                else if ((m = FilterRegex.Match(lines[index])).Success)
                {
                    canContinueFilter = true;
                    if (!string.IsNullOrEmpty(inputData.Filter))
                    {
                        throw new FormatException();
                    }
                    string filter = m.Groups["filter"].Value;
                    inputData.Filter = filter;
                }
                else
                {
                    if (canContinueFilter)
                    {
                        inputData.Filter += lines[index];
                    }
                    else
                    {
                        throw new FormatException();
                    }
                }
            }
            catch (FormatException)
            {
                Status.Error($"Error in input line {index + 1}.");
                Environment.Exit(Exit.INVALID_INPUT_FILE);
            }
        }
    }
}