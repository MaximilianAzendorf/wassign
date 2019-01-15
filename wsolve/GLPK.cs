using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Reflection.Metadata.Ecma335;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
// ReSharper disable InconsistentNaming
// ReSharper disable MemberCanBePrivate.Local
// ReSharper disable FieldCanBeMadeReadOnly.Local
// ReSharper disable MemberHidesStaticFromOuterClass

namespace wsolve
{
    public static class GLPK
    {
        private const string GlpkLibrary = "libglpk.so";
        
        private const int GLP_MIP = 3;
        private const int GLP_ON  = 1;
        private const int GLP_OFF = 0;

        private const int GLP_UNDEF = 1;
        private const int GLP_FEAS = 2;
        private const int GLP_INFEAS = 3;
        private const int GLP_NOFEAS = 4;
        private const int GLP_OPT = 5;
        private const int GLP_UNBND = 6;

        [StructLayout(LayoutKind.Sequential)]
        public unsafe struct glp_smcp
        {
            public int msg_lev;
            public int meth;
            public int pricing;
            public int r_test;
            public double tol_bnd;
            public double tol_dj;
            public double tol_piv;
            public double obj_ll;
            public double obj_ul;
            public int it_lim;
            public int tm_lim;
            public int out_frq;
            public int out_dly;
            public int presolve;

            private fixed byte unused[288];
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public unsafe struct glp_iocp
        {
            private static readonly Dictionary<string, object> Constants;

            static glp_iocp()
            {
                Constants = new Dictionary<string, object>();
                var cfields = typeof(glp_iocp)
                    .GetFields(BindingFlags.Public | BindingFlags.Static)
                    .Where(m => m.IsLiteral && !m.IsInitOnly)
                    .Where(m => m.FieldType == typeof(int));

                foreach (var c in cfields)
                {
                    string[] nameparts = c.Name.Split('_');
                    for (int i = 0; i < nameparts.Length; i++)
                    {
                        string name = string.Join("_", nameparts.Skip(i));
                        Constants.Add(name, c.GetRawConstantValue());
                    }
                }
            }
            
            public const int GLP_ON  = 1;
            public const int GLP_OFF = 0;
            
            public const int GLP_BR_FFV = 1;
            public const int GLP_BR_LFV = 2;
            public const int GLP_BR_MFV = 3;
            public const int GLP_BR_DTH = 4;
            public const int GLP_BR_PCH = 5;

            public const int GLP_BT_DFS = 1;
            public const int GLP_BT_BFS = 2;
            public const int GLP_BT_BLB = 3;
            public const int GLP_BT_BPH = 4;

            public const int GLP_PP_NONE = 0;
            public const int GLP_PP_ROOT = 1;
            public const int GLP_PP_ALL  = 2;
            
            public int msg_lev;
            public int br_tech;
            public int bt_tech;
            public double tol_int;
            public double tol_obj;
            public int tm_lim;
            public int out_frq;
            public int out_dly;

            private IntPtr cb_func;
            private IntPtr cb_info;
            private int cb_size;

            public int pp_tech;

            public double mip_gap;
            public int mir_cuts;
            public int gmi_cuts;
            public int cov_cuts;
            public int clq_cuts;
            public int presolve;
            public int binarize;
            public int fp_heur;
            public int ps_heur;
            public int ps_tim_lim;
            public int sr_heur;

            private fixed byte unused[256];

            public glp_iocp DynamicSet(string name, string value)
            {
                var field = typeof(glp_iocp).GetField(name);
                if (!Constants.TryGetValue(value, out object obj))
                {
                    double v = double.Parse(value);
                    obj = field.FieldType == typeof(double) ? v : (int) v;
                }

                object boxed = this;
                field.SetValue(boxed, obj);
                return (glp_iocp) boxed;
            }
        }

        private static readonly GlpkHookDelegateState glpkOutput = new GlpkHookDelegateState();
        private static GlpkHookDelegate glpkDelegate;
        
        private class GlpkHookDelegateState
        {
            private StringBuilder nextLine = new StringBuilder();
            
            public int Callback(IntPtr infp, IntPtr sptr)
            {
                string s = Marshal.PtrToStringAnsi(sptr);
                string[] lines = s.Split('\n');
                for (int i = 0; i < lines.Length - 1; i++)
                {
                    nextLine.Append(lines[i]);
                    Status.GLPK(nextLine.ToString());
                    nextLine.Clear();
                }

                nextLine.Append(lines.Last());
                return -1;
            }

