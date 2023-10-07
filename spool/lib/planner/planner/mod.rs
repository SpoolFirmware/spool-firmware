pub mod fixed_vector;
pub mod kinematic;

pub use kinematic::KinematicKind;

use itertools::Itertools;

use core::{cell::RefCell, fmt::Display, result::Result};

use crate::{MAX_AXIS, MAX_STEPPERS};
use arraydeque::{ArrayDeque, Saturating};
use fixed::{
    traits::{Fixed, FixedEquiv, FixedSigned, ToFixed},
    types::{I16F16, I20F12, U20F12},
};
use fixed_sqrt::FixedSqrt;
use log::{trace, warn};

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

    last_dequeued: Option<RefCell<PlannerJob>>,
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

#[derive(Debug, Clone)]
pub struct Move {
    // move for steppers, after kinematics
    pub move_steps: [i32; MAX_STEPPERS],
    // move for steppers, after kinematics
    pub move_stepper_mm: [I20F12; MAX_AXIS],
    // axis with the most step events
    pub max_stepper: usize,
    // move_steps[max_stepper] / len_mm
    pub steps_per_mm: U20F12,

    pub accelerate_mm: U20F12,
    pub decelerate_mm: U20F12,

    /// length of the move in mm
    pub len_mm: U20F12,

    // unit vector after kinematics, for mm
    pub unit_vec: FixedVector<I20F12, MAX_AXIS>,

    /// Acceleration (mm/s^2)
    pub acceleration_mms2: U20F12,
    /// (Entry Speed)^2 in (mm/s)^2
    pub entry_speed_mm_sq: U20F12,
    /// Steady State Speed Squared in mm
    pub speed_mm_sq: U20F12,
    /// (Exit Speed) squared in mm
    pub exit_speed_mm_sq: U20F12,

    /// (Max Entry Speed)^2 in (mm/s)^2
    pub max_entry_speed_mm_sq: U20F12,
    pub recalculate: bool,
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
    fn step_event_count(&self) -> u32 {
        self.move_steps[self.max_stepper].unsigned_abs()
    }

    /// sanity checks for the move
    /// the trapezoid sides <= the total length
    /// cruising speed >= entry speed
    /// cruising speed >= entry speed
    /// ||unit vector|| ~= 1
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

    fn calculate_trapezoid(&mut self) {
        if self.speed_mm_sq == U20F12::ZERO {
            panic!("speed_mm_sq is zero");
        }
        // v1 = v0 + at
        // x = v0t + 1/2at^2
        // by simple algebra
        // acceleration_mm = (speed_mm_sq - entry_speed_mm_sq) / (2 * acceleration_mms2)
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
            let direct_mm = accelerate_mm.to_fixed::<I20F12>() - decelerate_mm.to_fixed::<I20F12>();
            log::debug!(
                "len: {}, accelerate_mm: {}, decelerate_mm: {}, direct_mm: {}, exit: {}\n{:#?}",
                self.len_mm,
                accelerate_mm,
                decelerate_mm,
                direct_mm,
                self.exit_speed_mm_sq,
                self
            );

            if self.len_mm < direct_mm.unsigned_abs() {
                // impossible!!! because forward pass ensures next move starts at a possible speed
                // could it be rounding error?
                // unconvinced 20231007
                log::info!("rounding error? len: {}, direct_mm: {}", self.len_mm, direct_mm.unsigned_abs());
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
                self.max_entry_speed_mm_sq = new_entry_speed_mm_sq;
            } else {
                self.accelerate_mm = (direct_mm + self.len_mm.to_fixed::<I20F12>())
                    .to_fixed::<U20F12>()
                    * 0.5.to_fixed::<U20F12>();
                self.decelerate_mm = self.len_mm - self.accelerate_mm;

                self.speed_mm_sq =
                    self.accelerate_mm * 2.to_fixed::<U20F12>() * self.acceleration_mms2
                        + self.entry_speed_mm_sq;

                assert!(self.speed_mm_sq >= self.entry_speed_mm_sq);

                let exit_speed_mm_sq = self
                    .speed_mm_sq
                    .checked_sub(
                        self.decelerate_mm * 2.to_fixed::<U20F12>() * self.acceleration_mms2,
                    )
                    .unwrap_or(U20F12::ZERO);
                if (self.exit_speed_mm_sq.abs_diff(exit_speed_mm_sq) > 10.to_fixed::<U20F12>()) {
                    panic!("{:#?}", self);
                }
            }
        } else {
            self.accelerate_mm = accelerate_mm;
            self.decelerate_mm = decelerate_mm;
        }
        self.check_invariant();
    }
}

