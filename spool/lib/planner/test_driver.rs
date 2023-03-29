use fixed::types::I16F16;
use planner::planner::{Planner, PlannerMove, JobType};

fn main() {    
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

    println!("Hello {:#?}", planner);
}
