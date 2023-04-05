pub mod fixed_vector;

use itertools::Itertools;

use core::{cell::RefCell, fmt::Display, result::Result};

use crate::{MAX_AXIS, MAX_STEPPERS};
use arraydeque::{ArrayDeque, Saturating};
use fixed::{
    traits::{Fixed, ToFixed},
    types::{I16F16, I20F12, U20F12},
};
use fixed_sqrt::FixedSqrt;
use log::warn;

use fixed_vector::FixedVector;

#[derive(Debug)]
pub enum PlannerError {
    ArithmeticError,
    InvariantError,
    CapacityError,
}

impl Display for PlannerError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "{:?}", self)
    }
}

#[derive(Debug)]
pub struct Planner {
    num_axis: u8,
    num_stepper: u8,
    pub steps_per_mm: [I20F12; MAX_AXIS],

    job_queue: ArrayDeque<RefCell<PlannerJob>, 8, Saturating>,
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
    pub max_axis: usize,
    pub accelerate_mm: U20F12,
    pub decelerate_mm: U20F12,

    /// length of the move in mm
    pub len_mm: U20F12,

    pub unit_vec: FixedVector<I20F12, MAX_AXIS>,

    /// Acceleration (mm/s^2)
    pub acceleration_mms2: U20F12,
    /// (Entry Speed)^2 in (mm/s)^2
    pub entry_speed_mm_sq: U20F12,
    /// Steady State Speed Squared in mm
    pub speed_mm_sq: U20F12,
    /// (Exit Speed) squared in mm
    pub exit_speed_mm_sq: U20F12,
}

impl Move {
    fn check_invariant(&self) {
        assert!(self.accelerate_mm + self.decelerate_mm <= self.len_mm);
        assert!(self.speed_mm_sq >= self.entry_speed_mm_sq);
        assert!(self.speed_mm_sq >= self.exit_speed_mm_sq);
        assert!(self.unit_vec.mag().abs_diff(1.to_fixed::<U20F12>()) < 0.001.to_fixed::<U20F12>());
    }

    fn reverse_pass_kernel(&mut self) {
        self.check_invariant();

        use PlannerError::*;

        if self.speed_mm_sq == U20F12::ZERO {
            warn!("reverse_pass_kernel: speed_mm_sq is zero");
        }
        // TODO convincing explanation here
        //
        //
        //
        // (self.speed_mm_sq - self.entry_speed_mm_sq) / (2 * self.acceleration_mms2)
        let accelerate_mm = self
            .speed_mm_sq
            .checked_sub(self.entry_speed_mm_sq)
            .ok_or(ArithmeticError)
            .expect("accelerate_mm")
            / (2.to_fixed::<U20F12>() * self.acceleration_mms2);

        // (self.speed_mm_sq - self.exit_speed_mm_sq) / (2 * self.acceleration_mms2)
        let decelerate_mm = (self.speed_mm_sq.checked_sub(self.exit_speed_mm_sq))
            .ok_or(ArithmeticError)
            .expect("decelerate_mm")
            / (2.to_fixed::<U20F12>() * self.acceleration_mms2);

        let speed_change_mm = accelerate_mm + decelerate_mm;
        if self.len_mm < speed_change_mm {
            // try going directly from entry to exit speed
            let direct_mm = accelerate_mm.abs_diff(decelerate_mm);

            if self.len_mm < direct_mm {
                // since exit speed <= cruising speed, and due to the forward pass,
                // this move is able to reach cruising speed,
                // the move cannot slow down enough
                //
                // compute new initial speed to ensure we can slow down
                let new_entry_speed_mm_sq =
                    self.len_mm * 2.to_fixed::<U20F12>() * self.acceleration_mms2
                        + self.exit_speed_mm_sq;
                assert!(new_entry_speed_mm_sq <= self.entry_speed_mm_sq);
                self.accelerate_mm = 0.to_fixed();
                self.decelerate_mm = self.len_mm;
                self.entry_speed_mm_sq = new_entry_speed_mm_sq;
                self.speed_mm_sq = new_entry_speed_mm_sq;
            } else {
                self.accelerate_mm = (direct_mm + self.len_mm) / 2.to_fixed::<U20F12>();
                self.decelerate_mm = self.len_mm - self.accelerate_mm;

                self.speed_mm_sq =
                    self.accelerate_mm * 2.to_fixed::<U20F12>() * self.acceleration_mms2
                        + self.entry_speed_mm_sq;
                self.exit_speed_mm_sq = self
                    .speed_mm_sq
                    .checked_sub(
                        self.decelerate_mm * 2.to_fixed::<U20F12>() * self.acceleration_mms2,
                    )
                    .unwrap_or(U20F12::ZERO);
            }
        } else {
            self.accelerate_mm = accelerate_mm;
            self.decelerate_mm = decelerate_mm;
        }
    }
}

