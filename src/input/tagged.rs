#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub enum Tag {
    Min,
    Max,
    Bounds,
    Parts,
    Optional,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub struct Tagged {
    pub tag: Tag,
    pub values: Vec<i32>,
}

impl Tagged {
    pub fn new(tag: Tag, values: Vec<i32>) -> Self {
        Self { tag, values }
    }

    pub fn value(&self, index: usize) -> i32 {
        self.values[index]
    }
}
