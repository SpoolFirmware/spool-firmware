use planner::planner::{Planner, PlannerJob};
use std::alloc::Layout;

fn main() {
    let ptr = unsafe { std::alloc::alloc(Layout::new::<[PlannerJob; 8]>()) };
    
    let mut planner = Planner::new(4, 4, unsafe {(ptr as *mut [PlannerJob; 8]).as_mut::<'static>().unwrap()});

    println!("Hello {:#?}", planner);
}
