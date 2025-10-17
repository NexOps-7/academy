// hardware f3
#![no_std]
#![no_main]

use panic_halt as _;
use cortex_m_rt::entry;
// run on QEMU using semihosting
use cortex_m_semihosting::{debug, hprintln};
#[entry]
// !: no ret
fn main() -> ! {
    hprintln("hello world").unwrap();
    // exit QEMU
    debug::exit(debug::EXIT_SUCCESS);
    loop {}
}


// mircro-architecture 
// Cortex-M SysTick peripheral
use cortex_m::peripheral::{syst, Peripherals};
use cortex_m_rt::entry;
use panic_halt as _;
#[entry]
fn main() -> ! {
    // only one SYST struct
    let peripherals = Peripherals::take().unwrap();
    let mut systick = peripherals.SYST;
    systick.set_clock_source(syst::SystClkSource::Core);
    systick.set_reload(1_000);
    systick.clear_current();
    systick.enable_counter();
    // delay for x millisecs
    while !systick.has_wrapped() {
        // loop
    }
    loop{}
}

// PAC: peripheral access crate
// off-chip peripheral mem mapped registers
// tm4c123 80MHz Cortex-M4 256KiB flash
use panic_halt as _;
use cortex_m_rt::entry;
use tm4c123x;
#[entry]
pub fn init() -> (Delay, Leds) {
    let cp = cortex_m::Peripherals::take().unwrap();
    let p = tm4c123x::Peripherals::take().unwrap();
    let pwm = p.PWM0;
    // ctl: control register
    // globalsync0(): bit 0 update pwm generator 0 on slice 0
    // clear_bit(): set bit to 0, disable global
    pwm.ctl.write(|w| w.globalsync0().clear_bit());
    // mode 1 count up/down mode
    pwm._2_ctl.write(|w| w.enable().set_bit().mode().set_bit());
    pwm._2_gena.write(|w| w.actcmpau().zero().actcmpad().one());
    // 528 cycles(264 up & down) 4 loops per video line 2112 cycles
    // unsafe: svd(xml) doeant say if e.g. 32-bit has a special meaning
    // bits() raw bit writes w/o register field validation
    // 0-263->264 ticks cmpa: compare duty cycle 64/264=24.24%
    // pwm freq = clk/(LOAD+1)*div = clk/528 = 16 MHz/528 = 30 kHz
    pwm._2_load.write(|w| unsafe {w.load().bits(263)});
    pwm._2_cmpa.write(|w| unsafe {w.compa().bits(64)});
    pwm.enable.write(|w| w.pwm4en().set_bit());

    if pwm.ctl.read().globalsync0().is_set() {
    }
    /*  // in c
        uint32_t temp = pwm0.ctl.read();
        temp |= PWM0_CTL_GLOBALSYNC0;
        pwm0.ctl.write(temp);
        uint32_t temp2 = pwm0.enable.read();
        temp2 |= PWM0_ENABLE_PWM4EN;
        pwm0.enable.write(temp2);
     */
    pwm.ctl.modify(|r, w| w.globalsync0().clear_bit());
}

// HAL: hardware abstract layer, api 
// write_byte GPIO, Serial, I2C, SPI, timer, analog-digital
use panic_halt as _;
use cortex_m_rt::entry;
use tm4c123x_hal as hal;
use tm4c123x_hal::prelude::*;
use tm4c123x_hal::serial::{NewlineMode, Serial};
use tm4c123x_hal::sysctl;
#[entry]
fn main() -> ! {
    let p = hal::Peripherals:take().unwrap();
    let cp = hal::CorePeripherals::take().unwrap();

    // wrap up SYSCTL struct into higher-layer api obj
    let mut sc = p.SYSCTL.constrain();
    // oscillator settings
    sc.clock_setup.oscillator = sysctl::Oscillator::Main(
        sysctl::CrystalFrequency::_16mhz,
        // Pll: phase-locked loop, output high-freq phase fixed to inp low-freq phase
        sysctl::SystemClock::UsePll(sysctl::PllOutputFrequency::_80_00mhz),
    );
    // use the sc settings above
    let clocks = sc.clock_setup.freeze();
    // wrap up GPIO_PORTA struct to higher-layer api obj
    // power_control: power up GPIO automatically
    let mut porta = p.GPIO_PORTA.split(&sc.power_control);
    // activate uart serial
    let uart = Serial::uart0(
        p.UART0,
        // transmit pin pa1 port a pin 1 -> AF1 alternate function
        porta
            .pa1
            // push_pull: high & low
            .into_af_push_pull::<hal::gpio::AF1>(&mut porta.control),
        // receive pin pa0
        porta
            .pa0
            .into_af_push_pull::<hal::gpio::AF1>(&mut porta.control),
        (),
        (),
        // baud rate
        115200_u32.bps(),
        // output handling
        NewlineMode::SwapLFtoCRLF,
        &clocks,
        &sc.power_control,
    );
    loop {
        writeln!(uart, "hi\r\n").unwrap();
    }
}