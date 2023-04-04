use core::ops::Index;

use fixed::traits::{Fixed, ToFixed};
use fixed_sqrt::FixedSqrt;

#[derive(Debug, Clone, PartialEq)]
pub struct FixedVector<T: Fixed + FixedSqrt + PartialEq, const N: usize>([T; N]);

impl <T: Fixed + FixedSqrt, const N: usize> FixedVector<T, N> where T::Unsigned: Fixed + FixedSqrt + ToFixed {
    pub fn new(s: [T; N]) -> Self {
        FixedVector(s)
    }

    pub fn inner(self) -> [T; N] {
        self.0
    }

    pub fn mag(&self) -> T::Unsigned {
        let squared_sum = self.0.iter()
            .fold(T::Unsigned::default(), |elem, acc| (*acc * *acc).to_fixed::<T::Unsigned>() + elem);
        squared_sum / squared_sum.sqrt()
    }

    pub fn unit(&self) -> Self {
        let mag = self.mag();
        let mut new_vec = self.clone();
        new_vec.0.iter_mut().for_each(|f| { *f /= mag.to_fixed::<T>(); });
        new_vec
    }

    pub fn dot(&self, other: &Self) -> T {
        self.0.iter()
              .zip(other.0.iter())
              .fold(T::ZERO, |acc, (a, b)| acc + *a * *b)
    }

    pub fn sub(&self, other: &Self) -> Self {
        let mut new_vec = self.clone();
        new_vec.0.iter_mut()
                 .zip(other.0.iter())
                 .for_each(|(s, o)| *s -= *o);
        new_vec
    }

    pub fn iter(&self) -> impl Iterator<Item = &T> {
        self.0.iter()
    }
}

impl <T: Fixed + FixedSqrt, const N: usize> Index<usize> for FixedVector<T, N> {
    type Output = T;

    fn index(&self, index: usize) -> &Self::Output {
        &self.0[index]
    }
}

#[cfg(test)]
mod tests {
    use fixed::types::I20F12;

    use super::FixedVector;

    #[test]
    fn test_fixed_vec_mag() {
        let vec = FixedVector::<I20F12, 2>([I20F12::from_num(-3), I20F12::from_num(4)]);
        assert_eq!(5, vec.mag().to_num::<i32>());
    }

    #[test]
    fn test_fixed_vec_unit() {
        let vec = FixedVector::<I20F12, 2>([I20F12::from_num(4), I20F12::from_num(0)]);
        assert_eq!(4, vec.mag().to_num::<i32>());
        assert_eq!(FixedVector::<I20F12, 2>([I20F12::from_num(1), I20F12::from_num(0)]), vec.unit());
    }
}
