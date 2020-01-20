using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading;
using WSolve.Debugging;

namespace WSolve
{
    internal static class Program
    {
        public static string GetVersionString()
        {
            var version = Assembly.GetExecutingAssembly().GetName().Version;
            return $"{version.Major}.{version.Minor}.{version.Build}";
        }
        
        public static void PrintHeader()
        {
            Assembly assembly = Assembly.GetExecutingAssembly();
            Console.Error.WriteLine(
                "{0} [Version {1}]",
                ((AssemblyTitleAttribute) assembly.GetCustomAttributes(typeof(AssemblyTitleAttribute))
                    .SingleOrDefault())?.Title,
                GetVersionString());
            Console.Error.WriteLine(
                "{0}\n",
                ((AssemblyCopyrightAttribute) assembly.GetCustomAttributes(typeof(AssemblyCopyrightAttribute))
                    .SingleOrDefault())?.Copyright);
#if DEBUG
            Console.Error.WriteLine($"PID: {Process.GetCurrentProcess().Id}");
#endif
        }

        private static void PrintVersion()
        {
            Console.WriteLine(GetVersionString());
        }

        private static int Main(string[] args)
        {
            CultureInfo ci = CultureInfo.InvariantCulture;
            Thread.CurrentThread.CurrentCulture = ci;
            Thread.CurrentThread.CurrentUICulture = ci;
            
#if DEBUG
            if (!Debugger.IsAttached)
            {
                Console.Error.WriteLine("[Press Enter to start Program]");
                Console.ReadKey();
                Thread.Sleep(1000);
            }
            
            if (args.Length == 1 && args[0] == "--generate")
            {
                return InputGenerator.GenMain();
            }
#endif
            if (args.Length == 1 && args[0] == "--version")
            {
                PrintVersion();
                return Exit.OK;
            }

            if (!Options.ParseFromArgs(args))
            {
                return Exit.OK;
            }

            if (Options.Verbosity > 0)
            {
                PrintHeader();
            }

            InputData input = InputReader.ReadInput();

            TextWriter wr = null;
            if (Options.OutputFile != null)
            {
                File.Delete(Options.OutputFile);
                Console.SetOut(wr = File.CreateText(Options.OutputFile));
            }

            try
            {
                ISolver solver = Options.Solver;

                Solution output = solver.Solve(input);

                if (output == null)
                {
                    Status.Warning("No solution found.");
                }
                else
                {
                    output.Verify();
                    Status.Info($"Solution score: " + new Score(input).Evaluate(Candidate.FromSolution(input, output)));
                    OutputWriter.WriteSolution(output);
                }
            }
            catch (WSolveException ex)
            {
                Status.Error(ex.Message);
                return Exit.ERROR;
            }
            catch (VerifyException ex)
            {
                Status.Error("Solution failed verification: " + ex.Message);
                return Exit.ERROR;
            }

            wr?.Close();

            Status.ImportantInfo("Finished computation without errors.");
            return Exit.OK;
        }
    }
}