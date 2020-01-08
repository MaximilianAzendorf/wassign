# The algorithm behind wsolve

## High-level overview

The high level algorithm is based on [shotgun hill climbing](https://en.wikipedia.org/wiki/Hill_climbing):
1. Find a possible scheduling using a randomized depth-first search.
2. Find the best possible assignment for the given scheduling by constructing a corresponding linear optimization or (depending on the given extra constraints) mixed integer programming problem and solving it using a MIP solver.
3. Find a "neighbor scheduling" of the given scheduling (by moving a single event to another slot, if possible) and solve the assignment again. Repeat this until the solution of the neighbor scheduling is worse than the one before (this is the *hill climbing* part).
3. Do the above again and again until the timeout is reached (this is the *shotgun* part). The best solution found by then is the output of the program.

### Solution scoring

The algorithm needs to compare solutions based on how "good" they are. This is accomplished by assigning a score to each solution. This score is a pair *(major, minor)* defined as follows:
- Major score: The major score of a solution is the lowest preference any of the participants gave to one of the events they were assigned to. This is also called the *preference minimum* of a solution.
- Minor score: Let *p1, p2, ..., pn* be the list of all preferences that were assigned by a participant to one of the events they were assigned to and *P* the maximum of all preferences. The minor score is the sum over *(P-pk)^e* for all *k*=*1...n*, where *e* is the preference exponent, a parameter that can be freely picked and should be greater than one. Generally speaking, a higher preference exponent yields a "fairer" solution.

Two of these scores are first compared by the major, then (if equal) by the minor component, where a higher major component or a lower minor component indicates a "better" solution. 

This means that a solution where someone got assigned to an event for which they gave a preference of 10 is always worse than a solution where everyone gave their assigned workshops a preference of 11 or higher.

## Scheduling solving using backtraking

Valid schedulings are generated using a randomized depth-first backtracking search. Subtrees in the solution space are pruned on multiple conditions to reduce the search space as much as possible. Apart from that, another heuristic is used in order to find schedulings that allow for better assignment solutions:

### Critical set heuristic

Additionally to the basic backtracking search, the algorithm tries to satisfy *critical sets* as a heurstic to find schedulings that allow the preference minimum to be as high as possible (which is the main scoring criterion as described before).

Critical sets are computed statically once before the actual algorithm starts and are defined as follows:

A critical set is a set of events (labeled with a preference *p*) that have to cover all slots as a necessary prerequisite for the preference minimum to be at least *p*.

For example, given 3 slots, 6 events *A, B, C, D, E, F* and a participant with preferences `0, 10, 100, 5, 20, 0`. A critical set for *p*=10 would be {*B, C, E*}, because in order for the preference limit to be 10 or higher, these three events have to cover all three slots; a scheduling where two of these events are in the same slot would not *satisfy* this critical set, thus there would be no assignment for this scheduling where the preference minimum is 10 or higher.

This also allows us to immediately calculate an upper bound for the preference minimum: A critical set with fewer elements than there are slots can not be satisfied by any scheduling. For the example above, a critical set for *p*=20 would be {*C, E*} which has only 2 elements (
which is less than the number of slots, 3 in this case), so we know that the preference minimum has to be lower than 20.

## Assignment solving using (integer) linear optimization

The best possible assignment for a given scheduling is computed by transforming the scheduling and all preferences into a [min-cost flow](https://en.wikipedia.org/wiki/Minimum-cost_flow_problem) instance and solving it using a dedicated solver. Because it is possible to specify extra constraints, this min-cost flow can not be solved directly, but has to be transformed into a [mixed-integer programming](https://en.wikipedia.org/wiki/Integer_programming) instance instead:

Additional constraints are realised by removing certain edges from the min-cost flow instance or by adding so-called *edge groups*. An edge group is a set of edges in the min-cost flow instance which have capacity 1 and where all edges in the set have to have the same flow in the solution. Because these edge groups can not be expressed in a min-cost flow instance, the min-cost flow instance is instead transformed into a MIP instance; exactly one integer variable is added for each edge group. If there are no edge groups, no integer variables are present.

This MIP instance is then solved using an external MIP solver.