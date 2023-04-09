pub mod fixed_vector;
pub mod kinematic;

pub use kinematic::KinematicKind;

use itertools::Itertools;

use core::{cell::RefCell, fmt::Display, result::Result};

use crate::{MAX_AXIS, MAX_STEPPERS};
use arraydeque::{ArrayDeque, Saturating};
use fixed::{
    traits::{ToFixed, FixedEquiv},
    types::{I16F16, I20F12, U20F12},
};
use fixed_sqrt::FixedSqrt;
use log::{info, warn};

use fixed_vector::FixedVector;

#[derive(Debug)]
pub enum PlannerError {
    CapacityError,
}

impl Display for PlannerError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        write!(f, "{:?}", self)
    }
}

#[derive(Debug)]
pub struct Planner {
    kinematic_kind: KinematicKind,
    num_axis: u8,
    num_stepper: u8,
    pub steps_per_mm: [U20F12; MAX_AXIS],

    job_queue: ArrayDeque<RefCell<PlannerJob>, 8, Saturating>,
}

#[repr(u32)]
#[derive(Debug, PartialEq, Eq)]
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

#[derive(Debug)]
#[repr(C)]
pub struct MoveSteps {
    pub delta_x_steps: [u32; MAX_STEPPERS],
    pub step_dirs: u32,

    pub max_axis: u32,
    pub accelerate_until: u32,
    pub decelerate_after: u32,

    pub acceleration_stepss2: u32,

    pub entry_steps_s: u32,
    pub exit_steps_s: u32,
    pub cruise_steps_s: u32,
}

#[derive(Debug, Clone)]
#[repr(C)]
pub struct SyncJob {
    pub notify: *const u8,
    pub seq: u32,
}

#[repr(C)]
pub union ExecutorUnion {
    pub move_steps: core::mem::ManuallyDrop<MoveSteps>,
    pub sync: core::mem::ManuallyDrop<SyncJob>,
    pub unit: (),
}

#[repr(C)]
pub struct ExecutorJob {
    pub job_type: JobType,
    pub data: ExecutorUnion,
}

impl Move {
    fn check_invariant(&self) {
        if !(self.accelerate_mm + self.decelerate_mm <= self.len_mm) {
            log::error!("{:#?}", self);
            panic!();
        }
        assert!(self.speed_mm_sq >= self.entry_speed_mm_sq);
        assert!(self.speed_mm_sq >= self.exit_speed_mm_sq);
        let uv_mag = self.unit_vec.mag();
        let stmt = uv_mag.abs_diff(1.to_fixed::<U20F12>()) < 0.01.to_fixed::<U20F12>();
        if !stmt {
            log::error!("{:?} [{:?}]", uv_mag, self.unit_vec);
        }
        assert!(stmt);
    }

    fn reverse_pass_kernel(&mut self) {
        if self.speed_mm_sq == U20F12::ZERO {
            panic!("reverse_pass_kernel: speed_mm_sq is zero");
        }
        // TODO convincing explanation here
        //
        //
        //
        // (self.speed_mm_sq - self.entry_speed_mm_sq) / (2 * self.acceleration_mms2)
        let accelerate_mm = self
            .speed_mm_sq
            .checked_sub(self.entry_speed_mm_sq)
            .expect("accelerate_mm")
            / (2.to_fixed::<U20F12>() * self.acceleration_mms2);

        // (self.speed_mm_sq - self.exit_speed_mm_sq) / (2 * self.acceleration_mms2)
        let decelerate_mm = (self.speed_mm_sq.checked_sub(self.exit_speed_mm_sq))
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
        self.check_invariant();
    }
}

#[derive(Debug)]
pub enum PlannerJob {
    StepperJobRun(Move),
    StepperJobHomeX(Move),
    StepperJobHomeY(Move),
    StepperJobHomeZ(Move),

    StepperJobEnableSteppers,
    StepperJobDisableSteppers,
    StepperJobSync(SyncJob),
}

impl PlannerJob {
    pub fn as_move(&self) -> Option<&Move> {
        match self {
            PlannerJob::StepperJobRun(m)
            | PlannerJob::StepperJobHomeX(m)
            | PlannerJob::StepperJobHomeY(m)
            | PlannerJob::StepperJobHomeZ(m) => Some(m),
            _ => None,
        }
    }

