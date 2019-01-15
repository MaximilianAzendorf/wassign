using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Net;
using System.Runtime.CompilerServices;
using System.Text;
using System.Text.RegularExpressions;
using NDesk.Options;

namespace wsolve
{
    public static class InputReader
    {
        private static readonly Regex LineSplit = new Regex(@"\r\n|\r|\n", RegexOptions.Compiled);
        
        private static readonly Regex WorkshopRegex = new Regex(@"^\(workshop\)\s+((?<name>[a-zA-Z0-9_\- ]+)\s*:\s*(?<conductor>[a-zA-Z0-9_\- ]+)\s*,\s*(?<min>[0-9]+)\s*\-\s*(?<max>[0-9]+)\s*)*$", RegexOptions.Compiled);
        private static readonly Regex SlotRegex = new Regex(@"^\(slot\)\s+(?<name>[a-zA-Z0-9_\- ]+)", RegexOptions.Compiled);
        private static readonly Regex ParticipantRegex = new Regex(@"^\(person\)\s+(?<name>[a-zA-Z0-9_\- ]+)\s*:(?:\s*(?<pref>[0-9]+))+", RegexOptions.Compiled);
        public static Input ReadInput()
        {
            try
            {
                Status.Info("Begin parsing input.");
                return Parse(Options.InputFile == null ? Console.In.ReadToEnd() : File.ReadAllText(Options.InputFile));
            }
            catch (FileNotFoundException)
            {
                Status.Error("Input file not found.");
                Environment.Exit(Exit.InputFileNotFound);
            }
            return null;
        }

        private static string GetAdditionMProgCode(IEnumerable<string> list)
        {
            var lines = list.SelectMany(a =>
            {
                FileInfo f;
                try
                {
                    // ReSharper disable once ObjectCreationAsStatement
                    f = new FileInfo(a);
                }
                catch (Exception ex) when (ex is FileNotFoundException || ex is ArgumentException || ex is NotSupportedException)
                {
                    return new[]{a};
                }
                return f.Exists ? File.ReadAllText(a).Split('\n') : new[]{a};
            });

            if (Options.DirectCode)
            {
                return string.Join('\n', lines);
            }
            else
            {
                string[] fullLines = lines.ToArray();
                for (int i = 0; i < fullLines.Length; i++)
                {
                    fullLines[i] = $"s.t. extra_c{i+1}: {fullLines[i]};";
                }
                return string.Join('\n', fullLines);
            }
        }

        public static IEnumerable<(string name, string value)> GetSolverOptions(string optionString)
        {
            if (optionString == null) yield break;
            
            string[] opt = optionString.Split(',');
            foreach (string o in opt)
            {
                string[] kv = o.Split('=').Select(s => s.Trim()).ToArray();

                if (kv.Length != 2)
                {
                    Status.Error($"Invalid option solver options.");
                    Environment.Exit(Exit.InvalidArguments);
                }

                yield return (kv[0], kv[1]);
            }
        }

        public static string GetAdditionalSchedulingMProgCode() =>
            GetAdditionMProgCode(Options.ExtraSchedulingConditions);
        
        public static string GetAdditionalAssignmentMProgCode() =>
            GetAdditionMProgCode(Options.ExtraAssignmentConditions);

        private static Input Parse(string textInput)
        {
            string[] lines = textInput.Split('\n');
            Input input = new Input();
            List<(string, string, int, int)> preWorkshops = new List<(string, string, int, int)>();

            for (int i = 0; i < lines.Length; i++)
            {
                if(!string.IsNullOrEmpty(lines[i])) ParseLine(input, preWorkshops, lines, i);
            }

            foreach ((string name, string cond, int min, int max) in preWorkshops)
            {
                input.Workshops.Add((
                    name,
                    input.Participants.FindIndex(0, x => x.name == cond),
                    min,
                    max));
            }
            
            return input;
        }
        
        private static void InvalidInputFile()
        {
            Status.Error("Invalid input.");
            Environment.Exit(Exit.InvalidInputFile);
        }

        private static void ParseLine(Input input, List<(string, string, int, int)> preWorkshops, IReadOnlyList<string> lines, int index)
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
                    input.Slots.Add(m.Groups["name"].Value);
                }
                else if ((m = ParticipantRegex.Match(lines[index])).Success)
                {
                    int[] pref = m.Groups["pref"].Captures.Select(x => int.Parse(x.Value)).ToArray();
                    input.Participants.Add((m.Groups["name"].Value, pref));
                }
                else
                {
                    throw new FormatException();
                }
            }
            catch(FormatException)
            {
                Status.Error($"Error in input line {index+1}.");
                Environment.Exit(Exit.InvalidInputFile);
            }
        }
    }
}