#![no_std]
#![no_main]

// cortex-m-semihosting
// logging to the host, slow
use panic_halt as _;
use cortex_m_rt::entry;
use cortex_m_semihosting::debug;
#[entry]
fn main() -> ! {
    let roses = "blue";
    if roses == "red" {
        debug::exit(debug::EXIT_SUCCESS);
    } else {
        debug::exit(debug::EXIT_FAILURE);
    }
    loop {}
}

// panic has exit feature: exit[EXIT_FAILURE]
use panic_semihosting as _;
use cortex_m_rt::entry;
use cortex_m_semihosting::debug;
#[entry]
fn main() -> ! {
    let roses = "blue";
    assert_eq!(roses, "red");
    loop {}
}

// unwind the stack of panicking thread
// #[panic_handler] fn(&PanicInfo) -> !
// dev profile: can put breakpoint on 'rust_begin_unwind'
#[cfg(debug_assertions)]
// handler included in the final executable instead of warning unused import
use panic_halt as _;
// release profile: minimize app binary size
#[cfg(not(debug_assertions))]
use panic_abort as _;

// index out of bound
use panic_semihosting as _;
use cortex_m_rt::entry;
#[entry]
fn main() -> ! {
    let xs = [0,1,2];
    let i = xs.len();
    let _y = xs[i];
    loop{}
}

//exception
