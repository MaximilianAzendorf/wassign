
----

# Appendix {-}

## Proof: Scheduling is NP-hard {- #proof-sched-np-hard}

We reduce [PARTITION](https://en.wikipedia.org/wiki/Partition_problem) to SCHEDULING.

Given a PARTITION-Instance $S=\{n_1, n_2, ..., n_k\}$, let $\sigma=\frac{1}{2}\sum_{i=1}^k n_k$. We define the following slot, choice and chooser sets
$$\Slots = \{s_1, s_2\},\; \Choices=\{w_1, w_2, ..., w_k\},\; \Choosers=\{p_1, p_2, ..., p_{\sigma}\}$$
with
$$\min(w_k)=\max(w_k)=n_k \text{ .}$$

Because $\min(w_k)=\max(w_k)=n_k$, the scheduling constraint (for a slot $s$)
$$\sum_{w\in\InvSched(s)}\min(w) \leq |\Choosers| \leq \sum_{w\in\InvSched(s)}\max(w)\text{ ,}$$
implies
$$\sum_{w\in\InvSched(s_1)}\min(w) = \sum_{w\in\InvSched(s_2)}\min(w)$$

Therefore, if and only if there exists a valid scheduling for $(\Slots, \Choices, \Choosers)$ there exists a perfect partitioning of $S$. 
<span class="qed"></span>

## Proof: Non-satisfiability of $\pref$-relevant CSA implies non-existence of $\pref$-solutions {- #proof-sol-implies-csa}

Proof by contradiction: Suppose there is a $\pref$-solution $\Sol=(\Sched, \Ass)$ and let $C\in\CSets(\pref)$ be a $\pref$-relevant critical set that is not satisfied by $\Sched$. 

Then, because $C$ is not satisfied by $\Sched$, there is a slot $s\in\Slots\setminus\bigcup_{w\in C}\{\Sched(w)\}$ and a chooser $p$ with
$$\min\left\{\pref_p(w)\mid w\in\InvSched(s)\right\} > \pref$$
and therefore $\pref_p\left(\Ass(p, s)\right) > \pref$, so $\prefm(\Sol)>\pref$ and $\Sol$ can not be a $\pref$-solution. 
<span class="qed"></span>

## Proof: All variables in a SCC of the implication graph have to have the same value {- #proof-scc-eq}

Given are the variables $V=\{v_1, v_2, ..., v_n\}$ in a strongly connected component of an implication graph $G=(V, I)$. For each pair of variables $u,v\in V$, we know that $v$ is reachable from $u$ and vice versa in $G$ through some paths $(u, a_1, ..., a_i, v)$ and $(v, b_1, ..., b_j, u)$. Applying the semantics of an implication graph, this is equivalent to
$$ u \leq a_1 \leq ... \leq a_i \leq v \;\wedge\; v \leq b_1 \leq ... \leq b_j \leq u $$
which (by transitivity and antisymmetry) implies $u=v$.
<span class="qed"></span>