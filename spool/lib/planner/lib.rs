#![no_std]
use core::{panic::PanicInfo, ptr::null};

extern "C" {
    fn __panic(file: *const u8, line: i32, msg: *const u8) -> !;
}

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    unsafe {
        match info.location() {
            Some(loc) => __panic(loc.file().as_bytes().as_ptr(), loc.line() as i32, null()),
            None => __panic(null(), 0, null()),
        }
    }
}

#[no_mangle]
pub extern "C" fn plannerGetOne() -> u32 {
    1
}