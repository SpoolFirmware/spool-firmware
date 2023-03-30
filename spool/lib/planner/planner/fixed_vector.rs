use fixed::traits::Fixed;
use fixed_sqrt::FixedSqrt;

#[derive(Debug, Clone, PartialEq)]
pub struct FixedVector<T: Fixed + FixedSqrt + PartialEq, const N: usize>([T; N]);

impl <T: Fixed + FixedSqrt, const N: usize> FixedVector<T, N> {
    pub fn mag(&self) -> T {
        let squared_sum = self.0.iter()
            .fold(T::default(), |elem, acc| (*acc * *acc) + elem);
        squared_sum / squared_sum.sqrt()
    }

    pub fn unit(&self) -> Self {
        let mag = self.mag();
        let mut new_vec = self.clone();
        new_vec.0.iter_mut().for_each(|f| { *f /= mag; });
        new_vec
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