use core::ops::{Add, Mul};

use fixed::{types::{I16F16, I20F12}, traits::Fixed};
use fixed_sqrt::FixedSqrt;

use crate::{MAX_AXIS, MAX_STEPPERS, stack_vec::StackVec};
use arraydeque::{ArrayDeque, Saturating};

#[derive(Debug)]
pub struct Planner {
    num_axis: u8,
    num_stepper: u8,
    pub steps_per_mm: [I20F12; MAX_AXIS],
    
    job_queue: ArrayDeque<PlannerMove, 8, Saturating>,
}

#[repr(u32)]
#[derive(Debug)]
pub enum JobType {
    StepperJobUndef = 0,
    /* moves */
    StepperJobRun,
    StepperJobHomeX,
    StepperJobHomeY,
    StepperJobHomeZ,
    /* not moves */
    StepperJobEnableSteppers,
    StepperJobDisableSteppers,
    StepperJobSync,
}

#[derive(Debug, Clone)]
pub struct FixedVector<T: Fixed + FixedSqrt, const N: usize>([T; N]);

impl <T: Fixed + FixedSqrt, const N: usize> FixedVector<T, N> {
    pub fn mag(&self) -> T {
        let squared_sum = self.0.iter()
            .fold(T::default(), |elem, acc| (*acc * *acc) + elem);
        squared_sum / squared_sum.sqrt()
    }

    pub fn unit(&self) -> Self {
        let mut new_vec = self.clone();
    }
}

#[derive(Debug)]
pub struct Move {
    pub delta_x: u32,
    pub delta_x_steps: u32,
    pub acceleration_steps: u32,
    pub deceleration_steps: u32,

    /// length of the move in mm
    pub len_mm: I20F12,

    pub unit_vec: FixedVector<I20F12, MAX_AXIS>,

    /// Acceleration Speed (mm/s^2)
    pub acceleration_mms2: u32,
    /// (Entry Speed)^2 in (mm/s)^2
    pub entry_speed_mm_sq: u32,
    /// Steady State Speed Squared in mm
    pub speed_mm_sq: u32,
    /// (Exit Speed) squared in mm
    pub exit_speed_mm_sq: u32,
}

#[derive(Debug)]
pub enum PlannerJob {
    StepperJobRun(Move)
}

#[repr(C)]
#[derive(Debug)]
pub struct PlannerMove {
    pub job_type: JobType,
    /// Number of steps each motor needs to move. 
    /// Computed by applying kinematics euqation.
    pub motor_steps: [u32; MAX_STEPPERS],
    /// Position change of the *carriage* in *mm*
    pub delta_x: [I16F16; MAX_AXIS],
    /// Max velocity on each axis for the *carriage* in *mm*
    pub max_v: [u32; MAX_AXIS],
    /// Acceleration on each axis for the *carriage* in *mm*
    pub acc: [u32; MAX_AXIS],
    /// Should the carriage come to a complete stop.
    pub stop: bool,
}

impl Planner {
    pub fn new(num_axis: u32, num_stepper: u32) -> Self {
        Planner {
            num_axis: num_axis as u8,
            num_stepper: num_stepper as u8,
            steps_per_mm: [I20F12::from_num(0); MAX_AXIS],
            job_queue: ArrayDeque::new(),
        }
    }

    pub fn plannerEnqueueMove(&mut self, new_move: &PlannerMove) {

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
}