    pub fn as_move_mut(&mut self) -> Option<&mut Move> {
        match self {
            PlannerJob::StepperJobRun(m)
            | PlannerJob::StepperJobHomeX(m)
            | PlannerJob::StepperJobHomeY(m)
            | PlannerJob::StepperJobHomeZ(m) => Some(m),
            _ => None,
        }
    }
}

#[repr(C)]
#[derive(Debug)]
pub struct PlannerMove {
    pub job_type: JobType,
    /// Movement in "steps".
    /// BEFORE applying kinematics euqation.
    pub motor_steps: [i32; MAX_STEPPERS],
    /// Position change of the *carriage* in *mm*
    pub delta_x: [I16F16; MAX_AXIS],
    /// Min velocity on each axis for the *carriage* in *mm*
    pub min_v: [I16F16; MAX_AXIS],
    /// Max velocity on each axis for the *carriage* in *mm*
    pub max_v: [I16F16; MAX_AXIS],
    /// Acceleration on each axis for the *carriage* in *mm*
    pub acc: [I16F16; MAX_AXIS],
    /// Should the carriage come to a complete stop.
    pub stop: bool,
}

impl PlannerMove {
    fn check_invariant(&self) {
        assert_ne!(self.job_type, JobType::StepperJobUndef);
    }
}

impl Planner {
    pub fn new(num_axis: u32, num_stepper: u32, kinematic_kind: KinematicKind) -> Self {
        Planner {
            kinematic_kind,
            num_axis: num_axis as u8,
            num_stepper: num_stepper as u8,
            steps_per_mm: [U20F12::from_num(0); MAX_AXIS],
            job_queue: ArrayDeque::new(),
        }
    }

