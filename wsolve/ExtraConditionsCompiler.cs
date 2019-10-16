using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.Loader;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.Text;

namespace WSolve
{
    public static class ExtraConditionsCompiler
    {
        private static readonly string CodeEnvPlaceholder = "###";
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
                CodeEnvPlaceholderLineOffset = CodeEnvironment.Split('\n').ToList().FindIndex(l => l.Contains(CodeEnvPlaceholder));
            }
        }

        public static Func<Chromosome, bool> Compile(string conditionCode)
        {
            var comp = GenerateCode(CodeEnvironment.Replace(CodeEnvPlaceholder, conditionCode));
            using (MemoryStream s = new MemoryStream())
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
                    var errorLine = firstError.Location.GetLineSpan().StartLinePosition.Line - CodeEnvPlaceholderLineOffset;
                    var firstErrorMessage = $"{errorDescription} (Line {errorLine})";
                    
                    throw new ArgumentException("Could not compile extra conditions: " + firstErrorMessage);
                }

                s.Seek(0, SeekOrigin.Begin);
                var assembly = AssemblyLoadContext.Default.LoadFromStream(s);
                var type = assembly.GetType(CodeEnvClassName);

                return chromosome =>
                    ((CustomFilter) Activator.CreateInstance(type, new object[] {(Chromosome) chromosome})).Result;
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
                MetadataReference.CreateFromFile(typeof(CustomFilter).Assembly.Location),
            };

            return CSharpCompilation.Create("Generated.dll",
                new[] { parsedSyntaxTree }, 
                references: references, 
                options: new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary, 
                    optimizationLevel: OptimizationLevel.Release,
                    assemblyIdentityComparer: DesktopAssemblyIdentityComparer.Default));
        }
    }
}