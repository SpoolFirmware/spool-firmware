pub mod fixed_vector;

use fixed::{
    types::{I16F16, I20F12, U20F12}, traits::ToFixed,
};
use crate::{MAX_AXIS, MAX_STEPPERS};
use arraydeque::{ArrayDeque, Saturating};

use fixed_vector::FixedVector;

#[derive(Debug)]
pub struct Planner {
    num_axis: u8,
    num_stepper: u8,
    pub steps_per_mm: [I20F12; MAX_AXIS],

    job_queue: ArrayDeque<PlannerJob, 8, Saturating>,
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
    pub delta_x_mm: u32,
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
    StepperJobRun(Move),
}

#[repr(C)]
#[derive(Debug)]
pub struct PlannerMove {
    pub job_type: JobType,
    /// Number of steps each motor needs to move.
    /// Computed by applying kinematics euqation.
    pub motor_steps: [i32; MAX_STEPPERS],
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

    fn populate_block(&self, new_move: &PlannerMove, prev_move: Option<&Move>) {
        let delta_x = {
            let mut delta_x = [I20F12::ZERO; MAX_AXIS];
            for i in 0..new_move.delta_x.len() {
                delta_x[i] = new_move.delta_x[i].to_fixed();
            }
            delta_x
        };

        let mut time_est = I20F12::ZERO;
        let mut max_axis_mm = U20F12::ZERO;
        let mut max_axis = 0;
        for (i, x) in delta_x.iter().enumerate() {
            time_est = core::cmp::max(time_est, *x/I20F12::from_num(new_move.max_v[i]));

            if x.unsigned_abs() > max_axis_mm {
                max_axis_mm = x.unsigned_abs();
                max_axis = i;
            }
        }

        if time_est == I20F12::ZERO {
            return
        }

        let acc_mm = {
            let mut acc_mm: U20F12 = new_move.acc[max_axis].to_fixed();
            delta_x.iter()
                   .zip(new_move.acc.iter())
                   .fold(acc_mm, |y, (x, acc)| {
                       if *x == I20F12::ZERO || acc.to_fixed::<U20F12>() <= y {
                           y
                       } else {
                           core::cmp::min(y, acc.to_fixed::<U20F12>() * max_axis_mm / x.unsigned_abs())
                       }
                   });
        };

        // println!("TimeEst: {}s", time_est);
    }

    pub fn enqueue_move(&mut self, new_move: &PlannerMove) -> Result<(), ()> {
        let prev_move = self.job_queue.back().map_or(None, |m| {
            if let PlannerJob::StepperJobRun(run) = m {
                Some(run)
            } else {
                None
            }
        });
        self.populate_block(new_move, prev_move);
        Ok(())
    }
}
