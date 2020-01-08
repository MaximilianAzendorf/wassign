using System;
using System.Collections.Generic;
using System.Collections.Immutable;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
using NDesk.Options;

namespace WSolve
{
    public static class Options
    {
        private static readonly Regex TimeRegex =
            new Regex(@"(?<amount>[0-9]+)(?<mult>[s|m|h|d|w])", RegexOptions.Compiled);

        private static readonly Regex ExpIntRegex =
            new Regex(@"^(?<from>[0-9.]+)(?:-(?<to>[0-9.]+)(?:\^(?<exp>[0-9.]+))?)?$", RegexOptions.Compiled);

        private static readonly Regex TournamentRegex =
            new Regex(@"^tournament\((?<size>[1-9][0-9]*)\)$", RegexOptions.Compiled);

        private static readonly IReadOnlyDictionary<string, int> TimeMultipliers = new Dictionary<string, int>
        {
            ["s"] = 1,
            ["m"] = 60,
            ["h"] = 60 * 60,
            ["d"] = 60 * 60 * 24,
            ["w"] = 60 * 60 * 24 * 7
        };

        public static ISolver Solver { get; private set; } = new MinCostFlowSolver();
        
        public static int Verbosity { get; private set; } = 3;

        private static readonly List<string> InputFilesList = new List<string>();
        public static IReadOnlyList<string> InputFiles => InputFilesList.ToImmutableList();

        public static string OutputFile { get; private set; }

        public static bool CsvOutput { get; private set; }

        public static bool ShowHelp { get; private set; }

        public static int TimeoutSeconds { get; private set; } = 60 * 5;

        public static int CriticalSetTimeoutSeconds { get; private set; } = 1;

        public static int CriticalSetProbingRetries { get; } = 120;

        public static bool NoCriticalSets { get; private set; }

        public static bool NoStats { get; private set; }

        public static bool RankedPreferences { get; private set; }
        
        public static double PreferenceExponent { get; private set; } = 3;
        public static bool Any { get; private set; }

        private static OptionSet OptionSet { get; } = new OptionSet
        {
            {
                "i|input=", "Specifies an input file.",
                i => InputFilesList.Add(i)
            },

            {
                "o|output=", "Specifies an output file.",
                i => OutputFile = i
            },

            {
                "c|csv", "Besides the output file, two CSV files will be generated, [output].scheduling.csv and [output].assignment.csv.",
                i => CsvOutput = true
            },

            {
                "v|verbosity=",
                "A number between 0 and 3 (default 3) indicating how much status information should be given.",
                (int v) => Verbosity = v
            },

            {
                "s|solver=",
                $"Selects the solving strategy, the only possible values is '{MinCostFlowSolver.PARAM_NAME}' (for min cost flow analysis with hill climbing). Default is {MinCostFlowSolver.PARAM_NAME}.",
                x => Solver = ParseSolver(x)
            },

            {
                "a|any",
                "Stop after the first found solution.",
                x => Any = true
            },

            {
                "p|pref-exp=", $"The preference exponent. Default is {PreferenceExponent}.",
                (double v) => PreferenceExponent = v
            },

            {
                "r|ranked-pref", "Preferences of every participant will be transformed into a ranking.",
                v => RankedPreferences = true
            },

            {
                "t|timeout=", $"Sets the optimization timeout. Default is {TimeoutSeconds}s.",
                x => TimeoutSeconds = ParseTime(x)
            },

            {
                "cs-timeout=",
                $"Sets the timeout for attempting to statisfy critical sets of a certain pereference level. Default is {CriticalSetTimeoutSeconds}s.",
                x => CriticalSetTimeoutSeconds = ParseTime(x)
            },

            {
                "no-cs", "Do not perform critical set anaylsis.",
                x => NoCriticalSets = true
            },

            {
                "no-stats", "Do not print solution statistics.",
                x => NoStats = true
            },

            {
                "h|help", "Show help.",
                x => ShowHelp = x != null
            },

            {
                "version", "Show version.",
                x => { }
            }
        };

        private static dynamic ThrowInvalidParameter(string value)
        {
            throw new FormatException($"Could not undestand parameter value \"{value}\".");
        }

        private static int ParseTime(string timeString)
        {
            int time = 0;
            int matchedLength = 0;
            foreach (Match m in TimeRegex.Matches(timeString))
            {
                time += int.Parse(m.Groups["amount"].Value) * TimeMultipliers[m.Groups["mult"].Value];
                matchedLength += m.Length;
            }

            if (matchedLength != timeString.Length)
            {
                ThrowInvalidParameter(timeString);
            }

            return time;
        }

        private static ISolver ParseSolver(string solverString)
        {
            return solverString switch
            {
                MinCostFlowSolver.PARAM_NAME => (ISolver) new MinCostFlowSolver(),
                _ => ThrowInvalidParameter(solverString)
            };
        }

        private static int ParseSeed(string seedString)
        {
            unchecked
            {
                int seed = 0;

                foreach (char c in seedString)
                {
                    seed = seed * 37 + c.GetHashCode();
                }

                return seed;
            }
        }

        public static bool ParseFromArgs(string[] args)
        {
            try
            {
                List<string> rem = OptionSet.Parse(args);

                if (rem.Any())
                {
                    throw new OptionException();
                }
            }
            catch (Exception ex) when (ex is OptionException || ex is InvalidOperationException)
            {
                Status.Error("Invalid Arguments.");
                PrintHelp();
                Environment.Exit(Exit.INVALID_ARGUMENTS);
            }

            if (ShowHelp)
            {
                PrintHelp();
                return false;
            }

            if (string.IsNullOrWhiteSpace(OutputFile) && CsvOutput)
            {
                Status.Warning("No output file specified; CSV flag is ignored.");
                CsvOutput = false;
            }

            return true;
        }

        public static void PrintHelp()
        {
            Program.PrintHeader();
            Console.Error.WriteLine("USAGE: {0} [Options]\n",
                Path.GetFileNameWithoutExtension(Assembly.GetExecutingAssembly().Location));
            Console.Error.WriteLine("OPTIONS:");
            OptionSet.WriteOptionDescriptions(Console.Error);
            Console.Error.WriteLine("\nINPUT: Consult the Readme file for information about the input format.\n");
        }
    }
}