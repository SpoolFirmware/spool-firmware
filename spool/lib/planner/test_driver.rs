use clap::{Arg, Command};
use fixed::traits::ToFixed;
use gcode::Mnemonic;
use log::{debug, error, info, trace, warn};
use std::boxed::Box;
use std::{fmt::Display, fs::File, io::Write, path::PathBuf};

use fixed::types::{I16F16, U20F12};
use log::{Log, Metadata, Record};
use planner::planner::{JobType, Move, MoveSteps, Planner, PlannerError, PlannerJob, PlannerMove, KinematicKind};
use std::cmp::min;

struct YomamaLogger;

impl log::Log for YomamaLogger {
    fn enabled(&self, _metadata: &Metadata) -> bool {
        true
    }

    fn log(&self, record: &Record) {
        if self.enabled(record.metadata()) {
            println!(
                "[{:?}][{}]: {}",
                &record.level(),
                record.target(),
                record.args()
            );
        }
    }

    fn flush(&self) {
        std::io::stdout().flush().unwrap();
    }
}

static YOMAMA: YomamaLogger = YomamaLogger;

struct MachineState {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub e: f32,
    pub fr: f32,
    pub abs: bool,
    pub e_abs: bool,
}

impl Display for MachineState {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> Result<(), std::fmt::Error> {
        write!(
            f,
            "MState(xyze = {}, {}, {}, {}, abs: {} eabs: {})",
            self.x, self.y, self.z, self.e, self.abs, self.e_abs
        )
    }
}

impl Default for MachineState {
    fn default() -> Self {
        MachineState {
            x: 0.0,
            y: 0.0,
            z: 0.0,
            e: 0.0,
            fr: 100.0 * 60.0,
            abs: true,
            e_abs: true,
        }
    }
}

macro_rules! delta_axis {
    ($self:ident, $gcode: ident, $letter: tt, $axis: tt, $abs_cond: expr) => {
        match $gcode.value_for($letter) {
            Some($axis) => {
                if $abs_cond {
                    $axis - $self.$axis
                } else {
                    $axis
                }
            }
            None => 0.0,
        }
    };
}

const STEPS_PER_MM: [f32; 4] = [80.0, 80.0, 400.0, 800.0];

impl MachineState {
    fn process_move(&mut self, m: MoveSteps) {}

    fn process_gcode(&mut self, gcode: &gcode::GCode) -> Option<PlannerMove> {
        match gcode.mnemonic() {
            Mnemonic::General => match gcode.major_number() {
                0 | 1 => {
                    let mut new_xyze = ();
                    let delta_e = delta_axis!(self, gcode, 'E', e, self.abs || self.e_abs);
                    let delta_x = delta_axis!(self, gcode, 'X', x, self.abs);
                    let delta_y = delta_axis!(self, gcode, 'Y', y, self.abs);
                    let delta_z = delta_axis!(self, gcode, 'Z', z, self.abs);
                    let xyze = [delta_x, delta_y, delta_z, delta_e];
                    let xyze_steps: Vec<_> = xyze
                        .iter()
                        .zip(STEPS_PER_MM.iter())
                        .map(|(mm, step)| (mm * step) as i32)
                        .collect();
                    if let Some(f) = gcode.value_for('F') {
                        self.fr = f / 60.0;
                    }
                    self.x += xyze[0];
                    self.y += xyze[1];
                    self.z += xyze[2];
                    self.e += xyze[3];
                    let fr = self.fr as u32;
                    return Some(PlannerMove {
                        job_type: JobType::StepperJobRun,
                        motor_steps: *(Box::<[i32; 4]>::try_from(xyze_steps).unwrap()),
                        delta_x: xyze.map(|x| x.to_fixed()),
                        max_v: [
                            min(fr, 200).to_fixed::<I16F16>(),
                            min(fr, 200).to_fixed::<I16F16>(),
                            min(fr, 5).to_fixed::<I16F16>(),
                            min(fr, 40).to_fixed::<I16F16>(),
                        ],
                        min_v: [
                            0.1.to_fixed(),
                            0.1.to_fixed(),
                            0.1.to_fixed(),
                            0.1.to_fixed(),
                        ],
                        acc: [
                            500.to_fixed::<I16F16>(),
                            500.to_fixed::<I16F16>(),
                            100.to_fixed::<I16F16>(),
                            1000.to_fixed::<I16F16>(),
                        ],
                        stop: false,
                    });
                }
                28 => {
                    *self = Self::default();
                }
                90 => {
                    self.abs = true;
                    self.e_abs = true;
                }
                91 => {
                    self.abs = false;
                }
                92 => {
                    if let Some(e) = gcode.value_for('E') {
                        self.e = e;
                    }
                    if let Some(x) = gcode.value_for('X') {
                        self.x = x;
                    }
                    if let Some(y) = gcode.value_for('Y') {
                        self.y = y;
                    }
                    if let Some(z) = gcode.value_for('Z') {
                        self.z = z;
                    }
                }
                // NOPS
                29 => {}
                _ => {
                    panic!("Unsupported G: {}", gcode);
                }
            },
            Mnemonic::Miscellaneous => match gcode.major_number() {
                82 => {
                    self.e_abs = true;
                }
                83 => {
                    self.e_abs = false;
                }
                // NOPs
                140 | 104 | 109 | 190 | 106 | 107 => {}
                _ => {
                    panic!("Unsupported M: {}", gcode);
                }
            },
            _ => {
                panic!("Unsupported Type: {}", gcode);
            }
        }
        None
    }
}

