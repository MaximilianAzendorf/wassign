// ReSharper disable PossiblyImpureMethodCallOnReadonlyVariable

using System;
using System.Diagnostics;
using System.Linq;

namespace WSolve
{
    public enum PrefPumpResult
    {
        Fail,
        Partial,
        Success
    }

    public static class PrefPumpHeuristic
    {
        public static PrefPumpResult TryPump(Chromosome chromosome, int preference, int maxDepth, TimeSpan timeout)
        {
#if DEBUG
            if (Debugger.IsAttached) timeout = TimeSpan.MaxValue;
#endif
            var partShuffle = Enumerable.Range(0, chromosome.InputData.Participants.Count).OrderBy(_ => RNG.NextInt())
                .ToArray();

            var visitedMap = new bool[chromosome.InputData.Participants.Count];
            var startTime = DateTime.Now;

            var successes = 0;
            var fails = 0;

            for (var pi = 0; pi < chromosome.InputData.Participants.Count; pi++)
            {
                var p = partShuffle[pi];
                for (var s = 0; s < chromosome.InputData.Slots.Count; s++)
                {
                    if (chromosome.InputData.Participants[p].preferences[chromosome.Workshop(p, s)] <
                        preference) continue;

                    var state = new State(chromosome, preference, visitedMap, partShuffle, timeout, startTime,
                        maxDepth);

                    if (TryPumpRec(state, p, s, chromosome.Workshop(p, s), 0))
                        successes++;
                    else
                        fails++;
                }
            }

            return fails == 0 ? PrefPumpResult.Success : successes == 0 ? PrefPumpResult.Fail : PrefPumpResult.Partial;
        }

        private static bool TryPumpWorkshopChange(State state, int participant, int slot, int wsOrig, bool onlyEconomic)
        {
            var currentWorkshop = state.Chromosome.Workshop(participant, slot);

            var wsPartCount = new int[state.Chromosome.InputData.Workshops.Count];

            for (var p = 0; p < state.Chromosome.InputData.Participants.Count; p++)
            for (var s = 0; s < state.Chromosome.InputData.Slots.Count; s++)
                wsPartCount[state.Chromosome.Workshop(p, s)]++;

            for (var w = 0; w < state.Chromosome.InputData.Workshops.Count; w++)
            {
                if (w == currentWorkshop) continue;

                if (state.Chromosome.Slot(w) != slot) continue;

                if (onlyEconomic && wsPartCount[w] / (float) state.Chromosome.InputData.Workshops[w].max >=
                    wsPartCount[currentWorkshop] /
                    (float) state.Chromosome.InputData.Workshops[currentWorkshop].max) continue;

                if (state.Chromosome.InputData.Participants[participant].preferences[w] >= state.Preference) continue;

                if (w == wsOrig
                    || state.Chromosome.CountParticipants(w) < state.Chromosome.InputData.Workshops[w].max
                    && state.Chromosome.CountParticipants(wsOrig) > state.Chromosome.InputData.Workshops[wsOrig].min)
                {
                    state.Chromosome.Workshop(participant, slot) = w;
                    return true;
                }
            }

            return false;
        }

        private static bool TryPumpExchangeChain(State state, int participant, int slot, int wsOrig, int depth)
        {
            state.Visited[participant] = true;
            for (var pswitchi = 0; pswitchi < state.Chromosome.InputData.Participants.Count; pswitchi++)
            {
                var pswitch = state.ParticipantShuffle[pswitchi];

                if (pswitch == participant) continue;

                var wswitch = state.Chromosome.Workshop(pswitch, slot);
                if (state.Chromosome.InputData.Workshops[wswitch].conductors.Contains(pswitch)) continue;

                var switchPref = state.Chromosome.InputData.Participants[participant].preferences[wswitch];
                if (switchPref >= state.Preference) continue;

                if (TryPumpRec(state, pswitch, slot, wsOrig, depth + 1))
                {
                    state.Chromosome.Workshop(participant, slot) = wswitch;
                    state.Visited[participant] = false;
                    return true;
                }
            }

            state.Visited[participant] = false;
            return false;
        }

        private static bool TryPumpRec(State state, int participant, int slot, int wsOrig, int depth)
        {
            if (DateTime.Now - state.StartTime > state.Timeout) return false;

            if (state.MaxDepth >= 0 && depth > state.MaxDepth) return false;

            if (state.Visited[participant]) return false;

            var currentWorkshop = state.Chromosome.Workshop(participant, slot);
            if (state.Chromosome.InputData.Workshops[currentWorkshop].conductors.Contains(participant)) return false;

            // We will look for favorable workshops that aren't full yet and also look for chains of participants
            // that can exchange their workshops with each other in order to eliminate the preference.
            //
            // We also try to equalize the filling degree of all workshops (especially for future preference pumping
            // attempts) by first searching only for workshops the participant can change to that have a lower filling
            // degree than his current workshop, then looking for chains, and THEN considering all other workshops.
            //
            if (TryPumpWorkshopChange(state, participant, slot, wsOrig, true)) return true;

            if (TryPumpExchangeChain(state, participant, slot, wsOrig, depth)) return true;

            if (TryPumpWorkshopChange(state, participant, slot, wsOrig, false)) return true;

            return false;
        }

        private struct State
        {
            public readonly Chromosome Chromosome;
            public readonly int Preference;
            public readonly bool[] Visited;
            public readonly int[] ParticipantShuffle;
            public readonly TimeSpan Timeout;
            public readonly DateTime StartTime;
            public readonly int MaxDepth;

            public State(Chromosome chromosome, int preference, bool[] visited, int[] participantShuffle,
                TimeSpan timeout, DateTime startTime, int maxDepth)
            {
                Chromosome = chromosome;
                Preference = preference;
                Visited = visited;
                ParticipantShuffle = participantShuffle;
                Timeout = timeout;
                StartTime = startTime;
                MaxDepth = maxDepth;
            }
        }
    }
}