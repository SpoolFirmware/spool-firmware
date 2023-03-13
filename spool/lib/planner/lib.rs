#![no_std]

#[macro_use]
pub mod platform;

#[no_mangle]
pub extern "C" fn plannerGetOne() -> u32 {
    println!("hello");
    panic!();
}