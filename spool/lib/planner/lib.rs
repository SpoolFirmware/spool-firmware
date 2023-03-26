#![no_std]

use planner::Planner;

#[macro_use]
pub mod platform;

mod planner;

pub const MAX_STEPPERS: usize = 4;
pub const MAX_AXIS: usize = 4;

#[no_mangle]
extern "C" fn plannerGetOne() -> u32 {
    println!("hello");
    panic!();
}

#[no_mangle]
extern "C" fn plannerGetPlannerSize() -> u32 {
    core::mem::size_of::<Planner>() as u32
}

extern "C" fn plannerGetPlannerAlignment() -> u32 {
    core::mem::align_of::<Planner>() as u32
}

#[no_mangle]
extern "C" fn plannerInit(planner_ptr: *mut Planner) {
    let planner = unsafe {
        planner_ptr.as_mut().unwrap()
    };
    planner.init();
}