            public void Flush()
            {
                if(nextLine.Length > 0) Status.GLPK(nextLine.ToString());
                nextLine.Clear();
            }
        }
        
        static GLPK()
        {
            glpkDelegate = glpkOutput.Callback;
            glp_term_hook(glpkDelegate, IntPtr.Zero);
        }
        
        [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
        private delegate int GlpkHookDelegate(IntPtr info, IntPtr s);

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern void glp_term_hook([MarshalAs(UnmanagedType.FunctionPtr)] GlpkHookDelegate func, IntPtr info);
        
        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern IntPtr glp_create_prob();

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern IntPtr glp_mpl_alloc_wksp();

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern int glp_mpl_read_model(IntPtr tran, string fname, int skip);

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern int glp_mpl_read_data(IntPtr tran, string fname);

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern int glp_mpl_generate(IntPtr tran, string fname);

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern void glp_mpl_build_prob(IntPtr tran, IntPtr lp);

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern void glp_mpl_postsolve(IntPtr tran, IntPtr lp, int sol);

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern double glp_mip_col_val(IntPtr lp, int j);

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern unsafe int glp_simplex(IntPtr lp, glp_smcp* parm);
        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern unsafe int glp_intopt(IntPtr lp, glp_iocp* parm);

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern void glp_write_mip(IntPtr lp, string fname);
        
        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern void glp_write_sol(IntPtr lp, string fname);
        
        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern int glp_get_num_cols(IntPtr lp);
        
        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern IntPtr glp_get_col_name(IntPtr lp, int j);

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern int glp_find_row(IntPtr lp, string name);

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern unsafe void glp_init_iocp(glp_iocp* parm);
        
        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern unsafe void glp_init_smcp(glp_smcp* parm);

        [DllImport(GlpkLibrary, SetLastError = true)]
        private static extern int glp_mip_status(IntPtr lp);

        private static string glp_get_col_name_ex(IntPtr lp, int j) =>
            Marshal.PtrToStringAnsi(glp_get_col_name(lp, j));

        public static unsafe glp_iocp get_default_iocp()
        {
            glp_iocp iocp = new glp_iocp();
            glp_init_iocp(&iocp);
            return iocp;
        }

        public static unsafe glp_smcp get_default_smcp()
        {
            glp_smcp smcp = new glp_smcp();
            glp_init_smcp(&smcp);
            return smcp;
        }
        
        public static unsafe VariableLookup<int> Solve(string modFile, string datFile, glp_iocp iocp)
        {
            Status.Info("Starting glpk solver.");
            Stopwatch sw = Stopwatch.StartNew();
            
            IntPtr lp = glp_create_prob();
            IntPtr tran = glp_mpl_alloc_wksp();
            
            int ret = glp_mpl_read_model(tran, modFile, 0);
            if (ret != 0)
            {
                Status.Error($"GLPK Error parsing model, Code {ret}.");
                Environment.Exit(Exit.ModelParsingError);
            }

            ret = glp_mpl_read_data(tran, datFile);
            if (ret != 0)
            {
                Status.Error($"GLPK Error parsing data, Code {ret}.");
                Environment.Exit(Exit.DataParsingError);
            }

            ret = glp_mpl_generate(tran, null);
            if (ret != 0)
            {
                Status.Error($"GLPK Error generating model, Code {ret}.");
                Environment.Exit(Exit.ModelGenerationError);
            }
            
            glp_mpl_build_prob(tran, lp);

            glp_smcp smcp = new glp_smcp();
            glp_init_smcp(&smcp);
            smcp.meth = 2;
            glp_simplex(lp, &smcp);
            glp_intopt(lp, &iocp);
            if (Options.IntermediateFiles)
            {
                glp_write_mip(lp, Path.GetFileNameWithoutExtension(modFile) + ".ilp.sol");
            }
            
            glp_mpl_postsolve(tran, lp, GLP_MIP);
            
            int colCount = glp_get_num_cols(lp);
            
            VariableLookup<int> sol = new VariableLookup<int>();

            for (int i = 1; i <= colCount; i++)
            {
                sol.Add(glp_get_col_name_ex(lp, i), (int)Math.Round(glp_mip_col_val(lp, i)));
            }

            int solStat = glp_mip_status(lp);
            
            glpkOutput.Flush();
            Status.Info($"Finished glpk solver. Solver took {sw.Elapsed}.");
            return solStat == GLP_FEAS || solStat == GLP_OPT ? sol : null;
        }
    }
}