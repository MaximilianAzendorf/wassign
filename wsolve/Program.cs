using System;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading;

namespace WSolve
{
    internal static class Program
    {
        public static void PrintHeader()
        {
            var assembly = Assembly.GetExecutingAssembly();
            Console.Error.WriteLine(
                "{0} [Version {1}]",
                ((AssemblyTitleAttribute) assembly.GetCustomAttributes(typeof(AssemblyTitleAttribute))
                    .SingleOrDefault())?.Title,
                Assembly.GetExecutingAssembly().GetName().Version);
            Console.Error.WriteLine(
                "{0}\n",
                ((AssemblyCopyrightAttribute) assembly.GetCustomAttributes(typeof(AssemblyCopyrightAttribute))
                    .SingleOrDefault())?.Copyright);
#if DEBUG
            Status.Info($"PID: {Process.GetCurrentProcess().Id}");
#endif
        }

        private static void PrintVersion()
        {
            Console.WriteLine(Assembly.GetExecutingAssembly().GetName().Version);
        }

        private static int Main(string[] args)
        {
#if DEBUG
            if (!Debugger.IsAttached)
            {
                Console.Error.WriteLine("[Press Enter to start Program]");
                Console.ReadKey();
                Thread.Sleep(1000);
            }
#endif

            var ci = CultureInfo.InvariantCulture;
            Thread.CurrentThread.CurrentCulture = ci;
            Thread.CurrentThread.CurrentUICulture = ci;

#if DEBUG
            if (args.Length == 1 && args[0] == "--generate") return InputGenerator.GenMain();
#endif
            if (args.Length == 1 && args[0] == "--version")
            {
                PrintVersion();
                return Exit.OK;
            }

            if (!Options.ParseFromArgs(args)) return Exit.OK;

            if (Options.Verbosity > 0) PrintHeader();

            var input = InputReader.ReadInput();

            if (Options.Seed != null) input.Shuffle(Options.Seed.Value);

            TextWriter wr = null;
            if (Options.OutputFile != null)
            {
                File.Delete(Options.OutputFile);
                Console.SetOut(wr = File.CreateText(Options.OutputFile));
            }

            try
            {
                ISolver solver = new GaSolver();

                var output = solver.Solve(input);
                output.Verify();

                OutputWriter.WriteSolution(output);
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