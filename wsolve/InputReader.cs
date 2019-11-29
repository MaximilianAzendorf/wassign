using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;

namespace WSolve
{
    public static class InputReader
    {
        private static readonly Regex WorkshopRegex =
            new Regex(
                @"^\(event\)\s+((?<name>[a-zA-Z0-9_\- ]+)\s*:\s*(?:(?<conductor>[^,\s][^,]*)\s*,\s*)?(?<min>[0-9]+)\s*\-\s*(?<max>[0-9]+)\s*)*$",
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
                Status.Info($"Read {res.SlotCount} slot(s), {res.WorkshopCount} event(s), {res.ParticipantCount} participant(s), and {res.SchedulingConstraints.Count}+{res.AssignmentConstraints.Count} constraint(s).");
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
            var preWorkshops = new List<(string, string, int, int)>();

            bool canContinueFilter = false;
            for (int i = 0; i < lines.Length; i++)
            {
                if (!string.IsNullOrWhiteSpace(lines[i]))
                {
                    ParseLine(inputData, preWorkshops, lines, i, ref canContinueFilter);
                }
            }

            int wsidx = 0;
            foreach ((string name, string cond, int min, int max) in preWorkshops)
            {
                string[] conductorNames = cond.Split('+');
                int[] conductors = conductorNames.Select(c => inputData.Participants.FindIndex(0, x => x.name == c))
                    .ToArray();

                foreach (int c in conductors)
                {
                    inputData.Participants[c].preferences[wsidx] = 0;
                    inputData.Conductors.Add((c, inputData.Workshops.Count));
                }

                inputData.Workshops.Add((name, min, max));

                wsidx++;
            }

            return inputData.ToImmutableInputData();
        }

        private static void ParseLine(
            MutableInputData inputData,
            List<(string name, string conductor, int min, int max)> preWorkshops,
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
                        m.Groups.ContainsKey("conductor") ? m.Groups["conductor"].Value : "",
                        int.Parse(m.Groups["min"].Value),
                        int.Parse(m.Groups["max"].Value)));
                }
                else if ((m = SlotRegex.Match(lines[index])).Success)
                {
                    canContinueFilter = false;
                    inputData.Slots.Add(m.Groups["name"].Value);
                }
                else if ((m = ParticipantRegex.Match(lines[index])).Success)
                {
                    canContinueFilter = false;
                    int[] pref = m.Groups["pref"].Captures.Select(x => 100 - int.Parse(x.Value)).ToArray();
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