fn assert_kinematic_eq(
    x_steps: u32,
    steps_per_mm: u32,
    initial_speed: U20F12,
    acceleration_mms2: U20F12,
    final_speed: U20F12,
) {
    let x = x_steps as f32 / steps_per_mm as f32;
}

fn main() {
    log::set_logger(&YOMAMA);
    log::set_max_level(log::LevelFilter::Trace);

    let matches = Command::new("GCodeDriver")
        .version("1.0")
        .arg(Arg::new("file").required(true))
        .get_matches();
    let file_path = matches.get_one::<String>("file").expect("Must have a file");
    let file_path = PathBuf::from(file_path);
    info!("Trying file {:?}", &file_path);

    let mut planner = Planner::new(4, 4, KinematicKind::CoreXY);
    planner.steps_per_mm = [
        80.to_fixed::<U20F12>(),
        80.to_fixed::<U20F12>(),
        400.to_fixed::<U20F12>(),
        800.to_fixed::<U20F12>(),
    ];

    debug!("Init Planner {:#?}", planner);

    let mut machine_state = MachineState::default();
    info!("Init State: {}", &machine_state);

    let buffer = std::fs::read(&file_path).expect("File no existo?");
    let string = String::from_utf8(buffer).expect("bad file content");
    let gcodes: Vec<_> = gcode::parse(&string).collect();

    let mut last_move: Option<(MoveSteps, PlannerJob)> = None;

    for (i, gcode) in gcodes.iter().enumerate() {
        info!("Input[{:04}]: {}", i, gcode);
        if let Some(mut planner_move) = machine_state.process_gcode(&gcode) {
            if i == gcodes.len() - 1 {
                planner_move.stop = true;
            }
            trace!("Move: {:#?}", &planner_move);
            loop {
                match planner.enqueue_move(&planner_move) {
                    Ok(_) => break,
                    Err(PlannerError::CapacityError) => {
                        let (executor_job, planner_job) = planner.dequeue_move_test_only().unwrap();
                        assert_eq!(executor_job.job_type, JobType::StepperJobRun);
                        let res = core::mem::ManuallyDrop::into_inner(unsafe { executor_job.data.move_steps });

                        info!("[EXEC] {:?}", res);
                        if let Some(last_move_) = last_move {
                            let vel_change_threshold = 10.to_fixed::<U20F12>();
                            // invariant check against last move
                            if !last_move_
                                .1.as_move().unwrap()
                                .exit_speed_mm_sq
                                .abs_diff(planner_job.as_move().unwrap().entry_speed_mm_sq)
                                <= vel_change_threshold
                            {
                                panic!("[LAST_MOVE] {:#?}", last_move_);
                            }

                            last_move = Some((res, planner_job))
                        } else {
                            last_move = Some((res, planner_job))
                        }
                    }
                    Err(_) => {
                        panic!("skill issue");
                    }
                }
            }
        }
        info!("New State: {}", &machine_state);
    }
    while !planner.is_empty() {
        let (executor_job, planner_job) = planner.dequeue_move_test_only().unwrap();
        assert_eq!(executor_job.job_type, JobType::StepperJobRun);
        let res = core::mem::ManuallyDrop::into_inner(unsafe { executor_job.data.move_steps });
        info!("[FINAL_BLK] {:#?}", res);
        info!("{:#?}", planner_job)
    }
}
