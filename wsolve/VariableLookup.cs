using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Text.RegularExpressions;

namespace wsolve
{
    public class VariableLookup<T> : IDictionary<string, T>
    {
        private static readonly Regex VarNameRegex = new Regex(@"^(?=[^\u001f]+$)(?<name>[^\[]+)(?:\s*\[\s*(?:(?<param>\'[^']+\'|\""[^""]+\""|(?!='|"")[^,]+)\s*(?:,|(?=\]))\s*)+\])?$", RegexOptions.Compiled);
        
        private readonly Dictionary<string, T> vars;
        private const char Separator = '\u001f';

        public VariableLookup()
        {
            vars = new Dictionary<string, T>();
        }
        
        private string ToKey(string varName)
        {
            string dequote(string x) => x.First() == x.Last() ? x.Trim('\'').Trim('\"') : x;
            
            Match m = VarNameRegex.Match(varName);

            if (!m.Success)
            {
                throw new ArgumentException($"\"{varName}\" is not a valid variable name.");
            }

            return string.Join(Separator,
                new[] {m.Groups["name"].Value}.Concat(m.Groups["param"].Captures.Select(c => c.Value).Select(dequote)));
        }

        private string ToName(string key)
        {
            string quote(string x) => x.Contains('\'') ? $"\"{x}\"" : $"\'{x}\'";
            
            string[] token = key.Split(Separator);

            return token[0] + "[" + string.Join(',', token.Skip(1).Select(quote)) + "]";
        }

        public IEnumerator<KeyValuePair<string, T>> GetEnumerator()
        {
            foreach(var kvp in vars) yield return new KeyValuePair<string, T>(ToName(kvp.Key), kvp.Value);
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return GetEnumerator();
        }

        public void Add(KeyValuePair<string, T> item)
        {
            Add(item.Key, item.Value);
        }

        public void Clear()
        {
            vars.Clear();
        }

        public bool Contains(KeyValuePair<string, T> item)
        {
            return TryGetValue(item.Key, out T value) && value.Equals(item.Value);
        }

        public void CopyTo(KeyValuePair<string, T>[] array, int arrayIndex)
        {
            var me = this.ToArray();
            Array.Copy(me, 0, array, arrayIndex, me.Length);
        }

        public bool Remove(KeyValuePair<string, T> item)
        {
            throw new InvalidOperationException();
        }

        public int Count => vars.Count;
        public bool IsReadOnly => false;
        public void Add(string key, T value)
        {
            vars.Add(ToKey(key), value);
        }

        public bool ContainsKey(string key)
        {
            return vars.ContainsKey(ToKey(key));
        }

        public bool Remove(string key)
        {
            return vars.Remove(ToKey(key));
        }

        public bool TryGetValue(string key, out T value)
        {
            return vars.TryGetValue(ToKey(key), out value);
        }

        public T this[string key]
        {
            get => vars[ToKey(key)];
            set => vars[ToKey(key)] = value;
        }

        public ICollection<string> Keys => vars.Keys.Select(ToName).ToArray();
        public ICollection<T> Values => vars.Values;
    }
}