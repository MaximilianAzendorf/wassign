using System;
using System.Collections.Generic;
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

        public static int Verbosity { get; private set; } = 3;

        public static int? Seed { get; private set; }

        public static string InputFile { get; private set; }

        public static string OutputFile { get; private set; }

        public static string CsvOutputFile { get; private set; }

        public static bool ShowHelp { get; private set; }

        public static int TimeoutSeconds { get; private set; } = 60 * 60;

        public static int PreferencePumpTimeoutSeconds { get; private set; } = 10;

        public static int CriticalSetTimeoutSeconds { get; private set; } = 1;

        public static int CriticalSetProbingRetries { get; } = 120;

        public static int PreferencePumpMaxDepth { get; private set; } = -1;

        public static double FinalPhaseStart { get; private set; } = 0.8;

        public static bool NoGeneticOptimizations { get; private set; }

        public static bool NoLocalOptimizations { get; private set; }

        public static bool NoPrefPump { get; private set; }

        public static bool NoCriticalSets { get; private set; }

        public static bool NoStats { get; private set; }

        public static ExpInterpolation MutationChance { get; private set; } = new ExpInterpolation(0.5, 0.3, 1.0);

        public static ExpInterpolation CrossoverChance { get; private set; } = new ExpInterpolation(0.75, 0.5, 1.0);

        public static ExpInterpolation PopulationSize { get; private set; } = new ExpInterpolation(5000, 30, 1.8);

        public static ISelection Selection { get; private set; } = new TournamentSelection(1.65f);

        public static int BucketSize { get; private set; } = 5000;

        public static string ExtraConditions { get; private set; }

        public static double PreferenceExponent { get; private set; } = 3;

        private static OptionSet OptionSet { get; } = new OptionSet
        {
            {
                "i|input=", "Specifies an input file.",
                i => InputFile = i
            },

            {
                "o|output=", "Specifies an output file.",
                i => OutputFile = i
            },

            {
                "c|csv-output=", "Specifies a csv output file.",
                i => CsvOutputFile = i
            },

            {
                "s|shuffle=", "Sets the seed for the random number generator and shuffles the input.",
                x => Seed = ParseSeed(x)
            },

            {
                "v|verbosity=",
                "A number between 0 and 3 (default 3) indicating how much status information should be given.",
                (int v) => Verbosity = v
            },

            {
                "p|pref-exp=", $"The preference exponent. Default is {PreferenceExponent}.",
                (double v) => PreferenceExponent = v
            },

            {
                "f|final-phase=", $"The beginning of the final phase. Default is {FinalPhaseStart}.",
                (double v) => FinalPhaseStart = v
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
                "prp-timeout=",
                $"Sets the timeout for the preference pump heuristic. Default is {PreferencePumpTimeoutSeconds}s.",
                x => PreferencePumpTimeoutSeconds = ParseTime(x)
            },

            {
                "prp-depth=",
                $"Sets the maximum search depth for the preference pump heuristic. Specify -1 for unbounded depth. Default is {PreferencePumpTimeoutSeconds}.",
                (int x) => PreferencePumpMaxDepth = x
            },

            {
                "a|any-solution",
                "Only compute a greedy solution without any optimizations. Same as --no-ga --no-prp --no-lopt --no-cs.",
                x => NoLocalOptimizations = NoGeneticOptimizations = NoPrefPump = NoCriticalSets = true
            },

            {
                "no-ga", "Do not perform genetic optimizations.",
                x => NoGeneticOptimizations = true
            },

            {
                "no-lopt", "Do not perform local optimizations.",
                x => NoLocalOptimizations = true
            },

            {
                "no-prp", "Do not use preference pump heuristics.",
                x => NoPrefPump = true
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
                "mutation=", $"The mutation chance. Default is {MutationChance}.",
                x => MutationChance = ParseExpInt(x)
            },

            {
                "crossover=", $"The crossover chance. Default is {CrossoverChance}.",
                x => CrossoverChance = ParseExpInt(x)
            },

            {
                "population=", $"The population size. Default is {PopulationSize}.",
                x => PopulationSize = ParseExpInt(x)
            },

            {
                "selection=",
                $"The selection algorithm. Possible values are 'elite' or 'tournament([size])'. Default is {Selection}.",
                x => Selection = ParseSelection(x)
            },

            {
                "bucket-size=", $"The bucket size. Default is {BucketSize}.",
                (int b) => BucketSize = b
            },

            {
                "x|conditions=",
                "Specify extra conditions. See the Readme file for more information. This value will first get interpreted as file name (to a file containing extra conditions). If the file does not exist, it will be interpreted as condition expression.",
                x => ExtraConditions = x
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

        private static void ThrowInvalidParameter(string value)
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

        private static ExpInterpolation ParseExpInt(string expIntString)
        {
            Match match = ExpIntRegex.Match(expIntString);
            if (!match.Success)
            {
                ThrowInvalidParameter(expIntString);
            }

            double from = double.NaN, to = double.NaN, exp = double.NaN;

            try
            {
                from = double.Parse(match.Groups["from"].Value);
                to = match.Groups["to"].Success ? double.Parse(match.Groups["to"].Value) : from;
                exp = match.Groups["exp"].Success ? double.Parse(match.Groups["exp"].Value) : 1.0;
            }
            catch (FormatException)
            {
                ThrowInvalidParameter(expIntString);
            }

            return new ExpInterpolation(from, to, exp);
        }

        private static ISelection ParseSelection(string selectionString)
        {
            if (selectionString == "elite")
            {
                return new EliteSelection();
            }

            Match match = TournamentRegex.Match(selectionString);
            if (!match.Success)
            {
                ThrowInvalidParameter(selectionString);
            }

            return new TournamentSelection(int.Parse(match.Groups["size"].Value));
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