#[derive(Debug, Clone)]
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
    pub motor_cart_steps: [i32; MAX_STEPPERS],
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
            last_dequeued: None,
        }
    }

    fn construct_move(&self, new_move: &PlannerMove, prev_move: Option<&Move>) -> Option<Move> {
        // using lazy_static (uses spin) or once_cell (uses critical_section) to
        // read a constant seems brain dead, so here it is, for now, a constant
        let junction_stop_vel_thres: I20F12 = I20F12::from_num(0.99999);
        let platform_junction_deviation: U20F12 = U20F12::from_num(0.01);
        let junction_smoothing_thres: I20F12 = I20F12::from_num(-0.707);

        // move in fixed point mm
        let delta_x_cart = {
            let mut delta_x = [I20F12::ZERO; MAX_AXIS];
            for i in 0..new_move.delta_x.len() {
                delta_x[i] = new_move.delta_x[i].to_fixed();
            }
            delta_x
        };

        let mut move_steps: [i32; MAX_STEPPERS] = [0i32; MAX_STEPPERS];
        self.kinematic_kind
            .plan(&new_move.motor_cart_steps, &mut move_steps);

        let len_mm = self
            .kinematic_kind
            .move_len(&delta_x_cart[..3], &delta_x_cart[3..]);
        let mut move_stepper_mm = [I20F12::ZERO; MAX_AXIS];
        self.kinematic_kind
            .plan(&delta_x_cart, &mut move_stepper_mm);
        let move_stepper_mm = FixedVector::new(move_stepper_mm);
        let unit_vec = move_stepper_mm.unit();
        let move_stepper_mm = move_stepper_mm.inner();

        // time taken by the slowest axis, longest axis move magnitude, index
        let (time, (max_stepper, max_stepper_steps)) = move_steps
            .iter()
            .zip(move_stepper_mm.iter())
            .enumerate()
            .fold(
                (U20F12::ZERO, (0usize, 0u32)),
                |(time, (max_stepper, max_stepper_steps)),
                 (move_stepper_idx, (move_steps, move_stepper_mm))| {
                    if *move_steps == 0 {
                        (time, (max_stepper, max_stepper_steps))
                    } else {
                        let new_move_max_v_stepper =
                            new_move.max_v[move_stepper_idx].to_fixed::<U20F12>();
                        assert_ne!(new_move_max_v_stepper, U20F12::ZERO);
                        let move_steps_abs = move_steps.unsigned_abs();
                        (
                            core::cmp::max(
                                time,
                                move_stepper_mm.unsigned_abs() / new_move_max_v_stepper,
                            ),
                            if move_steps_abs > max_stepper_steps {
                                (move_stepper_idx, move_steps_abs)
                            } else {
                                (max_stepper, max_stepper_steps)
                            },
                        )
                    }
                },
            );

        // empty block
        if time == U20F12::ZERO {
            return None;
        }

        let speed_mm_sq = {
            let a = len_mm / time;
            a * a
        };

        assert_ne!(len_mm, U20F12::ZERO);
        let steps_per_mm = max_stepper_steps.to_fixed::<U20F12>() / len_mm;
        // let delta_x = delta_x_vec.inner();

        // acceleration of the block, capped by moving axes
        let acceleration_mms2 = {
            let acceleration_mms2: U20F12 = new_move.acc[max_stepper].to_fixed();
            // this deviates from the c impl, and opts to not exceed acceleration based on the direction
            move_stepper_mm
                .iter()
                .zip(new_move.acc.iter())
                .zip(unit_vec.iter())
                .fold(acceleration_mms2, |y, ((x, acceleration_x), unit_vec_x)| {
                    let acceleration_x = acceleration_x.to_fixed::<U20F12>();
                    if *x == I20F12::ZERO || acceleration_x >= y * unit_vec_x.unsigned_abs() {
                        y
                    } else {
                        assert_ne!(*unit_vec_x, I20F12::ZERO);
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
                    let min_speed: U20F12 = new_move.min_v[max_stepper].to_fixed();
                    (min_speed * min_speed, acceleration_mms2)
                } else {
                    let junction_unit_vec: FixedVector<_, MAX_AXIS> =
                        unit_vec.sub(&prev_move.unit_vec).unit();

                    let acceleration_mms2 = {
                        // this deviates from the c impl, and opts to not exceed acceleration based on the direction
                        move_stepper_mm
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
                                        assert_ne!(*junc_unit_vec_axis, I20F12::ZERO);
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

                    // small segments
                    let limited_speed_mm_sq = if len_mm < 1.to_fixed::<I20F12>()
                        && cos_theta < junction_smoothing_thres
                    {
                        let theta: I20F12 = cordic::acos(cos_theta);
                        assert_ne!(theta, I20F12::ZERO);
                        let limit_sqr: U20F12 = ((len_mm.to_fixed::<I20F12>() / theta)
                            * acceleration_mms2.to_fixed::<I20F12>())
                        .to_fixed();
                        core::cmp::min(speed_mm_sq, limit_sqr)
                    } else {
                        speed_mm_sq
                    };

                    (
                        core::cmp::min(
                            limited_speed_mm_sq,
                            core::cmp::min(entry_speed_mm_sq, prev_move.speed_mm_sq),
                        ),
                        acceleration_mms2,
                    )
                }
            });

        let accelerate_mm = speed_mm_sq
            .checked_sub(entry_speed_mm_sq)
            .expect("accelerate_mm")
            / (2.to_fixed::<U20F12>() * acceleration_mms2);

        let (speed_mm_sq, accelerate_mm) = if len_mm < accelerate_mm {
            (
                len_mm * 2.to_fixed::<U20F12>() * acceleration_mms2 + entry_speed_mm_sq,
                len_mm,
            )
        } else {
            (speed_mm_sq, accelerate_mm)
        };

        let entry_speed_mm_sq = core::cmp::min(entry_speed_mm_sq, speed_mm_sq);

        let exit_speed_mm_sq = U20F12::ZERO;

        let mut mov = Move {
            move_steps,
            move_stepper_mm,
            max_stepper,
            steps_per_mm,
            accelerate_mm,
            decelerate_mm: 0.to_fixed::<U20F12>(),
            len_mm,
            unit_vec,
            acceleration_mms2,
            entry_speed_mm_sq,
            speed_mm_sq,
            exit_speed_mm_sq,
            max_entry_speed_mm_sq: entry_speed_mm_sq,
            recalculate: true,
        };

        Some(mov)
    }

    fn reverse_pass_kernel(prev: &mut Move, next: &Move) {
        // if prev.entry_speed_mm_sq <= next.entry_speed_mm_sq {
        //     return false;
        // }

        let prev_max_entry = prev
            .max_entry_speed_mm_sq
            .min(next.entry_speed_mm_sq.saturating_add(
                (2.to_fixed::<U20F12>() * prev.acceleration_mms2).saturating_mul(prev.len_mm),
            ));

        if prev_max_entry != prev.entry_speed_mm_sq {
            prev.entry_speed_mm_sq = prev_max_entry;
            prev.recalculate = true;
        }
    }

    /// convinced 20231005
    fn reverse_pass(&mut self) {
        let mut next_iter = self
            .job_queue
            .iter()
            .rev()
            .filter(|x| x.borrow().as_move().is_some());
        let mut prev_iter = self
            .job_queue
            .iter()
            .rev()
            .filter(|x| x.borrow().as_move().is_some());

        prev_iter.next();

        while let Some(prev) = prev_iter.next() {
            if let Some(next) = next_iter.next() {
                Self::reverse_pass_kernel(
                    prev.borrow_mut().as_move_mut().unwrap(),
                    next.borrow().as_move().unwrap(),
                )
            }
        }
    }

    fn forward_pass_kernel(prev: &Move, next: &mut Move) {
        // if next.entry_speed_mm_sq <= prev.entry_speed_mm_sq {
        //     return false;
        // }

        // assuming previous move only accelerates, how fast can it get??????
        let prev_max_exit = prev.entry_speed_mm_sq.saturating_add(
            (2.to_fixed::<U20F12>() * prev.acceleration_mms2).saturating_mul(prev.len_mm),
        );
        if prev_max_exit < next.entry_speed_mm_sq {
            next.entry_speed_mm_sq = prev_max_exit;
            next.recalculate = true;
        }
    }

    fn forward_pass(&mut self) {
        let mut next_iter = self
            .job_queue
            .iter()
            .filter(|x| x.borrow().as_move().is_some());
        let mut prev_iter = self
            .last_dequeued
            .iter()
            .chain(self.job_queue.iter())
            .filter(|x| x.borrow().as_move().is_some());

        if self.last_dequeued.is_none() {
            next_iter.next();
        }

        while let Some(prev) = prev_iter.next() {
            if let Some(next) = next_iter.next() {
                Self::forward_pass_kernel(
                    prev.borrow().as_move().unwrap(),
                    next.borrow_mut().as_move_mut().unwrap(),
                )
            }
        }
    }

    fn recalculate(&mut self) {
        let mut next_iter = self
            .job_queue
            .iter()
            .filter(|x| x.borrow().as_move().is_some());
        let mut prev_iter = self
            .last_dequeued
            .iter()
            .chain(self.job_queue.iter())
            .filter(|x| x.borrow().as_move().is_some());

        if self.last_dequeued.is_none() {
            next_iter.next();
        }

        while let Some(prev) = prev_iter.next() {
            if let Some(next) = next_iter.next() {
                let mut prev = prev.borrow_mut();
                let mut prev = prev.as_move_mut().unwrap();
                let next = next.borrow();
                let next = next.as_move().unwrap();

                if prev.exit_speed_mm_sq != next.entry_speed_mm_sq {
                    prev.exit_speed_mm_sq = next.entry_speed_mm_sq;
                }

                if prev.recalculate || next.recalculate {
                    prev.calculate_trapezoid();
                    prev.recalculate = false;
                }
            } else {
                let mut prev = prev.borrow_mut();
                let mut prev = prev.as_move_mut().unwrap();

                prev.calculate_trapezoid();
                prev.recalculate = false;
            }
        }
    }

    pub fn dequeue_move(&mut self) -> Option<ExecutorJob> {
        self.dequeue_move_test_only().map(|a| a.0)
    }

    fn move_to_move_steps(&self, mov: &Move) -> core::mem::ManuallyDrop<MoveSteps> {
        assert!(!mov.recalculate);

        let accelerate_steps = mov.accelerate_mm * mov.steps_per_mm;
        let decelerate_steps = mov.decelerate_mm * mov.steps_per_mm;
        let acceleration_stepss2 = (mov.acceleration_mms2 * mov.steps_per_mm).to_num::<u32>();
        let max_stepper_steps = mov.move_steps[mov.max_stepper]
            .unsigned_abs()
            .to_fixed::<U20F12>();
        let (accelerate_steps, decelerate_steps) =
            if accelerate_steps + decelerate_steps > max_stepper_steps {
                let scaling_factor = max_stepper_steps / (accelerate_steps + decelerate_steps);
                (
                    accelerate_steps * scaling_factor,
                    decelerate_steps * scaling_factor,
                )
            } else {
                (accelerate_steps, decelerate_steps)
            };

        let mut step_dirs: u32 = 0;
        for i in 0..self.num_stepper {
            if mov.move_steps[i as usize] > 0 {
                step_dirs |= 1 << i;
            }
        }

        let mut delta_x_steps: [u32; MAX_STEPPERS] = [0u32; MAX_STEPPERS];
        delta_x_steps
            .iter_mut()
            .zip(mov.move_steps.iter())
            .for_each(|(unsigned, signed)| *unsigned = signed.unsigned_abs());

        let move_steps = MoveSteps {
            delta_x_steps,
            step_dirs,
            max_axis: mov.max_stepper as u32,
            accelerate_until: accelerate_steps.to_num::<u32>(),
            decelerate_after: delta_x_steps[mov.max_stepper] - decelerate_steps.to_num::<u32>(),
            acceleration_stepss2,

            entry_steps_s: (mov.entry_speed_mm_sq.sqrt() * mov.steps_per_mm).to_num::<u32>(),
            cruise_steps_s: (mov.speed_mm_sq.sqrt() * mov.steps_per_mm).to_num::<u32>(),
            exit_steps_s: (mov.exit_speed_mm_sq.sqrt() * mov.steps_per_mm).to_num::<u32>(),
        };

        trace!("{:?}", move_steps);

        core::mem::ManuallyDrop::new(move_steps)
    }

    fn set_last_dequeued(&mut self, mov: &Move) {
        if self.is_empty() {
            self.last_dequeued = None;
        } else {
            self.last_dequeued
                .replace(RefCell::new(PlannerJob::StepperJobRun(mov.clone())));
        }
    }

    pub fn dequeue_move_test_only(&mut self) -> Option<(ExecutorJob, PlannerJob)> {
        if let Some(job) = self.job_queue.pop_front().map(|j| j.into_inner()) {
            let executor_job = match &job {
                PlannerJob::StepperJobRun(mov) => {
                    self.set_last_dequeued(mov);
                    ExecutorJob {
                        job_type: JobType::StepperJobRun,
                        data: ExecutorUnion {
                            move_steps: self.move_to_move_steps(mov),
                        },
                    }
                }
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
            .last_dequeued
            .iter()
            .chain(self.job_queue.iter())
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
        trace!("new_move {:?}", new_move);

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

        trace!("enqueue_move {:?}", job);

        if let Some(job) = job {
            self.job_queue
                .push_back(RefCell::new(job))
                .map_err(|_| PlannerError::CapacityError)
                .expect("Qnospace");
            self.reverse_pass();
            self.forward_pass();
            self.recalculate();
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
