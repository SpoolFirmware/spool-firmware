use fixed::types::{I16F16, I20F12};

use crate::{MAX_AXIS, MAX_STEPPERS};

pub struct Planner {
    pub num_axis: u8,
    pub steps_per_mm: [I20F12; MAX_AXIS],
}

#[repr(u32)]
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

#[repr(C)]
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
    fn new() -> Self {
        Planner {
            num_axis: 4,
            steps_per_mm: [0u8; MAX_AXIS],
        }
    }

    pub fn init(&mut self) {
        *self = Self::new();        
    }
}