#[derive(Debug)]
pub enum PlannerJob {
    StepperJobRun(Move),
}

impl PlannerJob {
    fn as_move(&self) -> Option<&Move> {
        match self {
            PlannerJob::StepperJobRun(m) => Some(m),
        }
    }

    fn as_move_mut(&mut self) -> Option<&mut Move> {
        match self {
            PlannerJob::StepperJobRun(m) => Some(m),
        }
    }
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

    fn insert_move_job(
        new_move: &PlannerMove,
        prev_move: Option<&Move>,
    ) -> Result<Option<PlannerJob>, PlannerError> {
        // using lazy_static (uses spin) or once_cell (uses critical_section) to
        // read a constant seems brain dead, so here it is, for now, a constant
        let junction_stop_vel_thres: I20F12 = I20F12::from_num(0.99999);
        let platform_junction_deviation: U20F12 = U20F12::from_num(0.015);
        let junction_smoothing_thres: I20F12 = I20F12::from_num(-0.707);

        // move in fixed point mm
        let delta_x = {
            let mut delta_x = [I20F12::ZERO; MAX_AXIS];
            for i in 0..new_move.delta_x.len() {
                delta_x[i] = new_move.delta_x[i].to_fixed();
            }
            delta_x
        };

        // time taken by the slowest axis, longest axis move magnitude, index
        let (time_est, max_axis_len_mm, max_axis) = {
            let mut time_est = U20F12::ZERO;
            let mut max_axis_len_mm = U20F12::ZERO;
            let mut max_axis = 0;
            for (i, x) in delta_x.iter().enumerate() {
                let x_time_est = x.abs() / I20F12::from_num(new_move.max_v[i]);

                time_est = core::cmp::max(time_est, x_time_est.to_fixed::<U20F12>());
                if x.unsigned_abs() > max_axis_len_mm {
                    max_axis_len_mm = x.unsigned_abs();
                    max_axis = i;
                }
            }
            (time_est, max_axis_len_mm, max_axis)
        };

        // empty block
        if time_est == U20F12::ZERO {
            return Ok(None);
        }

        let speed_mm_sq = max_axis_len_mm / time_est;

        let delta_x_vec = FixedVector::new(delta_x.clone());
        let len_mm = delta_x_vec.mag();
        let unit_vec = delta_x_vec.unit();
        // let delta_x = delta_x_vec.inner();

        // acceleration of the block, capped by moving axes
        let acceleration_mms2 = {
            let mut acceleration_mms2: U20F12 = new_move.acc[max_axis].to_fixed();
            // this deviates from the c impl, and opts to not exceed acceleration based on the direction
            delta_x
                .iter()
                .zip(new_move.acc.iter())
                .zip(unit_vec.iter())
                .fold(acceleration_mms2, |y, ((x, acceleration_x), unit_vec_x)| {
                    let acceleration_x = acceleration_x.to_fixed::<U20F12>();
                    if *x == I20F12::ZERO || acceleration_x >= y * unit_vec_x.unsigned_abs() {
                        y
                    } else {
                        core::cmp::min(y, acceleration_x / unit_vec_x.unsigned_abs())
                    }
                })
        };

        let (entry_speed_mm_sq, acceleration_mms2): (U20F12, U20F12) =
            prev_move.map_or((U20F12::ZERO, acceleration_mms2), |prev_move: &Move| {
                let cos_theta = -unit_vec.dot(&prev_move.unit_vec);

                if cos_theta > junction_stop_vel_thres {
                    // FIXME this may be an okay min_speed, but not ideal
                    // min_speed here should respect the min speed of each axis
                    let min_speed: U20F12 = new_move.min_v[max_axis].to_fixed();
                    (min_speed * min_speed, acceleration_mms2)
                } else {
                    let junction_unit_vec: FixedVector<_, MAX_AXIS> =
                        unit_vec.sub(&prev_move.unit_vec).unit();

                    let acceleration_mms2 = {
                        let mut acceleration_mms2: U20F12 = new_move.acc[max_axis].to_fixed();
                        // this deviates from the c impl, and opts to not exceed acceleration based on the direction
                        delta_x
                            .iter()
                            .zip(new_move.acc.iter())
                            .zip(junction_unit_vec.iter())
                            .fold(acceleration_mms2, |y, ((x, acceleration_x), unit_vec_x)| {
                                let acceleration_x = acceleration_x.to_fixed::<U20F12>();
                                if *x == I20F12::ZERO
                                    || acceleration_x >= y * unit_vec_x.unsigned_abs()
                                {
                                    y
                                } else {
                                    core::cmp::min(y, acceleration_x / unit_vec_x.unsigned_abs())
                                }
                            })
                    };

                    // skipped over a jacc_mm calculation here, convinced myself that it was dead code

                    // avoid div by 0 for oneMinusSinThetaDiv2f
                    let cos_theta = if cos_theta < -0.999.to_fixed::<I20F12>() {
                        -0.999.to_fixed::<I20F12>()
                    } else {
                        cos_theta
                    };

                    // number is positive because -1 < cos_theta < 1
                    let sin_theta_div_2: U20F12 = (0.5.to_fixed::<I20F12>()
                        * (1.to_fixed::<I20F12>() - cos_theta))
                        .sqrt()
                        .to_fixed::<U20F12>();
                    let one_minus_sin_theta_div_2: U20F12 =
                        1.to_fixed::<U20F12>() - sin_theta_div_2;
                    let entry_speed_mm_sq: U20F12 =
                        platform_junction_deviation * sin_theta_div_2 * acceleration_mms2
                            / one_minus_sin_theta_div_2;

                    // skipped over small segment handling code
                    (
                        core::cmp::min(
                            speed_mm_sq,
                            core::cmp::min(entry_speed_mm_sq, prev_move.exit_speed_mm_sq),
                        ),
                        acceleration_mms2,
                    )
                }
            });

        let accelerate_mm = speed_mm_sq
            .checked_sub(entry_speed_mm_sq)
            .ok_or(PlannerError::ArithmeticError)?
            / (2.to_fixed::<U20F12>() * acceleration_mms2);

        let speed_mm_sq = if len_mm < accelerate_mm {
            len_mm * 2.to_fixed::<U20F12>() * acceleration_mms2 + entry_speed_mm_sq
        } else {
            speed_mm_sq
        };
        let exit_speed_mm_sq = speed_mm_sq;

        Ok(Some(PlannerJob::StepperJobRun(Move {
            delta_x_steps: new_move.motor_steps.clone(),
            max_axis,
            accelerate_mm,
            decelerate_mm: 0.to_fixed::<U20F12>(),
            len_mm,
            unit_vec,
            acceleration_mms2,
            entry_speed_mm_sq,
            speed_mm_sq,
            exit_speed_mm_sq,
        })))
    }

