use std::collections::HashMap;
use std::hash::Hash;
use std::time::Duration;

use good_lp::{
    Expression, Solution as _, SolutionStatus, SolverModel, Variable, WithTimeLimit, constraint,
    variable, variables,
};

#[derive(Debug, Clone)]
pub struct MipFlow<NodeKey, EdgeKey>
where
    NodeKey: Eq + Hash,
    EdgeKey: Eq + Hash,
{
    pub node_map: HashMap<NodeKey, usize>,
    pub edge_map: HashMap<EdgeKey, usize>,
    pub supply: Vec<i32>,
    pub outgoing: Vec<Vec<usize>>,
    pub incoming: Vec<Vec<usize>>,
    pub edges_max: Vec<i32>,
    pub edges_cost: Vec<i64>,
    pub edge_cost_multiplier: f64,
    pub edge_objective_bounds: Vec<Option<i64>>,
    pub objective_bound_multiplier: f64,
    pub edge_groups: Vec<Vec<usize>>,
    pub blocked_edges: Vec<usize>,
    pub solution: Vec<i32>,
}

impl<NodeKey, EdgeKey> Default for MipFlow<NodeKey, EdgeKey>
where
    NodeKey: Eq + Hash,
    EdgeKey: Eq + Hash,
{
    fn default() -> Self {
        Self {
            node_map: HashMap::new(),
            edge_map: HashMap::new(),
            supply: Vec::new(),
            outgoing: Vec::new(),
            incoming: Vec::new(),
            edges_max: Vec::new(),
            edges_cost: Vec::new(),
            edge_cost_multiplier: 1.0,
            edge_objective_bounds: Vec::new(),
            objective_bound_multiplier: 0.0,
            edge_groups: Vec::new(),
            blocked_edges: Vec::new(),
            solution: Vec::new(),
        }
    }
}

impl<NodeKey, EdgeKey> MipFlow<NodeKey, EdgeKey>
where
    NodeKey: Eq + Hash + Clone,
    EdgeKey: Eq + Hash + Clone,
{
    pub fn add_node(&mut self) -> usize {
        self.solution.clear();
        self.outgoing.push(Vec::new());
        self.incoming.push(Vec::new());
        self.supply.push(0);
        self.node_count() - 1
    }

    pub fn add_keyed_node(&mut self, key: NodeKey) -> usize {
        let node = self.add_node();
        self.node_map.insert(key, node);
        node
    }

    pub fn set_supply(&mut self, node: usize, supply: i32) {
        self.solution.clear();
        self.supply[node] = supply;
    }

    pub fn add_edge(&mut self, from: usize, to: usize, max: i32, unit_cost: i64) -> usize {
        self.solution.clear();
        self.edges_max.push(max);
        self.edges_cost.push(unit_cost);
        self.edge_objective_bounds.push(None);
        let edge = self.edge_count() - 1;
        self.outgoing[from].push(edge);
        self.incoming[to].push(edge);
        edge
    }

    pub fn add_keyed_edge(
        &mut self,
        key: EdgeKey,
        from: usize,
        to: usize,
        max: i32,
        unit_cost: i64,
    ) -> usize {
        let edge = self.add_edge(from, to, max, unit_cost);
        self.edge_map.insert(key, edge);
        edge
    }

    pub fn set_edge_objective_bound(&mut self, edge: usize, bound: i64) {
        self.solution.clear();
        self.edge_objective_bounds[edge] = Some(bound);
    }

    pub fn set_lexicographic_objective_scale(&mut self, big: i64) {
        self.solution.clear();
        self.edge_cost_multiplier = 1.0 / (big as f64);
        self.objective_bound_multiplier = 1.0;
    }

    pub fn create_edge_group_or_block_edges<I>(&mut self, keys: I)
    where
        I: IntoIterator<Item = EdgeKey>,
    {
        let mut edges = Vec::new();
        let mut blocked = false;

        for key in keys {
            if let Some(&edge) = self.edge_map.get(&key) {
                edges.push(edge);
            } else {
                blocked = true;
            }
        }

        if blocked {
            self.blocked_edges.extend(edges);
        } else {
            self.edge_groups.push(edges);
        }
    }

    pub fn solve(&mut self, time_limit: Option<Duration>) -> bool {
        let mut vars = variables!();
        let edge_variables = self
            .edges_max
            .iter()
            .enumerate()
            .map(|(index, &max)| {
                vars.add(
                    variable()
                        .integer()
                        .min(0.0)
                        .max(f64::from(max))
                        .name(format!("v{index}")),
                )
            })
            .collect::<Vec<_>>();

        let objective_bound_variable = (self.objective_bound_multiplier > 0.0)
            .then(|| vars.add(variable().min(0.0).name("major")));
        let edge_cost_objective =
            edge_variables
                .iter()
                .enumerate()
                .fold(Expression::from(0.0), |expr, (index, &var)| {
                    expr + self.edge_cost_multiplier * (self.edges_cost[index] as f64) * var
                });
        let objective = objective_bound_variable.map_or(edge_cost_objective.clone(), |var| {
            edge_cost_objective + self.objective_bound_multiplier * var
        });

        let mut group_switches = Vec::<Variable>::new();
        for (index, _) in self.edge_groups.iter().enumerate() {
            group_switches.push(vars.add(variable().binary().name(format!("s{index}"))));
        }
        let mut model = vars.minimise(objective).using(good_lp::microlp);
        if let Some(limit) = time_limit {
            if limit.is_zero() {
                return false;
            }
            model = model.with_time_limit(limit.as_secs_f64());
        }

        if let Some(objective_bound_variable) = objective_bound_variable {
            for (edge, &bound) in self.edge_objective_bounds.iter().enumerate() {
                let Some(bound) = bound else {
                    continue;
                };
                model = model.with(constraint!(
                    objective_bound_variable >= (bound as f64) * edge_variables[edge]
                ));
            }
        }

        for (group_index, group) in self.edge_groups.iter().enumerate() {
            for &edge in group {
                model = model.with(constraint!(
                    edge_variables[edge] - group_switches[group_index] == 0
                ));
            }
        }

        for &edge in &self.blocked_edges {
            model = model.with(constraint!(edge_variables[edge] == 0));
        }

        for node in 0..self.node_count() {
            let incoming = self.incoming[node]
                .iter()
                .fold(Expression::from(0.0), |expr, &edge| {
                    expr + edge_variables[edge]
                });
            let outgoing = self.outgoing[node]
                .iter()
                .fold(Expression::from(0.0), |expr, &edge| {
                    expr + edge_variables[edge]
                });
            model = model.with(constraint!(
                incoming - outgoing == -f64::from(self.supply[node])
            ));
        }

        let Ok(solution) = model.solve() else {
            return false;
        };
        if !matches!(solution.status(), SolutionStatus::Optimal) {
            return false;
        }

        self.solution = edge_variables
            .iter()
            .map(|&var| solution.value(var).round() as i32)
            .collect();
        true
    }

    pub fn solution_value_at(&self, key: &EdgeKey) -> i32 {
        assert!(
            !self.solution.is_empty(),
            "The MIP flow instance is not solved."
        );
        self.edge_map
            .get(key)
            .map_or(0, |&edge| self.solution[edge])
    }

    pub fn node_count(&self) -> usize {
        self.outgoing.len()
    }

    pub fn edge_count(&self) -> usize {
        self.edges_max.len()
    }
}
