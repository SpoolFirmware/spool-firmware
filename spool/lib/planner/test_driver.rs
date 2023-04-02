use log::{info, trace, warn, debug};
use std::io::{Write};

use log::{Log, Record, Metadata};
use fixed::types::I16F16;
use planner::planner::{Planner, PlannerMove, JobType};

struct YomamaLogger;

impl log::Log for YomamaLogger {
    fn enabled(&self, _metadata: &Metadata) -> bool {
        true
    }

    fn log(&self, record: &Record) {
        if self.enabled(record.metadata()) {
            println!("[{:?}][{}]: {}", &record.level(), record.target(), record.args());
        }
    }

    fn flush(&self) {
        std::io::stdout().flush().unwrap();
    }
}

static YOMAMA: YomamaLogger = YomamaLogger;

fn main() {
    log::set_logger(&YOMAMA);
    log::set_max_level(log::LevelFilter::Trace);

    let mut planner = Planner::new(4, 4);

    let new_move = PlannerMove {
        job_type: JobType::StepperJobRun,
        motor_steps: [1000, 1000, 0, 0],
        delta_x: [I16F16::from_num(10),I16F16::from_num(10),I16F16::from_num(0),I16F16::from_num(0)],
        max_v: [150u32; 4],
        acc: [1000u32; 4],
        stop: false,
    };
    planner.enqueue_move(&new_move);

    debug!("Hello {:#?}", planner);
}
