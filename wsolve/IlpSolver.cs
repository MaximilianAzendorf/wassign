using System.Collections.Generic;
using System.Data;
using System.IO;

namespace wsolve
{
    public class IlpSolver : ISolver
    {
        private static GLPK.glp_iocp IocpOptions()
        {
            GLPK.glp_iocp iocp = GLPK.get_default_iocp();
            iocp.fp_heur = 1;
            iocp.sr_heur = 1;
            iocp.cov_cuts = GLPK.glp_iocp.GLP_ON;
            iocp.mir_cuts = GLPK.glp_iocp.GLP_ON;
            iocp.clq_cuts = GLPK.glp_iocp.GLP_ON;
            iocp.gmi_cuts = GLPK.glp_iocp.GLP_ON;
            iocp.mip_gap = 0.5;
            iocp.ps_heur = 1;
            iocp.ps_tim_lim = 3 * 60 * 1000;
            iocp.br_tech = GLPK.glp_iocp.GLP_BR_FFV;

            if (Options.AnySolution)
                iocp.mip_gap = float.MaxValue;

            if (Options.TimeoutScheduling > 0) iocp.tm_lim = Options.TimeoutScheduling;
            
            foreach((string name, string value) in InputReader.GetSolverOptions(Options.OptionsScheduling))
            {
                iocp = iocp.DynamicSet(name, value);
            }
            return iocp;
        }

        
        public Output Solve(Input input)
        {            
            HashSet<string> intermediateFiles = new HashSet<string>();
            
            int firstFeasible = int.MaxValue;
            bool foundSolution = false;
            
            VariableLookup<int> solution = new VariableLookup<int>();
            
            for (int pass = 0; pass < 2; pass++)
            {
                foundSolution = false;
                GLPK.glp_iocp options = IocpOptions();
                if (pass == 0)
                {
                    Status.Info($"======== Feasibility pass ========");
                    options.ps_heur = 0;
                    options.mip_gap = float.MaxValue;
                    options.tm_lim = int.MaxValue;
                    options.bt_tech = GLPK.glp_iocp.GLP_BT_BLB;
                    options.br_tech = GLPK.glp_iocp.GLP_BR_FFV;
                }
                else
                {
                    Status.Info($"======== Solution pass ========");
                }

                int pref = pass == 0 ? 0 : firstFeasible;
                for (; pref <= input.MaxPreference; pref++)
                {
                    Status.Info($"-------- Iteration for preference limit = {pref} --------");
                    (string modFile, string datFile) = GNUMP.CompileWhole(input, pref);
                    intermediateFiles.Add(modFile);
                    intermediateFiles.Add(datFile);

                    solution = GLPK.Solve(modFile, datFile, options);

                    if (solution == null)
                    {
                        Status.Info("No solution. Retry with higher preference limit.");
                        continue;
                    }

                    foundSolution = true;

                    if (pass == 0) break;
                    
                    Status.Info("Found solution.");

                    if (!Options.IntermediateFiles)
                    {
                        foreach (string intermediateFile in intermediateFiles)
                        {
                            File.Delete(intermediateFile);
                        }
                    }
                }

                if (pass == 0)
                {
                    firstFeasible = pref;
                    Status.Info("Found feasible preference limit.");
                }
            }

            if (!foundSolution)
            {
                Status.Info("No solution found.");
            }
            
            return new Output(
                ConstructSchedulingSolution(input, solution),
                ConstructAssignmentSolution(input, solution));
        }
        
        public static IEnumerable<(int workshop, int slot)> ConstructSchedulingSolution(Input input, VariableLookup<int> ssol)
        {
            for (int iws = 0; iws < input.Workshops.Count; iws++)
            {
                (string name, _, _, _) = input.Workshops[iws];
                int slot = 0;
                for (int isl = 0; isl < input.Slots.Count; isl++)
                {
                    if (ssol[$"IsInSlot['{name}','{input.Slots[isl]}']"] == 0)
                    {
                        slot = isl;
                        break;
                    }
                }

                yield return (iws, slot);
            }
        }

        public static IEnumerable<(int participant, int workshop)> ConstructAssignmentSolution(Input input, VariableLookup<int> asol)
        {
            for (int ip = 0; ip < input.Participants.Count; ip++)
            {
                (string pname, _) = input.Participants[ip];
                for (int iws = 0; iws < input.Workshops.Count; iws++)
                {
                    (string wname, _, _, _) = input.Workshops[iws];

                    if (!asol.TryGetValue($"IsInWorkshop['{pname}','{wname}']", out int v) || v != 1) continue;

                    yield return (ip, iws);
                }
            }
        }
    }
}