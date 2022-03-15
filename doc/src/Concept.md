# Inner workings of wassign


## Overview

The algorithm is based on [shotgun hill climbing](https://en.wikipedia.org/wiki/Hill_climbing). More details about each step are found under the respective section:

0. [Perform Critical Set Analysis](#critical-set-analysis): This step is optional (and is only performed at the very beginning), but the results of the critical set analysis are used in the scheduling as well as the assignment solver to drastically decrease the time it takes for good solutions to be found.
   
1. [Solving the scheduling](#solving-schedulings): Find a possible scheduling using a randomized depth-first search.
   
2. [Solving the assignment](#solving-assignments): Find the best possible assignment for the given scheduling by constructing a corresponding linear optimization or (depending on the given extra constraints) mixed integer programming problem instance and solving it using an MIP solver.
   
3. [Perfoming hill climbing](#hill-climbing): Find a "neighbor scheduling" of the given scheduling (by moving a single event to another slot, if possible) and solve the assignment again. Repeat this until the solution of the neighbor scheduling is worse than the one before (this is the *hill climbing* part).
   
4. [Performing *shotgun* hill climbing](#shotgun-hill-climbing): Do the above again and again until the timeout is reached (this is the *shotgun* part). The best solution found by then is the output of the program.


## Some notation and a word on preferences

### Common notation used throughout this section

\newcommand{\maj}{\textrm{maj}}

\newcommand{\Nat}{\mathbb{N}}

\newcommand{\Slots}{\mathbb{S}}
\newcommand{\Choices}{\mathbb{W}}
\newcommand{\Choosers}{\mathbb{P}}
\newcommand{\CSets}{\mathbb{C}}

\newcommand{\pref}{\varphi}
\newcommand{\prefb}{\overline{\pref}}
\newcommand{\prefm}{\pref_\max}
\newcommand{\Prefs}{\Phi}

\newcommand{\Sched}{\mathcal{S}}
\newcommand{\InvSched}{\mathcal{S}^{-1}}

\newcommand{\Ass}{\mathcal{A}}

\newcommand{\Sol}{\mathcal{L}}
\newcommand{\Sols}{\mathbb{L}}

This part of the manual is rather theory-heavy, so we have to introduce some common notation that is used from here onwards:

#### Notation regarding the input

 - $\Slots$, $\Choices$ and $\Choosers$ are the non-empty sets of slots, choices and choosers respectively.

 - $\min(w)\in\Nat$ and $\max(w)\in\Nat$ are the minimum and maximum number of choosers of a choice $w\in\Choices$. Generally, $0<\min(w)\leq\max(w)$.

 - $\pref_p(w)\in\Nat$ is the preference given by  a chooser $p\in\Choosers$ to the choice $w$. We oftentimes use $\pref_p=\left[\pref_p(w_1), \pref_p(w_2), ..., \pref_p(w_{|\Choices|})\right]$ as an abbreviation for the "preference vector" of chooser $p$ (with respect to an "obvious" ordering of $\Choices=\left\{w_1,...,w_{|\Choices|}\right\}$). 

 - $\Phi$ is the set of all preferences that are given by at least one chooser to at least one choice, so 
 $$\Phi=\bigcup_{p\in\Choosers}\left\{\pref_p(w)\mid w\in\Choices\right\}\text{ .}$$

#### Notation regarding the solutions

 - A scheduling (generally called $\Sched$) is a mapping $\Sched:\Choices\to\Slots$ (which means that a choice $w$ was scheduled to slot $\Sched(w)$ in a given scheduling). More on that in the the [respective section](#solving-schedulings).

 - An assignment (generally called $\Ass$) is a mapping $\Ass:\Choosers\times\Slots\to\Choices$ (which means that a chooser $p$ was assigned to the choice $\Ass(p, s)$ in slot $s$). More on that in the [respective section](#solving-assignments).

 - A solution (generally $\Sol$) is a pair $\Sol=(\Sched_\Sol,\Ass_\Sol)$ of a scheduling and an assignment (that is based on $\Sched_\Sol$). We call $\Sols$ the set of all solutions.

 - $\prefm(\Sol)$ is the maximum preference that a chooser $c$ has given to a choice $w$ where $c$ was assigned to $w$. It is defined as
 $$\prefm(\Sol)=\max\left\{\pref_p(w)|p\in\Choosers,w\in\Choices\wedge \exists s\in\Slots:\Ass_\Sol(p,s)=w\right\}\text{ .}$$
 - A $\pref$-solution is a solution $\Sol$ with $\prefm(\Sol)=\pref$.

### Preferences are mirrored

*It is very important to note* that throughout this section, preferences are "mirrored", meaning a lower preference given by a chooser to a choice means that the chooser "likes" the choice more. This is because

 - The forumla used to score solutions does not work the other way around, and therefore
 - the program also handles preferences this way (they are converted after the input is read).

So, for example, if a chooser has the preferences
$$100, 50, 0, 70, 30, 22$$
given in the input, they will be converted to
$$0, 50, 100, 30, 70, 78$$
assuming that 100 is the maximum preference given in the whole input.


## Solving schedulings

### The problem

When we want to compute a scheduling, we want to assign each of the choices one of the slots available. Given a scheduling $\Sched$, $\Sched(c)=s$ means that the choice $c$ was assigned to the slot $s$. We also define $\InvSched(s)=\left\{w\mid \Sched(w)=s\right\}$ as the inverse scheduling (the set of all choices that are scheduled to be in slot $s$).

This scheduling $\Sched$ has to adhere to some rules because we want to also calculate an assignment based on the scheduling:

 1. The sum of the minima and maxima of all choices in a slot have to allow for the number of choosers, so 
 $$\sum_{w\in\InvSched(s)}\min(w) \,\leq \,|\Choosers|\, \leq \sum_{w\in\InvSched(s)}\max(w)$$ 
 has to hold for each slot $s$, because else it would be impossible to assign every chooser to a choice in a slot where this constraint is violated.
   
 1. Additional constraints that are present in the input have to be satisfied.
   
 2. Ideally, we also want to get schedulings that allow for a better assignment solution. We use [critical set analysis](#critical-set-analysis) as a heuristic for this.

Because of the first requirement (sum of minima and maxima), this problem is NP-hard ([proof](#proof-sched-np-hard)).
 
### Algorithm

Fortunately, despite the problem being NP-hard, it generally isn't hard at all to find valid schedulings for real-world input data.

Because of this, valid schedulings are generated using a randomized (meaning solutions are visited in a semi-randomized order) depth-first backtracking search. A single backtracking step consists of deciding the slot of a single choice.

There are multiple heuristics at play to improve the performance of the scheduling solver.

 - Heuristic for choice order: The order in which choices are handled by the solver is determined by the number of scheduling constraints that apply to the single choices. Choices with the most constraints are handled first. Apart from that, the order of choices is random.
  
 - Heuristic for set order: Given the current partial solution $\Sched_p$, which slots are tried first in the backtracking is determined by $\sum_{w\in\InvSched_p(s)}\max(w)$ (so basically by how "full" the slot $s$ already is in terms of choice maxima). Slots with a lower value are tried first.
  
 - Critical set analysis: [Critical set analysis](#critical-set-analysis) is used by trying to satisfy all critical sets for a set preference level $p$ until a timeout is reached. If the timeout is reached without finding a valid solution, $p$ is raised to the next level (so all critical sets with a preference of $p$ are discarded), and the process is repeated until a solution is found.


## Solving assignments

### The problem

When we want to compute an assignment $\Ass$ based on a given scheduling $\Sched$, we want to assign each chooser to exactly one choice per slot (so for every pair of a slot $s$ and a chooser $p$, there is exactly one choice $\Ass(s, p)$).

We also want the resultion solution $\Sol=(\Sched, \Ass)$ to be the "best" one for the given scheduling, but we first have to define a metric for how "good" a solution is in order to be able to compare it to other solutions.

### Solution scoring

We define a scoring function $F:\Sols\to\mathbb{R}^2$ as follows

$$F(\Sol)=\left(\prefm(\Sol), \sum_{s\in\Slots,p\in\Choosers} \pref_p\left(\Ass_\Sol(s,p)\right)^{\gamma}\right)$$

where $\gamma\in\mathbb{R}^+$ is the so-called *preference exponent* that can be choosen freely and affects which solutions are favored over others. It can be thought of as the "fairness paramter". For more information on this parameter see [the respective section in the manual](#preference-exponent).

We use this function to assign a score to each solution, where lower scores (defined by the usual order relation $<$ over $\mathbb{R}^2$) are assigned to "better" solutions.

Note that we have a scoring function with two components, the first one being $\prefm(\Sol)$. This means that a solution $\Sol_1$ is always "better" than a solution $\Sol_2$ if $\prefm(\Sol_1)<\prefm(\Sol_2)$.

We may also look at the two components of the scoring function separately. We then call them the *major* and *minor term* $F_\maj$ and $F_\min$ of $F$, so $F(\Sol)=\left(F_\maj(\Sol), F_\min(\Sol)\right)$.

### Algorithm

#### Minimum-cost flow network

We calculate the optimal assignment for a given scheduling by modelling the problem as a [minimum-cost flow problem](https://en.wikipedia.org/wiki/Minimum-cost_flow_problem) instance like in this example with 2 slots, 3 choices and 4 choosers and a scheduling $\InvSched(s_1)=\{w_1, w_2\}$, $\InvSched(s_2)=\{w_3\}$:

``` {.tikz additionalPackages="\usepackage{adjustbox}"}
\usetikzlibrary{positioning, arrows}
\begin{tikzpicture}[node distance = {1.2cm}, scale=1.5, thick, every node/.style = {draw, circle}]
    \path 
    (0, 0) node (q11) {$q^1_1$}
    (1, 0) node (q12) {$q^1_2$}
    (2, 0) node (q21) {$q^2_1$}
    (3, 0) node (q22) {$q^2_2$}
    (4, 0) node (q31) {$q^3_1$}
    (5, 0) node (q32) {$q^3_2$}
    (6, 0) node (q41) {$q^4_1$}
    (7, 0) node (q42) {$q^4_2$}

    (7/6, 1.8) node (w1) {$w_1$}
    (7/6*3, 1.8) node (w2) {$w_2$}
    (7/6*5, 1.8) node (w3) {$w_3$}
    
    (7/6*2, 3) node (s1) {$s_1$}
    (7/6*4, 3) node (s2) {$s_2$}
    
    node[draw=none, above left of = s1] (sv1) {$-1$}
    node[draw=none, above left of = s2] (sv2) {$-3$}
    node[draw=none, above left of = w1] (wv1) {$-1$}
    node[draw=none, above right of = w2] (wv2) {$-2$}
    node[draw=none, above right of = w3] (wv3) {$-1$}
    
    node[draw=none, below right of=q11] (qv11) {$1$}
    node[draw=none, below right of=q12] (qv12) {$1$}
    node[draw=none, below right of=q21] (qv21) {$1$}
    node[draw=none, below right of=q22] (qv22) {$1$}
    node[draw=none, below right of=q31] (qv31) {$1$}
    node[draw=none, below right of=q32] (qv32) {$1$}
    node[draw=none, below right of=q41] (qv41) {$1$}
    node[draw=none, below right of=q42] (qv42) {$1$};

    \draw
    (7.6,0.1) -- (7.7,0.1) -- (7.7,1.7) -- (7.6,1.7)
    (7.6,1.9) -- (7.7,1.9) -- (7.7,2.9) -- (7.6,2.9);

    \path
    (7.8,1) node[draw=none, anchor=west, align=left] (x1) {$\text{capacity}=1$ \\ $\text{cost}=\text{preference}^\gamma$}
    (7.8,2.4) node[draw=none, anchor=west, align=left] (x1) {$\text{capacity}=\text{\#choosers}-\min$ \\ $\text{cost}=0$};

    \draw[->] 
    (q11) edge (w1) (q11) edge (w2)
    (q21) edge (w1) (q21) edge (w2)
    (q31) edge (w1) (q31) edge (w2)
    (q41) edge (w1) (q41) edge (w2)
    (q12) edge (w3)
    (q22) edge (w3)
    (q32) edge (w3)
    (q42) edge (w3)
    
    (w1) edge (s1)
    (w2) edge (s1)
    (w3) edge (s2);

    \draw[->, dotted]
    (s1) edge (sv1)
    (s2) edge (sv2)
    (w1) edge (wv1)
    (w2) edge (wv2)
    (w3) edge (wv3)
    (qv11) edge (q11)
    (qv12) edge (q12)
    (qv21) edge (q21)
    (qv22) edge (q22)
    (qv31) edge (q31)
    (qv32) edge (q32)
    (qv41) edge (q41)
    (qv42) edge (q42);
\end{tikzpicture}
```

Or more formally: Given a scheduling $\Sched$, we can construct a minimum-cost flow network $N=(V,E,z,c,a)$ with

 - $V=\Slots\cup\Choices\cup\left\{q^p_s\mid p\in\Choosers,s\in\Slots\right\}$ with a supply function $z:V\to\mathbb{Z}$ having the following values:
   - $z\left(q^p_s\right)=1$.
   - $z(w)=-\min(w)$.
   - $z(s)=-|\Choosers|+\sum_{w\in\InvSched(s)}\min(w)$.

 - $E=\{(w,s)\mid \Sched(w)=s\}\cup\left\{\left(q^p_s, w\right)\mid\Sched(w)=s \wedge q\in\Choosers\right\}$ with a capacity function $c:E\to\Nat$ and a cost function $a:E\to\Nat$ having the following values:
   - $c(w,s)=\max(w)-\min(w)$
   - $a(w,s)=0$.
   - $c(q^p_s, w)=1$.
   - $a(q^p_s, w)=\pref_p(w)^\gamma$.

The optimal flow $f$ of $N$ then is also directly the optimal assignment $\Ass$ with the following conversion:
$$f(q^p_s, w)=1 \Leftrightarrow \Ass(p, s) = w \text{ .}$$
Note that due to the flow integrality theorem, the resulting optimal flow is guaranteed to have integer flow values for every edge.

Furthermore, because how we defined our edge costs, the total cost of the flow is also the minor score $F_\min((\Sched, \Ass))$ of the resulting solution.

#### Handling inter-edge constraints

To model many kinds of additional assignment constraints one may want the solution to adhere to, we need to expand the standard model of a minimum-cost flow problem by what we from here on call the set of *edge implications* $I\subseteq E^2$ that have the following semantics:
$$ (u,v)\in I \Leftrightarrow f(u)\leq f(v) $$
So, if $(u,v)\in I$, a flow of $f(u)=1$ *implies* a flow of $f(v)=1$ on the other edge.

Because such a modified network $N=(V,E,I,z,c,a)$ is not a regular minimu-cost flow instance when $I\neq\emptyset$ and we have to make sure that the flow we are computing still is integer, we are going to translate the network into an [mixed integer programming](https://en.wikipedia.org/wiki/Integer_programming) instance (where each edge is a variable and constraints are based on flow conservation in flow networks) and solve it using a general MIP solver instead.

#### Implication graph

Because we gave to make sure that the flow we are calculating still consists of integer values when $I$ is not empty, we have to declare some variables in our MIP problem as integer variables. The naive approach to this would be to simply make every variable an integer variable, but because integer variables are the main hurdle for finding solutions to an MIP problem fast, we generally want to minimize the number of integer variables we have to introduce.

To find out which variables we need to make integer we can view the pair $(E, I)$ as a directed graph (called the *implication graph*). Note that the edges of the flow network (or equivalently the variables of the respective MIP instance) are the *vertices* of the implication graph and every implication is a directed edge. To avoid confusion, we will from now on call the vertices of the implication graph $variables$ and the edges $implications$.

We then can calculate which variables we need to declare as integer variables so that the whole optimal flow stays integer in a two-step process.

The first step becomes obvious if we observe that all variables in a [strongly connected components](https://en.wikipedia.org/wiki/Strongly_connected_component) (abbreviated as SCC) of the implication graph have to have the same value ([proof](#proof-scc-eq)). This means that for every SCC, we have to make one arbitrary variable in the SCC an integer variable in order for every variable in the SCC to become integer too. Let the set of these variables be $D_1$.

The second step is based on the observation that for an implication $(u,v)$, it is sufficient that either $v$ or $w$ are integer variables (because then the implication becomes a normal integer constraint in each subproblem inside the branch-and-cut process of the MIP solver which does not violate the prerequisites of the integrality theorem). To use this fact, we just have to look at the remaining implication graph $(E', I')$ where we removed

 - all variables (and incident implications) that were part of a SCC (because we already have "dealt" with them in the first step) and in turn
 - all variables with degree zero, because they either 
   - are not affected by any implications to begin with or 
   - they were adjacent to one or more SCCs in the original implication graph (so such particular variable $v$ is only affected by some implications $v\leq w$ or $v\geq w$ where $w$ is already guaranteed to be integer).

We then calculate a [dominating set](https://en.wikipedia.org/wiki/Dominating_set) $D_2$ of $(E', I')$. Because finding the minimal dominating set is an NP-hard problem we can resort to a simple [greedy algorithm](https://en.wikipedia.org/wiki/Set_cover_problem#Greedy_algorithm).

This whole process gives us a set of variables $D=D_1\cup D_2$ that we have to declare as integer variables in order for the optimal flow to be also integer.


#### Binary search through preference levels

So far, we are only optimizing by the minor score $F_\min$. In order to find the optimal solution by $F$, we just do binary search to find the minimum $F_\maj$ for which there exists a solution.

## Critical set analysis

The most important (and most computationally expensive) heuristic used throughout wassign is called *critical set analysis*. It is computed once at the start of the computation and its result is a set $\CSets$ of so called *critical sets*.

### Definition

#### Critical set, CSA, $\pref$-relevant CSA

A critical set $C$ is a set is a pair $C=(\pref_C, W_C)$ with the following properties:

 - $\pref_C\in\Prefs$ and $W_C\subseteq\Choices$,
  
 - There exists a chooser $p\in\Choosers$ for which $W_C=\left\{w\mid \pref_p(w)\leq\pref_C\right\}$ holds true.

The critical set analysis (in short CSA) $\CSets=\left\{C_1, C_2, ..., C_{|\CSets|}\right\}$ is the set of all critical sets.

We further call $\CSets(\varphi)=\left\{C\in\CSets\mid \pref_C=\varphi\right\}$ the $\pref$-relevant critical set analysis ($\pref$-relevant CSA).

**Example:** Given slots $\Slots=\{s_1,s_2,s_3\}$, choices $\Choices=\{a, b, c, d, e, f\}$ and a single chooser $\Choosers=\{p\}$ with preferences $\pref_p=\left[100, 50, 0, 90, 70, 100\right]$, all valid critical sets would be
$$C_1=(100, \{a, b, c, d, e, f\}), \quad C_2=(90, \{b, c, d, e\}),$$
$$C_3=(70, \{b, c, e\}), \quad C_4=(50, \{b, c\}), \quad C_5=(0, \{c\})\text{ .}$$

#### Satisfying critical sets

Given a scheduling $\Sched$, we call a critical set $C$ *satisfied* if
$$\bigcup_{w\in C}\left\{\Sched(w)\right\} = \Slots \text{ .}$$

Or in plain terms: A critical set is satisfied by a scheduling if, in this scheduling, every slot contains at least one choice in the critical set.

We can then use critical sets as a heuristic for how "good" assignments based on a given scheduling can be: If $\Sched$ does not satisfy all critical sets of $\CSets(\pref)$, there can not be a solution based on $\Sched$ that has a maximum preference of $\pref$ or lower ([proof](#proof-sol-implies-csa)).

### Preference bound

A useful piece of information we can immediately get out of the critical sets is a lower bound of the maximum preference a solution can have. We call this bound $\prefb$ the preference bound; it is defined as
$$\prefb=\min\left\{\pref_C\mid C\in\CSets\wedge |C|<|\Slots|\right\}\text{ .}$$
Or in plain words: If a critical set has fewer elements than there are slots in the input, this critical set can not be satisfied by any scheduling, and therefore there can not be any solution with a maximum preference lower than $\prefb$.

### Simplifying critical sets

We can eliminate most of the critical sets in a CSA by the following observation: Given two critical sets $C_1=(\pref_1, W_1)$ and $C_2=(\pref_2, W_2)$, if $\pref_1\leq\pref_2$ and $W_1\supseteq W_2$ then all schedulings satisfying $C_1$ also satisfy $C_2$. We say that $C_1$ is *covered* by $C_2$. 

We can therefore discard any critical sets that are already covered by another critical set; we then just have to alter our definition of $\pref$-relevant critical sets to include all critical sets that have the same *or greater* preference:
$$\CSets(\pref)=\left\{C\in\CSets\mid\pref_C\geq\pref\right\} \text{ .}$$

*Note:* If we need $\CSets(\pref)$ e.g. in the scheduling solver, we can also discard all critical sets that are simply superset of another critical set in $\CSets(\pref)$.


## Hill climbing

So far, we only optimize the solution *for a given scheduling*. In order to find a locally optimal scheduling (and the corresponding optimal assignment), we perform [first-choice hill climbing](https://en.wikipedia.org/wiki/Hill_climbing) by first finding any valid scheduling $S$ (using the [scheduling solving algorithm](#solving-schedulings) as described above) and then modifying this scheduling by moving a single event to a different slot (we call this scheduling a neighbor of $S$) until we can not find any neighbors of $S$ that has a better solution than $S$ itself.

Because the number of neighbors can be very large (and solving the assignment is quite expensive), we limit the number of neighbors that get considered as the next step.

### Shotgun hill climbing

We now optimize to a local optimum. To find the global optimum (or at least a very good local one), we simply repeat the hill climbing process over and over and choose the best solution found after a certain timeout; this is also called shotgun hill climbing or random-restart hill climbing.

This (very simple) approach has three main advantages over other optimization methods:

1. It is very easy to implement,
2. It is trivial to parallelize,
3. It is quite effective.
