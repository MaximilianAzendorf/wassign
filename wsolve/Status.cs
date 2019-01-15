using System;

namespace wsolve
{
    public static class Status
    {
        public static void Info(string text)
        {
            if(Options.Verbosity >= 3)
                Console.Error.WriteLine("INFO:    " + text);
        }

        public static void Warning(string text)
        {
            var c = Console.ForegroundColor;
            Console.ForegroundColor = ConsoleColor.Yellow;
            if(Options.Verbosity >= 2)
                Console.Error.WriteLine($"WARNING: " + text);
            Console.ForegroundColor = c;
        }

        public static void Error(string text)
        {
            var c = Console.ForegroundColor;
            Console.ForegroundColor = ConsoleColor.Red;
            if(Options.Verbosity >= 1)
                Console.Error.WriteLine($"ERROR:   " + text);
#if DEBUG
            Console.Error.WriteLine(Environment.StackTrace);
#endif
            Console.ForegroundColor = c;
        }

        public static void GLPK(string text)
        {
            var c = Console.ForegroundColor;
            Console.ForegroundColor = ConsoleColor.DarkGray;
            if(Options.Verbosity >= 4)
                Console.Error.WriteLine($"GLPK:    " + text);
            Console.ForegroundColor = c;
        }
    }
}