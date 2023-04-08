#![cfg_attr(not(feature = "std"), no_std)]
#[cfg(not(test))]
#[cfg(not(feature = "std"))]
#[macro_use]
pub mod platform_no_std;

#[cfg(feature = "std")]
mod platform_std;

#[cfg(not(test))]
#[cfg(not(feature = "std"))]
use crate::platform_no_std::logger_init;

pub mod platform {
    #[cfg(not(feature = "std"))]
    pub use crate::platform_no_std::*;
    #[cfg(feature = "std")]
    pub use crate::platform_std::*;
}

pub mod planner;

use core::{ffi::c_void, mem::MaybeUninit};

use planner::{Planner, PlannerMove, MoveSteps};

pub const MAX_STEPPERS: usize = 4;
pub const MAX_AXIS: usize = 4;

extern "C" {
    fn portMallocAligned(size: usize, alignment: usize) -> *mut c_void;
}

unsafe fn alloc_static<T>() -> Option<&'static mut MaybeUninit<T>> {
    let ptr = portMallocAligned(core::mem::size_of::<T>(), core::mem::align_of::<T>());
    (ptr as *mut MaybeUninit<T>).as_mut::<'static>()
}

#[cfg(any(not(test), not(feature = "std")))]
#[no_mangle]
#[allow(non_snake_case)]
extern "C" fn plannerInit(num_axis: u32, num_stepper: u32, steps_per_mm: *const u32) -> *mut Planner {
    let planner = unsafe {
        let planner = alloc_static::<Planner>().unwrap();
        *planner = Planner::new(num_axis, num_stepper);
        let steps_per_mm_slice = core::slice::from_raw_parts(steps_per_mm, num_axis);
        let planner = planner.assume_init_mut();
        planner.steps_per_mm[..].clone_from_slice(steps_per_mm_slice);
        planner
    };
    logger_init();
    planner as *mut Planner
}

#[cfg(any(not(test), not(feature = "std")))]
#[no_mangle]
#[allow(non_snake_case)]
extern "C" fn plannerEnqueue<'a>(planner: *mut Planner, planner_move: *const PlannerMove) -> bool {
    use planner::PlannerError;

    match unsafe { (planner.as_mut::<'static>(), planner_move.as_ref::<'a>()) } {
        (Some(planner), Some(planner_move)) =>
            match planner.enqueue_move(planner_move) {
                Ok(()) => true,
                Err(PlannerError::CapacityError) => false
            }
        _ => panic!("planner/planner_move NULL"),
    }
}

#[cfg(any(not(test), not(feature = "std")))]
#[no_mangle]
#[allow(non_snake_case)]
extern "C" fn plannerDequeue<'a>(planner: *mut Planner, move_steps: *mut MoveSteps) -> bool {
    use planner::PlannerError;

    match unsafe { (planner.as_mut::<'static>(), move_steps.as_ref::<'a>()) } {
        (Some(planner), Some(move_steps)) =>
            match planner.dequeue_move() {
                None => false,
                Some(steps) => {
                    *move_steps = steps;
                    true
                }
            }
        _ => panic!("planner/planner_move NULL"),
    }
}

#[cfg(any(not(test), not(feature = "std")))]
#[no_mangle]
#[allow(non_snake_case)]
extern "C" fn plannerFreeCapacity<'a>(planner: *const Planner) -> u32 {
    use planner::PlannerError;

    match unsafe { planner.as_ref::<'static>() } {
        Some(planner) => planner.free_capacity(),
        None => panic!("planner NULL"),
    }
}

#[cfg(test)]
#[macro_use]
extern crate std;

#[cfg(test)]
mod test {

    #[test]
    fn test_yomama() {
        println!("hello world!!!\n\n");
    }
}
