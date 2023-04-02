#![no_std]

#[cfg(not(test))]
#[cfg(not(feature = "std"))]
#[macro_use]
pub mod platform;

#[cfg(test)]
#[cfg(feature = "std")]
pub mod platform_std;

#[cfg(not(test))]
#[cfg(not(feature = "std"))]
use crate::platform::logger_init;

#[cfg(test)]
#[cfg(feature = "std")]
use crate::platform_std::logger_init;

pub mod planner;

use core::{ffi::c_void, mem::MaybeUninit};

use planner::{Planner, PlannerJob};

pub const MAX_STEPPERS: usize = 4;
pub const MAX_AXIS: usize = 4;

extern "C" {
    fn portMallocAligned(size: usize, alignment: usize) -> *mut c_void;
}

unsafe fn allocateStatic<T>() -> Option<&'static mut MaybeUninit<T>> {
    let ptr = portMallocAligned(core::mem::size_of::<T>(), core::mem::align_of::<T>());
    (ptr as *mut MaybeUninit<T>).as_mut::<'static>()
}

#[cfg(not(test))]
#[cfg(not(feature = "std"))]
#[no_mangle]
#[allow(non_snake_case)]
extern "C" fn plannerInit(num_axis: u32, num_stepper: u32) -> *mut Planner {
    let planner = unsafe {
        let planner = allocateStatic::<Planner>().unwrap();
        *planner = MaybeUninit::new(Planner::new(num_axis, num_stepper));
        planner.assume_init_mut()
    };
    logger_init();
    planner as *mut Planner
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
