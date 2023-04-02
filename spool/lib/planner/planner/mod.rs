pub mod fixed_vector;

use fixed_sqrt::FixedSqrt;
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
    pub delta_x_steps: [i32; MAX_STEPPERS],
    pub acceleration_steps: u32,
    pub deceleration_steps: u32,

    /// length of the move in mm
    pub len_mm: U20F12,

    pub unit_vec: FixedVector<I20F12, MAX_AXIS>,

    /// Acceleration Speed (mm/s^2)
    pub acceleration_mms2: U20F12,
    /// (Entry Speed)^2 in (mm/s)^2
    pub entry_speed_mm_sq: U20F12,
    /// Steady State Speed Squared in mm
    pub speed_mm_sq: U20F12,
    /// (Exit Speed) squared in mm
    pub exit_speed_mm_sq: U20F12,
}

impl Move {
    fn reverse_pass_kernel(&mut self) {

    }
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
    /// Min velocity on each axis for the *carriage* in *mm*
    pub min_v: [I16F16; MAX_AXIS],
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
        // using lazy_static (uses spin) or once_cell (uses critical_section) to
        // read a constant seems brain dead, so here it is, for now, a constant
        let junction_stop_vel_thres: I20F12 = I20F12::from_num(0.99999);
        let platform_junction_deviation: I20F12 = I20F12::from_num(0.015);
        let junction_smoothing_thres: I20F12 = I20F12::from_num(-0.707);

        // move in fixed point mm
        let delta_x = {
            let mut delta_x = [I20F12::ZERO; MAX_AXIS];
            for i in 0..new_move.delta_x.len() {
                delta_x[i] = new_move.delta_x[i].to_fixed();
            }
            delta_x
        };

        let delta_x_vec = FixedVector::new(delta_x.clone());
        let len_mm = delta_x_vec.mag();
        let delta_x = delta_x_vec.inner();

        // time taken by the slowest axis, longest axis move magnitude, index
        let (time_est, max_axis_mm, max_axis) = {
            let mut time_est = U20F12::ZERO;
            let mut max_axis_mm = U20F12::ZERO;
            let mut max_axis = 0;
            for (i, x) in delta_x.iter().enumerate() {
                let x_time_est = *x/I20F12::from_num(new_move.max_v[i]);

                assert!(!x_time_est.is_negative());

                time_est = core::cmp::max(time_est, x_time_est);
                if x.unsigned_abs() > max_axis_mm {
                    max_axis_mm = x.unsigned_abs();
                    max_axis = i;
                }
            }
            (time_est, max_axis_mm, max_axis)
        };

        // empty block
        if time_est == U20F12::ZERO {
            return
        }

        let speed_mm_sq = max_axis_mm / time_est;

        // acceleration of the block, capped by moving axes
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

        let unit_vec = FixedVector::new(delta_x).unit();
        let entry_speed_mm_sq: U20F12 = prev_move.map_or(U20F12::ZERO, |prev_move: &Move| {
            let cos_theta = -unit_vec.dot(&prev_move.unit_vec);

            if cos_new_prev > junction_stop_vel_thres {
                let min_speed: U20F12 = new_move.min_v[max_axis].to_fixed();
                min_speed * min_speed
            } else {
                let junction_dev: FixedVector<_, MAX_AXIS> = unit_vec.sub(&prev_move.unit_vec).unit();

                // skipped over a jacc_mm calculation here, convinced myself that it was dead code

                // avoid div by 0 for oneMinusSinThetaDiv2f
                let cos_theta = if cos_theta < -0.999.to_fixed::<I20F12>() { -0.999.to_fixed::<I20F12>() } else { cos_theta };

                let sin_theta_div_2 = (0.5.to_fixed::<I20F12>() * (1.to_fixed::<I20F12>() - cos_theta)).sqrt();
                let one_minus_sin_theta_div_2 = 1.to_fixed::<I20F12>() - sin_theta_div_2;
                let entry_speed_mm_sq = platform_junction_deviation * sin_theta_div_2 * acc_mm / one_minus_sin_theta_div_2;

                // skipped over small segment handling code
                core::cmp::min(speed_mm_sq, core::cmp::min(entry_speed_mm_sq, prev_move.exit_speed_mm_sq))
            }
        });




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
