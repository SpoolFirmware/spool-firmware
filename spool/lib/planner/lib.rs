#![no_std]
use core::panic::PanicInfo;

#[panic_handler]
fn panic(_: &PanicInfo) -> ! {
    loop {}
}
#[no_mangle]
pub extern "C" fn plannerGetOne() -> u32 {
    1
}