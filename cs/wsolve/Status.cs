using System;

namespace WSolve
{
    public static class Status
    {
        private static readonly object _syncroot = new object();
        
        public static void Info(string text)
        {
            lock (_syncroot)
            {
                if (Options.Verbosity >= 3)
                {
                    Console.Error.WriteLine("INFO:    " + text);
                }
            }
        }

        public static void ImportantInfo(string text)
        {
            lock (_syncroot)
            {
                if (Options.Verbosity >= 1)
                {
                    Console.Error.WriteLine("INFO:    " + text);
                }
            }
        }

        public static void Warning(string text)
        {
            lock (_syncroot)
            {
                ConsoleColor c = Console.ForegroundColor;
                Console.ForegroundColor = ConsoleColor.Yellow;
                if (Options.Verbosity >= 2)
                {
                    Console.Error.WriteLine("WARNING: " + text);
                }

                Console.ForegroundColor = c;
            }
        }

        public static void Error(string text)
        {
            lock (_syncroot)
            {
                ConsoleColor c = Console.ForegroundColor;
                Console.ForegroundColor = ConsoleColor.Red;
                if (Options.Verbosity >= 1)
                {
                    Console.Error.WriteLine("ERROR:   " + text);
                }
#if DEBUG
                Console.Error.WriteLine(Environment.StackTrace);
#endif
                Console.ForegroundColor = c;
            }
        }
    }
}