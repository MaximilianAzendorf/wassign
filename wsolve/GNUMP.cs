using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;

namespace wsolve
{
    public static class GNUMP
    {
        public const string WholeModFile = "model/whole.gmod";
        public const string SchedulingModFile = "model/scheduling.gmod";
        public const string AssignmentModFile = "model/assignment.gmod";

        public const string ModExt = ".mod";
        public const string DatExt = ".dat";
        
        public const string WholeOutput = "whole";
        public const string SchedulingOutput = "scheduling";
        public const string AssignmentOutput = "assignment";
        
        public static (string modFile, string datFile) CompileWhole(Input input, int prefLevel)
        {
            Status.Info($"Compiling model and data for preference level {prefLevel}.");
            Stopwatch sw = Stopwatch.StartNew();
            
            var res = Compile(
                File.ReadAllText(WholeModFile),
                InputReader.GetAdditionalSchedulingMProgCode(),
                $"param allowed_preference := {prefLevel};",
                input);

            Status.Info($"Finished compilation. Compilation took {sw.Elapsed}.");
            string modFile = WholeOutput + ModExt;
            string datFile = WholeOutput + DatExt;
            File.WriteAllText(modFile, res.prog);
            File.WriteAllText(datFile, res.data + "\nend;\n");
            return (modFile, datFile);
        }

        private static string W(int i) => $"'W{i}'";
        private static string P(int i) => $"'P{i}'";
        private static string S(int i) => $"'S{i}'";

        private static (string prog, string data) Compile(
            string baseCode,
            string additionalCode,
            string additionalData,
            Input input)
        {
            string prog = baseCode + "\n\n/* Additional code */\n\n" + additionalCode + "\nend;\n";
            
            StringBuilder data = new StringBuilder();

            data.Append("set PARTICIPANTS :=");
            for (int i = 0; i < input.Participants.Count; i++) data.Append($" '{input.Participants[i].name}'");
            data.AppendLine(";");
            data.Append("set WORKSHOPS :=");
            for (int i = 0; i < input.Workshops.Count; i++) data.Append($" '{input.Workshops[i].name}'");
            data.AppendLine(";");
            data.Append("set SLOTS :=");
            for (int i = 0; i < input.Slots.Count; i++) data.Append($" '{input.Slots[i]}'");
            data.AppendLine(";");
            data.AppendLine();
            data.AppendLine($"param pref_exp := {Options.PreferenceExponent};");
            data.AppendLine();
            
            data.AppendLine($"param: min_participants max_participants conductor :=");
            for(int i = 0; i < input.Workshops.Count; i++)
            {
                var ws = input.Workshops[i];
                data.AppendLine($"'{ws.name}'  {ws.min} {ws.max} '{ws.conductor}'");
            }
            data.AppendLine(";");
            
            data.Append("param preference:\n");
            for (int i = 0; i < input.Workshops.Count; i++)
            {
                data.Append($"'{input.Workshops[i].name}' ");
            }
            data.Append(":=");
            for (int p = 0; p < input.Participants.Count; p++)
            {
                data.Append($"\n'{input.Participants[p].name}' ");
                for (int w = 0; w < input.Workshops.Count; w++)
                {
                    data.Append($" {input.Participants[p].preferences[w]}");
                }
            }

            data.AppendLine(";");
            data.AppendLine(additionalData);

            return (prog, data.ToString());
        }
    }
}