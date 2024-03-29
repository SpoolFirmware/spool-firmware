use core::{panic::PanicInfo, ptr::null};

use log::{Log, Record, Metadata};

extern "C" {
    fn __panic(file: *const u8, line: i32, msg: *const u8) -> !;
    fn dbgPutc(c: u8);
    fn sqrtf(f: f32) -> f32;
}

pub fn platform_sqrtf(f: f32) -> f32 {
    unsafe {
        sqrtf(f)
    }
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
        $crate::platform::dbgprint(format_args!($($arg)*));
    });
}

struct SpoolLogger;
static LOGGER: SpoolLogger = SpoolLogger;

impl Log for SpoolLogger {
    fn enabled(&self, _metadata: &Metadata) -> bool {
        true
    }

    fn log(&self, record: &Record) {
        if self.enabled(record.metadata()) {
            println!("[{:?}][{}]: {}", &record.level(), record.target(), record.args());
        }
    }

    fn flush(&self) {}
}

pub fn logger_init() {
    log::set_logger(&LOGGER).unwrap();
    log::set_max_level(log::LevelFilter::Info);
}

#[panic_handler]
pub fn panic(info: &PanicInfo) -> ! {
    println!("UwU: {}", info);
    unsafe {
        __panic(null(), 0, null());
    }
}
