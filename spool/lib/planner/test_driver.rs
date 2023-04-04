use clap::{Arg, Command};
use fixed::traits::ToFixed;
use gcode::Mnemonic;
use log::{debug, info, trace, warn, error};
use std::{fmt::Display, fs::File, io::Write, path::PathBuf};
use std::boxed::Box;

use fixed::types::I16F16;
use log::{Log, Metadata, Record};
use planner::planner::{JobType, Planner, PlannerMove, PlannerJob};

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
            },
            None => 0.0,
        }
    };
}

const STEPS_PER_MM: [f32; 4] = [80.0, 80.0, 400.0, 800.0];

impl MachineState {
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
                    let xyze_steps: Vec<_> = xyze.iter().zip(STEPS_PER_MM.iter()).map(|(mm, step)| (mm * step) as i32).collect();
                    self.x += xyze[0];
                    self.y += xyze[1];
                    self.z += xyze[2];
                    self.e += xyze[3];
                    return Some(PlannerMove {
                        job_type: JobType::StepperJobRun,
                        motor_steps: *(Box::<[i32; 4]>::try_from(xyze_steps).unwrap()),
                        delta_x: xyze.map(|x| x.to_fixed()),
                        max_v: [200, 200, 5, 40],
                        acc: [1500, 1500, 500, 1000],
                        stop: false,
                    });
                }
                28 => {
                    *self = Self::default();
                }
                90 => {
                    self.abs = true; self.e_abs = true;
                }
                91 => {
                    self.abs = false;
                }
                _ => {
                    debug!("Unsupported G: {}", gcode);
                }
            },
            Mnemonic::Miscellaneous => match gcode.major_number() {
                82 => {
                    self.e_abs = true;
                }
                83 => {
                    self.e_abs = false;
                }
                _ => {
                    debug!("Unsupported M: {}", gcode);
                }
            }
            _ => {
                debug!("Unsupported Type: {}", gcode);
            }
        }
        None
    }
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

    let mut planner = Planner::new(4, 4);

    debug!("Init Planner {:#?}", planner);

    let mut machine_state = MachineState::default();
    info!("Init State: {}", &machine_state);

    let buffer = std::fs::read(&file_path).expect("File no existo?");
    let string = String::from_utf8(buffer).expect("bad file content");
    let gcodes = gcode::parse(&string);
    for (i, gcode) in gcodes.enumerate() {
        info!("Input[{:04}]: {}", i, gcode);
        if let Some(planner_move) = machine_state.process_gcode(&gcode) {
            trace!("Move: {:#?}", &planner_move);
            if let Err(_) = planner.enqueue_move(&planner_move) {
                error!("failed to enqueue");
            }
        }
        info!("New State: {}", &machine_state);
    }
}
