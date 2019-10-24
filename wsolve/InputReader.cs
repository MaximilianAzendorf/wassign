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
                @"^\(workshop\)\s+((?<name>[a-zA-Z0-9_\- ]+)\s*:\s*(?<conductor>[a-zA-Z0-9+_\- ]+)\s*,\s*(?<min>[0-9]+)\s*\-\s*(?<max>[0-9]+)\s*)*$",
                RegexOptions.Compiled);

        private static readonly Regex SlotRegex =
            new Regex(@"^\(slot\)\s+(?<name>[a-zA-Z0-9_\- ]+)", RegexOptions.Compiled);

        private static readonly Regex ParticipantRegex =
            new Regex(@"^\(person\)\s+(?<name>[a-zA-Z0-9_\- ]+)\s*:(?:\s*(?<pref>[0-9]+))+", RegexOptions.Compiled);

        public static InputData ReadInput()
        {
            try
            {
                Status.Info("Begin parsing input.");
                return Parse(Options.InputFile == null ? Console.In.ReadToEnd() : File.ReadAllText(Options.InputFile));
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
            var lines = textInput.Split('\n');
            var inputData = new InputData();
            var preWorkshops = new List<(string, string, int, int)>();

            for (var i = 0; i < lines.Length; i++)
                if (!string.IsNullOrEmpty(lines[i]))
                    ParseLine(inputData, preWorkshops, lines, i);

            var wsidx = 0;
            foreach (var (name, cond, min, max) in preWorkshops)
            {
                var conductorNames = cond.Split('+');
                var conductors = conductorNames.Select(c => inputData.Participants.FindIndex(0, x => x.name == c))
                    .ToArray();

                foreach (var c in conductors) inputData.Participants[c].preferences[wsidx] = 0;

                inputData.Workshops.Add((
                    name,
                    conductors,
                    min + conductors.Length,
                    max + conductors.Length));

                wsidx++;
            }

            return inputData;
        }

        private static void ParseLine(
            InputData inputData,
            List<(string name, string conductor, int min, int max)> preWorkshops,
            IReadOnlyList<string> lines,
            int index)
        {
            Match m;
            try
            {
                if ((m = WorkshopRegex.Match(lines[index])).Success)
                {
                    preWorkshops.Add((
                        m.Groups["name"].Value,
                        m.Groups["conductor"].Value,
                        int.Parse(m.Groups["min"].Value),
                        int.Parse(m.Groups["max"].Value)));
                }
                else if ((m = SlotRegex.Match(lines[index])).Success)
                {
                    inputData.Slots.Add(m.Groups["name"].Value);
                }
                else if ((m = ParticipantRegex.Match(lines[index])).Success)
                {
                    var pref = m.Groups["pref"].Captures.Select(x => 100 - int.Parse(x.Value)).ToArray();
                    inputData.Participants.Add((m.Groups["name"].Value, pref));
                }
                else
                {
                    throw new FormatException();
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