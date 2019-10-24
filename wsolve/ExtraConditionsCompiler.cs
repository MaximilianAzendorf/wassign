using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Loader;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.Text;

namespace WSolve
{
    public static class ExtraConditionsCompiler
    {
        private static readonly string CodeEnvPlaceholder = "##C";
        private static readonly string CodeEnvExtraPlaceholder = "##E";
        private static readonly string CodeEnvClassName = "WSolve.Generated.ExtraConditions";
        private static readonly int CodeEnvPlaceholderLineOffset;

        private static readonly string CodeEnvironment;

        static ExtraConditionsCompiler()
        {
            using (var reader = new StreamReader(
                typeof(CustomFilter).Assembly.GetManifestResourceStream(
                    "WSolve.resources.ExtraConditionsCodeEnvironment.txt") ?? throw new InvalidOperationException()))
            {
                CodeEnvironment = reader.ReadToEnd();
                CodeEnvPlaceholderLineOffset =
                    CodeEnvironment.Split('\n').ToList().FindIndex(l => l.Contains(CodeEnvPlaceholder));
            }
        }

        public static string GenerateExtraDefinitions(InputData data)
        {
            var extraDefinitions = new StringBuilder();

            (int s, int w, int p) ignored = (0, 0, 0);
            (int s, int w, int p) conflicts = (0, 0, 0);
            var total = 0;

            var usedNames = new HashSet<string>();

            foreach (var s in data.Slots)
            {
                var name = s.Split(' ')[0];
                if (!IsValidIdentifier(name))
                {
                    ignored.s++;
                }
                else if (usedNames.Contains(name))
                {
                    conflicts.s++;
                }
                else
                {
                    extraDefinitions.AppendLine($"private Slot {name} => Slot(\"{name}\");");
                    usedNames.Add(name);
                    total++;
                }
            }

            foreach (var s in data.Workshops.Select(ws => ws.name))
            {
                var name = s.Split(' ')[0];
                if (!IsValidIdentifier(name))
                {
                    ignored.p++;
                }
                else if (usedNames.Contains(name))
                {
                    conflicts.p++;
                }
                else
                {
                    extraDefinitions.AppendLine($"private Workshop {name} => Workshop(\"{name}\");");
                    usedNames.Add(name);
                    total++;
                }
            }

            foreach (var s in data.Participants.Select(p => p.name))
            {
                var name = s.Split(' ')[0];
                if (!IsValidIdentifier(name))
                {
                    ignored.w++;
                }
                else if (usedNames.Contains(name))
                {
                    conflicts.w++;
                }
                else
                {
                    extraDefinitions.AppendLine($"private Participant {name} => Participant(\"{name}\");");
                    usedNames.Add(name);
                    total++;
                }
            }

            if (ignored != (0, 0, 0))
                Status.Warning(
                    $"{ignored.s} slot, {ignored.w} workshop and {ignored.p} participant identifier(s) were ignored.");

            if (conflicts != (0, 0, 0))
                Status.Warning(
                    $"{conflicts.s} slot, {ignored.w} workshop and {ignored.p} participant identifier(s) were omitted due to name conflics.");

            Status.Info($"{total} identifier(s) were generated.");

            return extraDefinitions.ToString();
        }

        public static Func<Chromosome, bool> Compile(string conditionCode, InputData data)
        {
            conditionCode = CodeEnvironment
                .Replace(CodeEnvPlaceholder, conditionCode)
                .Replace(CodeEnvExtraPlaceholder, GenerateExtraDefinitions(data));

            var comp = GenerateCode(conditionCode);
            using (var s = new MemoryStream())
            {
                var compRes = comp.Emit(s);
                if (!compRes.Success)
                {
                    var compilationErrors = compRes.Diagnostics.Where(diagnostic =>
                            diagnostic.IsWarningAsError ||
                            diagnostic.Severity == DiagnosticSeverity.Error)
                        .ToList();

                    var firstError = compilationErrors.First();
                    var errorDescription = firstError.GetMessage();
                    var errorLine = firstError.Location.GetLineSpan().StartLinePosition.Line -
                                    CodeEnvPlaceholderLineOffset + 1;
                    var firstErrorMessage = $"{errorDescription} (Line {errorLine})";

                    throw new WSolveException("Could not compile extra conditions: " + firstErrorMessage);
                }

                s.Seek(0, SeekOrigin.Begin);
                var assembly = AssemblyLoadContext.Default.LoadFromStream(s);
                var type = assembly.GetType(CodeEnvClassName);

                return chromosome =>
                    ((CustomFilter) Activator.CreateInstance(type, chromosome)).Result;
            }
        }

        private static CSharpCompilation GenerateCode(string sourceCode)
        {
            var codeString = SourceText.From(sourceCode);
            var options = CSharpParseOptions.Default.WithLanguageVersion(LanguageVersion.CSharp7_3);

            var parsedSyntaxTree = SyntaxFactory.ParseSyntaxTree(codeString, options);
            var dotNetCoreDir = Path.GetDirectoryName(typeof(object).GetTypeInfo().Assembly.Location);

            var references = new MetadataReference[]
            {
                MetadataReference.CreateFromFile(Path.Combine(dotNetCoreDir, "System.Runtime.dll")),
                MetadataReference.CreateFromFile(typeof(Math).Assembly.Location),
                MetadataReference.CreateFromFile(typeof(Console).Assembly.Location),
                MetadataReference.CreateFromFile(typeof(Enumerable).Assembly.Location),
                MetadataReference.CreateFromFile(typeof(CustomFilter).Assembly.Location)
            };

            return CSharpCompilation.Create(
                "Generated.dll",
                new[] {parsedSyntaxTree},
                references,
                new CSharpCompilationOptions(
                    OutputKind.DynamicallyLinkedLibrary,
                    optimizationLevel: OptimizationLevel.Release,
                    assemblyIdentityComparer: DesktopAssemblyIdentityComparer.Default));
        }

        private static bool IsValidIdentifier(string name)
        {
            return name != "Slot" && name != "Workshop" && name != "Participant" && SyntaxFacts.IsValidIdentifier(name);
        }
    }
}