    fn construct_move(&self, new_move: &PlannerMove, prev_move: Option<&Move>) -> Option<Move> {
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
        
        let mut motor_steps: [i32; MAX_STEPPERS] = [0i32; MAX_STEPPERS];
        self.kinematic_kind.plan(&new_move.motor_steps, &mut motor_steps);


        // time taken by the slowest axis, longest axis move magnitude, index
        let (time_est, max_axis) = {
            let mut time_est = U20F12::ZERO;
            let mut max_axis_len_mm = U20F12::ZERO;
            let mut max_axis = 0;
            for (i, x) in delta_x.iter().enumerate() {
                if *x == I20F12::ZERO {
                    continue;
                }
                let x_time_est = x.abs() / I20F12::from_num(new_move.max_v[i]);

                time_est = core::cmp::max(time_est, x_time_est.to_fixed::<U20F12>());
                if x.unsigned_abs() > max_axis_len_mm {
                    max_axis_len_mm = x.unsigned_abs();
                    max_axis = i;
                }
            }
            (time_est, max_axis)
        };

        // empty block
        if time_est == U20F12::ZERO {
            return None;
        }

        let delta_x_vec = FixedVector::new(delta_x.clone());
        let len_mm = delta_x_vec.mag();
        let unit_vec = delta_x_vec.unit();

        let speed_mm_sq = {
            let a = len_mm / time_est;
            a * a
        };
        // let delta_x = delta_x_vec.inner();

        // acceleration of the block, capped by moving axes
        let acceleration_mms2 = {
            let acceleration_mms2: U20F12 = new_move.acc[max_axis].to_fixed();
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
                        // this deviates from the c impl, and opts to not exceed acceleration based on the direction
                        delta_x
                            .iter()
                            .zip(new_move.acc.iter())
                            .zip(junction_unit_vec.iter())
                            .fold(
                                acceleration_mms2,
                                |accum, ((x, acceleration_x), junc_unit_vec_axis)| {
                                    let acceleration_x = acceleration_x.to_fixed::<U20F12>();
                                    if *x == I20F12::ZERO
                                        || acceleration_x
                                            >= accum * junc_unit_vec_axis.unsigned_abs()
                                    {
                                        accum
                                    } else {
                                        core::cmp::min(
                                            accum,
                                            acceleration_x / junc_unit_vec_axis.unsigned_abs(),
                                        )
                                    }
                                },
                            )
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
            .expect("accelerate_mm")
            / (2.to_fixed::<U20F12>() * acceleration_mms2);

        let speed_mm_sq = if len_mm < accelerate_mm {
            len_mm * 2.to_fixed::<U20F12>() * acceleration_mms2 + entry_speed_mm_sq
        } else {
            speed_mm_sq
        };

        let exit_speed_mm_sq = if new_move.stop {
            U20F12::ZERO
        } else {
            speed_mm_sq
        };

        let mut mov = Move {
            delta_x_steps: motor_steps, 
            max_axis,
            accelerate_mm,
            decelerate_mm: 0.to_fixed::<U20F12>(),
            len_mm,
            unit_vec,
            acceleration_mms2,
            entry_speed_mm_sq,
            speed_mm_sq,
            exit_speed_mm_sq,
        };

        if new_move.stop {
            mov.reverse_pass_kernel();
        }

        Some(mov)
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

    pub fn dequeue_move(&mut self) -> Option<ExecutorJob> {
        self.dequeue_move_test_only().map(|a| a.0)
    }

    fn move_to_move_steps(&self, mov: &Move) -> core::mem::ManuallyDrop<MoveSteps> {
        let max_stepper = mov.delta_x_steps.iter().enumerate().fold(
            (0usize, 0u32),
            |(max_axis, max_steps), (axis, steps)| {
                if steps.unsigned_abs() > max_steps {
                    (axis, steps.unsigned_abs())
                } else {
                    (max_axis, max_steps)
                }
            },
        ).0;
        let max_axis_proj = mov.unit_vec[max_stepper].unsigned_abs();
        let accelerate_steps = max_axis_proj * mov.accelerate_mm * self.steps_per_mm[max_stepper];
        let decelerate_steps = max_axis_proj * mov.decelerate_mm * self.steps_per_mm[max_stepper];

        let acceleration_stepss2 =
            (max_axis_proj * mov.acceleration_mms2 * self.steps_per_mm[max_stepper])
                .to_num::<u32>();

        let max_axis_delta_x = mov.delta_x_steps[max_stepper]
            .unsigned_abs()
            .to_fixed::<U20F12>();
        let (accelerate_steps, decelerate_steps) =
            if accelerate_steps + decelerate_steps > max_axis_delta_x {
                let scaling_factor = max_axis_delta_x / (accelerate_steps + decelerate_steps);
                (
                    accelerate_steps * scaling_factor,
                    decelerate_steps * scaling_factor,
                )
            } else {
                (accelerate_steps, decelerate_steps)
            };

        let mut step_dirs: u32 = 0;
        for i in 0..self.num_stepper {
            if mov.delta_x_steps[i as usize] > 0 {
                step_dirs |= 1 << i;
            }
        }

        let mut delta_x_steps: [u32; MAX_STEPPERS] = [0u32; MAX_STEPPERS];
        delta_x_steps
            .iter_mut()
            .zip(mov.delta_x_steps.iter())
            .for_each(|(unsigned, signed)| *unsigned = signed.unsigned_abs());

        let move_steps = MoveSteps {
            delta_x_steps,
            step_dirs,
            max_axis: max_stepper as u32,
            accelerate_until: accelerate_steps.to_num::<u32>(),
            decelerate_after: delta_x_steps[max_stepper] - decelerate_steps.to_num::<u32>(),
            acceleration_stepss2,

            entry_steps_s: (mov.entry_speed_mm_sq.sqrt()
                * max_axis_proj
                * self.steps_per_mm[max_stepper])
                .to_num::<u32>(),
            cruise_steps_s: (mov.speed_mm_sq.sqrt()
                * max_axis_proj
                * self.steps_per_mm[max_stepper])
                .to_num::<u32>(),
            exit_steps_s: (mov.exit_speed_mm_sq.sqrt()
                * max_axis_proj
                * self.steps_per_mm[max_stepper])
                .to_num::<u32>(),
        };

        info!("{:?}", move_steps);

        core::mem::ManuallyDrop::new(move_steps)
    }

    pub fn dequeue_move_test_only(&mut self) -> Option<(ExecutorJob, PlannerJob)> {
        if let Some(job) = self.job_queue.pop_front().map(|j| j.into_inner()) {
            let executor_job = match &job {
                PlannerJob::StepperJobRun(mov) => ExecutorJob {
                    job_type: JobType::StepperJobRun,
                    data: ExecutorUnion {
                        move_steps: self.move_to_move_steps(mov),
                    },
                },
                PlannerJob::StepperJobHomeX(mov) => ExecutorJob {
                    job_type: JobType::StepperJobHomeX,
                    data: ExecutorUnion {
                        move_steps: self.move_to_move_steps(mov),
                    },
                },
                PlannerJob::StepperJobHomeY(mov) => ExecutorJob {
                    job_type: JobType::StepperJobHomeY,
                    data: ExecutorUnion {
                        move_steps: self.move_to_move_steps(mov),
                    },
                },
                PlannerJob::StepperJobHomeZ(mov) => ExecutorJob {
                    job_type: JobType::StepperJobHomeZ,
                    data: ExecutorUnion {
                        move_steps: self.move_to_move_steps(mov),
                    },
                },
                PlannerJob::StepperJobEnableSteppers => ExecutorJob {
                    job_type: JobType::StepperJobEnableSteppers,
                    data: ExecutorUnion { unit: () },
                },
                PlannerJob::StepperJobDisableSteppers => ExecutorJob {
                    job_type: JobType::StepperJobDisableSteppers,
                    data: ExecutorUnion { unit: () },
                },
                PlannerJob::StepperJobSync(sync) => ExecutorJob {
                    job_type: JobType::StepperJobSync,
                    data: ExecutorUnion {
                        sync: core::mem::ManuallyDrop::new(sync.clone()),
                    },
                },
            };
            Some((executor_job, job))
        } else {
            None
        }
    }

    fn construct_move_job(&self, new_move: &PlannerMove) -> Option<Move> {
        let prev_move = self
            .job_queue
            .iter()
            .rev()
            .find(|m| m.borrow().as_move().is_some());

        if let Some(prev_move) = prev_move {
            self.construct_move(new_move, Some(prev_move.borrow().as_move().unwrap()))
        } else {
            self.construct_move(new_move, None)
        }
    }

    pub fn enqueue_sync(&mut self, sync: &SyncJob) -> Result<(), PlannerError> {
        self.job_queue
            .push_back(RefCell::new(PlannerJob::StepperJobSync(sync.clone())))
            .map_err(|_| PlannerError::CapacityError)
    }

    pub fn enqueue_other(&mut self, job_type: JobType) -> Result<(), PlannerError> {
        let job = match job_type {
            JobType::StepperJobEnableSteppers => PlannerJob::StepperJobEnableSteppers,
            JobType::StepperJobDisableSteppers => PlannerJob::StepperJobDisableSteppers,
            JobType::StepperJobUndef
            | JobType::StepperJobRun
            | JobType::StepperJobHomeX
            | JobType::StepperJobHomeY
            | JobType::StepperJobHomeZ
            | JobType::StepperJobSync => panic!(),
        };

        self.job_queue
            .push_back(RefCell::new(job))
            .map_err(|_| PlannerError::CapacityError)
    }

    pub fn enqueue_move(&mut self, new_move: &PlannerMove) -> Result<(), PlannerError> {
        // unfortunately we cannot reserve a slot in the job queue
        if self.job_queue.is_full() {
            return Err(PlannerError::CapacityError);
        }

        new_move.check_invariant();
        info!("new_move {:?}", new_move);

        let job = match new_move.job_type {
            JobType::StepperJobUndef => panic!(),
            JobType::StepperJobRun => self
                .construct_move_job(new_move)
                .map(PlannerJob::StepperJobRun),
            JobType::StepperJobHomeX => self
                .construct_move_job(new_move)
                .map(PlannerJob::StepperJobHomeX),
            JobType::StepperJobHomeY => self
                .construct_move_job(new_move)
                .map(PlannerJob::StepperJobHomeY),
            JobType::StepperJobHomeZ => self
                .construct_move_job(new_move)
                .map(PlannerJob::StepperJobHomeZ),
            JobType::StepperJobEnableSteppers
            | JobType::StepperJobDisableSteppers
            | JobType::StepperJobSync => panic!(),
        };

        info!("enqueue_move {:?}", job);

        if let Some(job) = job {
            self.job_queue
                .push_back(RefCell::new(job))
                .map_err(|_| PlannerError::CapacityError)
                .expect("Qnospace");
            self.reverse_pass();
        }
        Ok(())
    }

    pub fn is_empty(&self) -> bool {
        self.job_queue.is_empty()
    }

    pub fn free_capacity(&self) -> u32 {
        (self.job_queue.capacity() - self.job_queue.len()) as u32
    }
}
