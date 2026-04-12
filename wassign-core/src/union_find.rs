#[derive(Debug, Clone)]
pub(crate) struct UnionFind<T> {
    elements: std::collections::BTreeMap<T, usize>,
    parents: Vec<Option<usize>>,
    heights: Vec<usize>,
}

impl<T> UnionFind<T>
where
    T: Copy + Ord + TryFrom<i32>,
    i32: TryFrom<T>,
{
    pub(crate) fn new(max: T) -> Self {
        let max_value = i32::try_from(max).ok().expect("max value must fit in i32");
        assert!(max_value >= 0, "invalid max value");

        let mut elements = std::collections::BTreeMap::new();
        for value in 0..max_value {
            elements.insert(
                T::try_from(value)
                    .ok()
                    .expect("element must convert from i32"),
                usize::try_from(value).expect("element index must be non-negative"),
            );
        }

        Self {
            parents: vec![None; elements.len()],
            heights: vec![0; elements.len()],
            elements,
        }
    }

    pub(crate) fn join(&mut self, left: T, right: T) -> usize {
        let mut left_idx = self.find(left);
        let mut right_idx = self.find(right);

        if left_idx != right_idx {
            if self.heights[left_idx] > self.heights[right_idx] {
                std::mem::swap(&mut left_idx, &mut right_idx);
            }
            self.parents[left_idx] = Some(right_idx);
            self.heights[right_idx] = self.heights[right_idx].max(self.heights[left_idx] + 1);
        }

        right_idx
    }

    pub(crate) fn find(&self, element: T) -> usize {
        let mut index = *self.elements.get(&element).expect("element not found");
        while let Some(parent) = self.parents[index] {
            index = parent;
        }
        index
    }

    #[cfg(test)]
    pub(crate) fn size(&self) -> usize {
        self.elements.len()
    }

    pub(crate) fn groups(&self) -> Vec<Vec<T>> {
        let mut group_map = std::collections::BTreeMap::<usize, Vec<T>>::new();
        for &element in self.elements.keys() {
            group_map
                .entry(self.find(element))
                .or_default()
                .push(element);
        }
        group_map.into_values().collect()
    }
}

#[cfg(test)]
mod tests {
    use super::UnionFind;

    #[test]
    fn union_find_constructor_works() {
        assert_eq!(UnionFind::<i32>::new(0).size(), 0);
        assert_eq!(UnionFind::<i32>::new(1).size(), 1);
        assert_eq!(UnionFind::<i32>::new(5).size(), 5);
        assert!(std::panic::catch_unwind(|| UnionFind::<i32>::new(-1).size()).is_err());
    }

    #[test]
    fn union_find_works() {
        const MAX: i32 = 4;
        let mut uf = UnionFind::new(MAX);

        for i in 0..MAX {
            for j in (i + 1)..MAX {
                assert_ne!(uf.find(i), uf.find(j));
            }
        }

        let groups = uf.groups();
        let mut found = vec![false; usize::try_from(MAX).expect("MAX must be non-negative")];
        for group in &groups {
            assert_eq!(group.len(), 1);
            let index = usize::try_from(group[0]).expect("group element must be non-negative");
            assert!(!found[index]);
            found[index] = true;
        }

        uf.join(0, 1);
        uf.join(1, 3);
        assert_eq!(uf.find(0), uf.find(1));
        assert_eq!(uf.find(1), uf.find(3));
        assert_ne!(uf.find(1), uf.find(2));

        let mut found_joined_group = false;
        for mut group in uf.groups() {
            if group.len() == 3 {
                assert!(!found_joined_group);
                found_joined_group = true;
                group.sort_unstable();
                assert_eq!(group, vec![0, 1, 3]);
            }
        }

        assert!(found_joined_group);
    }
}
