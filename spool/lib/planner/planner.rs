use fixed::types::{I16F16, I20F12};

use crate::{MAX_AXIS, MAX_STEPPERS, stack_vec::StackVec};

#[derive(Debug)]
pub struct Planner {
    num_axis: u8,
    num_stepper: u8,
    pub steps_per_mm: [I20F12; MAX_AXIS],
    
    job_queue: StackVec<'static, PlannerJob>,
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

#[derive(Debug)]
pub struct Move {
    pub delta_x: u32,
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
    pub fn new(num_axis: u32, num_stepper: u32, queue: &'static mut [PlannerJob]) -> Self {
        Planner {
            num_axis: num_axis as u8,
            num_stepper: num_stepper as u8,
            steps_per_mm: [I20F12::from_num(0); MAX_AXIS],
            job_queue: StackVec::new(queue),
        }
    }
}