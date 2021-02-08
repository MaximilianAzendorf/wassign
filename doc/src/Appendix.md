
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