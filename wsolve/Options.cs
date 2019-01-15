using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text.RegularExpressions;
using NDesk.Options;

namespace wsolve
{
    public static class Options
    {
        public static int Verbosity { get; private set; } = 4;
        public static int? Seed { get; private set; }
        public static string InputFile { get; private set; }
        public static string OutputFile { get; private set; }
        public static bool IntermediateFiles { get; private set; }
        public static List<string> ExtraSchedulingConditions { get; } = new List<string>();
        public static List<string> ExtraAssignmentConditions { get; } = new List<string>();
        public static bool WorkshopsOnly { get; private set; }
        public static bool ShowHelp { get; private set; }
        public static bool DirectCode { get; private set; }
        public static bool AnySolution { get; private set; }

        public static int TimeoutScheduling { get; private set; } = 600 * 1000;
        public static int TimeoutAssignment { get; private set; } = 600 * 1000;
        
        public static string OptionsScheduling { get; private set; }
        public static string OptionsAssignment { get; private set; }

        public static double PreferenceExponent { get; private set; } = 3;

        private static readonly Regex RangeRegex = new Regex(@"([0-9]+)\s*\-\s*([0-9]+)", RegexOptions.Compiled);
        
        private static OptionSet OptionSet { get; } = new OptionSet()
        {
            {"i|input=", "Specifies an input file.",
                (string i) => InputFile = i },
            
            {"o|output=", "Specifies an file file.",
                (string i) => OutputFile = i },
            
            {"s|shuffle=", "Sets the seed for the random number generator and shuffles the input.", 
                (int x) => Seed = (x == -1 ? (int?)null : x) },
            
            {"n|intermediate-files", "Do not delete intermediate files (model and data descriptions).", 
                x => IntermediateFiles = x != null },
            
            {"scheduling-only", "Only calculate a workshop scheduling.", 
                x => WorkshopsOnly = x != null },
            
            {"v|verbosity=", "A number between 0 and 4 (default 4) indicating how much status information should be given.", 
                (int v) => Verbosity = v },
            
            {"e|pref-exp=", "The preference exponent. Default 3.0.", 
                (double v) => PreferenceExponent = v },
            
            {"t|timeout=", "Sets the scheduling and assignment timeout simultaneously.",
                (int x) => TimeoutScheduling = TimeoutAssignment = x * 1000 + 1 },

            {"timeout-s=", $"A time in seconds after which the scheduling optimization will be aborted. Default is {TimeoutScheduling / 1000}.",
                (int x) => TimeoutScheduling =  x * 1000 + 1  },
            
            {"timeout-a=", $"A time in seconds after which the assignment optimization will be aborted. Default is {TimeoutAssignment / 1000}.",
                (int x) => TimeoutAssignment =  x * 1000 + 1  },
            
            {"p|options=", "Sets the scheduling and assignment options simultaneously.",
                (string x) => OptionsScheduling = OptionsAssignment = x },
            
            {"options-s=", "A comma separated list of options that will be passed to the scheduling optimizer. See the 'glp_iocp' struct of GLPK for possible options and values.",
                (string x) => OptionsScheduling = x },
            
            {"options-a=", "A comma separated list of options that will be passed to the assignment optimizer. See the 'glp_iocp' struct of GLPK for possible options and values.",
                (string x) => OptionsAssignment = x },
            
            {"extra-s=", "Specify am extra condition for a valid scheduling solution, specified as code fragments written in GNU MathProg. You can also specify a file. You can specify a linear inequality of variables of model (see model files), which will be wrapped in condition statements.", 
                extra => ExtraSchedulingConditions.Add(extra) },
            
            {"extra-a=", "Specify an condition for a valid assignment solution, specified as code fragments written in GNU MathProg. See --extra-scheduling for more infromation.", 
                extra => ExtraAssignmentConditions.Add(extra) },

            {"d|direct-code", "If this flag is set, specified extra conditions will be included as-is in the model. You then have to write complete MathProg conditions.",
                x => DirectCode = x != null },

            {"a|any-solution", "If this flag is set, no optimization is performed; the first found solution will be given.",
                x => AnySolution = x != null },
            
            {"h|help", "Show help.", 
                x => ShowHelp = x != null },
            
            {"version", "Show version.", 
                x => {} },
        };

        public static bool ParseFromArgs(string[] args)
        {
            try
            {
                var rem = OptionSet.Parse(args);

                if (rem.Any())
                {
                    throw new OptionException();
                }
            }
            catch (Exception ex) when (ex is OptionException || ex is InvalidOperationException)
            {
                Status.Error("Invalid Arguments.");
                PrintHelp();
                Environment.Exit(Exit.InvalidArguments);
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
            Console.Error.WriteLine("USAGE: {0} [Options]\n", Path.GetFileNameWithoutExtension(Assembly.GetExecutingAssembly().Location));
            Console.Error.WriteLine("OPTIONS:");
            OptionSet.WriteOptionDescriptions(Console.Error);
            Console.Error.WriteLine("\nINPUT: Consult the Readme file for information about the input format.\n");
            Console.Error.WriteLine("If you need further help, you can contact max.azendorf@outlook.com.");
        }

    }
}