    fn reverse_pass_kernel(next: &Move, prev: &mut Move) -> bool {
        let vel_change_threshold = 10.to_fixed::<U20F12>();

        if prev.exit_speed_mm_sq.abs_diff(next.entry_speed_mm_sq) >= vel_change_threshold {
            prev.exit_speed_mm_sq = next.entry_speed_mm_sq;
            prev.reverse_pass_kernel();
            true
        } else {
            false
        }
    }

    fn reverse_pass(&mut self) {
        for (next, prev) in self
            .job_queue
            .iter()
            .rev()
            .filter(|x| x.borrow().as_move().is_some())
            .tuple_windows()
        {
            if !Self::reverse_pass_kernel(
                next.borrow().as_move().unwrap(),
                prev.borrow_mut().as_move_mut().unwrap(),
            ) {
                return;
            }
        }
    }

    pub fn enqueue_move(&mut self, new_move: &PlannerMove) -> Result<(), PlannerError> {
        // unfortunately we cannot reserve a slot in the job queue
        if self.job_queue.is_full() {
            return Err(PlannerError::CapacityError);
        }

        // let job = self
        //     .job_queue
        //     .iter()
        //     .rev()
        //     .find_map(|m| {
        //         m.borrow()
        //             .as_move()
        //             .map(|prev_move| Self::insert_move_job(new_move, Some(prev_move)))
        //     })
        //     .unwrap_or_else(|| Self::insert_move_job(new_move, None));

        let prev_move = self
            .job_queue
            .iter()
            .rev()
            .find(|m| m.borrow().as_move().is_some());
        let job = if let Some(prev_move) = prev_move {
            Self::insert_move_job(new_move, Some(prev_move.borrow().as_move().unwrap()))
        } else {
            Self::insert_move_job(new_move, None)
        }?;

        if let Some(job) = job {
            self.job_queue
                .push_back(RefCell::new(job))
                .map_err(|_| PlannerError::CapacityError)
                .unwrap();
            self.reverse_pass();
        }
        Ok(())
    }
}
