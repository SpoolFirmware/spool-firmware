#![no_std]

use core::{panic::PanicInfo, ptr::null};

extern "C" {
    fn __panic(file: *const u8, line: i32, msg: *const u8) -> !;
    fn dbgPutc(c: u8);
}

pub struct Console;
impl core::fmt::Write for Console {
    fn write_str(&mut self, s: &str) -> core::fmt::Result {
        for s in s.bytes() {
            unsafe { dbgPutc(s); }
        }
        Ok(())
    }
}
static mut CONSOLE: Console = Console;

pub fn dbgprint(args: core::fmt::Arguments) {
    use core::fmt::Write;
    unsafe {
        let _ = CONSOLE.write_fmt(args);
    };
}

macro_rules! println {
    ($fmt:expr) => (print!(concat!($fmt, "\n")));
    ($fmt:expr, $($arg:tt)*) => (print!(concat!($fmt, "\n"), $($arg)*));
}

macro_rules! print {
    ($($arg:tt)*) => ({
        $crate::dbgprint(format_args!($($arg)*));
    });
}

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    unsafe {
        __panic(null(), 0, null());
    }
}

#[no_mangle]
pub extern "C" fn plannerGetOne() -> u32 {
    println!("hello");
    panic!();
}