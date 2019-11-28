using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Loader;
using System.Text;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.Emit;
using Microsoft.CodeAnalysis.Text;
using WSolve.ExtraConditions;
using WSolve.ExtraConditions.Constraints;

namespace WSolve.ExtraConditions
{
    public static class ConditionCompiler
    {
        private static readonly string CodeEnvPlaceholder = "##C";
        private static readonly string CodeEnvExtraPlaceholder = "##E";

        private static readonly string CodeEnvStatelessPostfix =
            nameof(WorkshopStateless).Substring(nameof(Workshop).Length);
        
        private static readonly string ConstraintCodeEnvClassName = "WSolve.Generated.CustomConstraints";
        private static readonly string FilterCodeEnvClassName = "WSolve.Generated.CustomFilter";
        
        private static readonly int ConstraintCodeEnvLineOffset;
        private static readonly int FilterCodeEnvLineOffset;

        private static readonly string ConstraintCodeEnvironment;
        private static readonly string FilterCodeEnvironment;

        private static readonly string ConstraintCodeEnvResName =
            "WSolve.Resources.ConstraintCodeEnvironment.txt";
        private static readonly string FilterCodeEnvResName =
            "WSolve.Resources.FilterCodeEnvironment.txt";

        static (string content, int lineOffset) LoadCodeEnvironment(string resourceName)
        {
            using (var reader = new StreamReader(
                typeof(CustomConstraintsBase).Assembly.GetManifestResourceStream(
                    resourceName) ?? throw new InvalidOperationException()))
            {
                string codeEnv = reader.ReadToEnd();
                int lineOffset = codeEnv.Split('\n').ToList().FindIndex(l => l.Contains(CodeEnvPlaceholder));

                return (codeEnv, lineOffset);
            }
        }
        
        static ConditionCompiler()
        {
            (ConstraintCodeEnvironment, ConstraintCodeEnvLineOffset) = LoadCodeEnvironment(ConstraintCodeEnvResName);
            (FilterCodeEnvironment, FilterCodeEnvLineOffset) = LoadCodeEnvironment(FilterCodeEnvResName);
        }

        private static string GenerateExtraDefinitions(MutableInputData data)
        {
            var extraDefinitions = new StringBuilder();

            (int s, int w, int p) ignored = (0, 0, 0);
            (int s, int w, int p) conflicts = (0, 0, 0);
            int total = 0;

            var usedNames = new HashSet<string>();

            foreach (string s in data.Slots)
            {
                string name = s.Split(' ')[0];
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
                    extraDefinitions.AppendLine($"private Slot {name} => Slot(\"{s}\");");
                    usedNames.Add(name);
                    total++;
                }
            }

            foreach (string w in data.Workshops.Select(ws => ws.name))
            {
                string name = w.Split(' ')[0];
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
                    extraDefinitions.AppendLine($"private Workshop {name} => Workshop(\"{w}\");");
                    usedNames.Add(name);
                    total++;
                }
            }

            foreach (string p in data.Participants.Select(p => p.name))
            {
                string name = p.Split(' ')[0];
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
                    extraDefinitions.AppendLine($"private Participant {name} => Participant(\"{p}\");");
                    usedNames.Add(name);
                    total++;
                }
            }

            (int s, int w, int p) sum = (conflicts.s + ignored.s, conflicts.w + ignored.w, conflicts.p + ignored.p);
            if (sum != (0, 0, 0))
            {
                Status.Warning(
                    $"{sum.s} slot, {sum.w} workshop and {sum.p} participant identifier(s) are not available to constraints and filters due to name conflics.");
            }

            return extraDefinitions.ToString();
        }

        public static IEnumerable<Constraint> CompileConstraints(this MutableInputData inputData)
        {
            if (!inputData.Constraints.Any())
            {
                return Enumerable.Empty<Constraint>();
            }
            
            string code = string.Join('\n', inputData.Constraints.Select(c => $"AddConstraint({c});"));
            
            CustomConstraintsBase constraints = CompileObject<CustomConstraintsBase>(code, inputData,
                ConstraintCodeEnvironment, ConstraintCodeEnvLineOffset, ConstraintCodeEnvClassName);

            return constraints.GetStaticConstraints();
        }

        public static Func<Chromosome, bool> CompileFilter(this MutableInputData inputData)
        {
            if (string.IsNullOrEmpty(inputData.Filter))
            {
                return _ => true;
            }

            CustomFilterBase filter = CompileObject<CustomFilterBase>(inputData.Filter, inputData,
                FilterCodeEnvironment, FilterCodeEnvLineOffset, FilterCodeEnvClassName);

            return filter.GetFilterResult;
        }
        
        public static T CompileObject<T>(string conditionCode, MutableInputData inputData, string codeEnvironment, int lineOffset, string className)
        {
            conditionCode = codeEnvironment
                .Replace(CodeEnvPlaceholder, conditionCode)
                .Replace(CodeEnvExtraPlaceholder, GenerateExtraDefinitions(inputData));

            CSharpCompilation comp = GenerateCode(conditionCode);
            using var s = new MemoryStream();
            
            EmitResult compRes = comp.Emit(s);
            if (!compRes.Success)
            {
                List<Diagnostic> compilationErrors = compRes.Diagnostics.Where(diagnostic =>
                        diagnostic.IsWarningAsError ||
                        diagnostic.Severity == DiagnosticSeverity.Error)
                    .ToList();

                Diagnostic firstError = compilationErrors.First();
                string errorDescription = firstError.GetMessage();
                int errorLine = firstError.Location.GetLineSpan().StartLinePosition.Line - lineOffset + 1;
                string firstErrorMessage = $"{errorDescription} (Line {errorLine})";

                throw new WSolveException("Could not compile extra conditions: " + firstErrorMessage);
            }

            s.Seek(0, SeekOrigin.Begin);
            Assembly assembly = AssemblyLoadContext.Default.LoadFromStream(s);
            Type type = assembly.GetType(className);

            if (!typeof(T).IsAssignableFrom(type))
            {
                throw new InvalidOperationException($"The compiled object is not assignable to type {typeof(T)}");
            }

            return (T)Activator.CreateInstance(type, inputData.ToImmutableInputDataDontCompile());
        }

        private static CSharpCompilation GenerateCode(string sourceCode)
        {
            SourceText codeString = SourceText.From(sourceCode);
            CSharpParseOptions options = CSharpParseOptions.Default.WithLanguageVersion(LanguageVersion.CSharp7_3);

            SyntaxTree parsedSyntaxTree = SyntaxFactory.ParseSyntaxTree(codeString, options);
            string dotNetCoreDir = Path.GetDirectoryName(typeof(object).GetTypeInfo().Assembly.Location);

            var references = new MetadataReference[]
            {
                MetadataReference.CreateFromFile(Path.Combine(dotNetCoreDir, "System.Runtime.dll")),
                MetadataReference.CreateFromFile(typeof(Math).Assembly.Location),
                MetadataReference.CreateFromFile(typeof(Console).Assembly.Location),
                MetadataReference.CreateFromFile(typeof(Enumerable).Assembly.Location),
                MetadataReference.CreateFromFile(typeof(CustomConstraintsBase).Assembly.Location)
            };

            return CSharpCompilation.Create(
                $"Generated_{DateTime.Now.Ticks}.dll",
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