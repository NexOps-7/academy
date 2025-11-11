## binary
    two orders: 0+1+1 -> binary 10 carryout+sum
        carry-in    B   A carry-out sum
        0           1   1   1       0
        (A+B) & (A XOR B) never both 1, max bound to three orders 100
            no double carry in OR, (A+B)=1->10, (A XOR B)=0
## fan-out: no. of logic inputs another single logic gate output can drive
    400mV margin
## logic design
    RTL NOR gate: 1->inp, saturate transistor, output->0 0 0 ->1
        0.2->0 0.9->1 
        fan-out 4 5 parellel current hogging -> resistor
        delay: switching speed 50ns
        noise immunity/margin 0.5v 0.2v
    RCTL: leading/trailing edge bypass
        power dissipation per gate
    DTL: NAND
        input 1/vcc 4v -> no cur, resistence, hold saturation, collector low 0 output
        input 0/gnd -> voltage drop max 0.7v -> not enough
            transistor off, collector Vcc
            0.2v
            1 4v
        fan-out: 8
        delay 25ns
        noise margin: 1.2v series threshold 0.7x2=1.4v
    TTL: emitter-based diode -> input
            base-collector diode -> series diode
        diffused: collector region <- base <- emitter
        output stage: active pull-up transistor
    ECL: emitter ref transistor T4
        common emitter resistor high: constant-current source
        inputs 0 -> T off, no current, collector vcc, T5 output 1
        inputs higher > ref vbb, cur, collector potential fall -> output 0
        prevent transistor saturation, fast 
            logic transistor current up, ref T cur down
        n ns
        power dissipation: 50mW
        fan-out: 25
    lowest cost per gate: as many gates as possible on one chip
        12 99 >100 gates
        reliability from minimized interconnection
        89% reduction in pcb
        connector, design time, assembly labor, test, inventory, stockning, handling, negotiations all reduced
## TI 54/74
    5v 0-0.2v 1-3v noise: 1v  
    high speed: less resistence, low output impedence
        clamping diodes for fast transmission line effect
            darlington transistor pair
            6ns per gate
    low power: more resistence, reduction in power, 
        1/10 1mw 33ns speed per gate
    schottky: high speed low power
        high speed: unsaturated logic/ecl
            no minority carries/stored charge
            switching time less
            lower voltage drop than silicon p-n junction
        clamp excess current, prevent saturation
            max current/low voltage drop/closed sw Q2 Q3
                T4 off collector vcc 1 output
        3ns 20mw
## NAND:
    all input 1 -> Q1 inverse Q2 conduct/voltage drop -> v>1.3 -> output 0
    transition period: 0.7-1.3 Q2(R2/R3) & Q4 on -> Q3 Q2 Q4 on/current spike
         -> Q4 off -> 0
    any 0 -> v < 0.7 -> output 1
        base-emitter forward-biased/conducts/saturates
        Q1 collector == Q2 base -> Q1 saturates -> fast removal Q2 stored base charge
            faster turn-off time
        multi-emitter: small, low cost/capcitance, fast switching
        output Q3 4 low source impedence, allow high capacitance load/output
            Q4 drive current into load, impeded only by Q3
## worst-senario params test
    pos: current flow towards IH high
        neg: current flow away from IL low
    inpuot voltage vs current 
        temp vcc
        source current OH
        sink current OL
    IL: 
        test vcc 4.5v for IL V<0.8v -> normal Vbe drop & diode V drop
            -> test fail: OL greater than max 0.4v sink 1.6mAx10=16mA 0
                vcc max all unused inputs 1 -> output 0
    IH:
        emitter-base reverse-biased/off collector-base forward-biased/conducts
            emitter->collector collector->emitter
                inverse current gain of Q1 as input current
            -> multi emitter inverse in between/leakage instead
                IH split current
                -> test leakage: V IH 5.5v>2v IH<1mA max 5.5mW
                    input 0, vcc max->max current 
                        unused inputs grounded 
        IH>2v at OH min 2.4v
            -> test OH min 2.4v 40uA 10
    output: 
        OL: lower temp/vcc 
                -> OL V higher, more difficult to keep in saturation
            -> test OL max 0.4v 16mA
                all inputs 1 2v 
        OH: lower temp/vcc
                -> OH V lower 
            -> test OH min 2.4v source |-400uA| 10
                min input 0.8v, hold Q3 off
                    unused vcc
            -> test output short to ground, R4 Q4 D
                OH V = 0
                    all inputs ground, max vcc
                -> test |-20mA| < current < |-55mA|
    switch speed: 
        propagation delay time output
         1 -> 0 HL 15ns 7ns
         0 -> 1 LH  22ns 11ns
        higher capacitance, more delay
        HL lower temp/higher vcc, more delay
            higher temp, less delay
        LH higher temp, more delay
            lower temp/higher vcc, less delay
        avg higher vcc, less delay
        more fan-outs, more delay/less voltage <- increased capacitance
    supply current:
        average per-gate supply current same 
            0 Icc L max 5.5mA 3mA 2.4v
            1 Icc H max 2mA 1mA 0.4v
        nominal vcc 5v 91% worst-case val
            increase current draw -> charge internal capacitance if no load
            charge discharge
                LH charge by increased current up til output short current
        lower freq>50khz/higher cap, more current
            higher freq>50khz/lower cap, less current
    dc noise margin
        400 mV -> 1v
        0 <0.4v output + noise = 0.8v input 0.2v tolerate 1.15v
        1 >2.4v - noise = 2v 3.3v, tolerate 1.5v
        higer temp, less margin
            less temp, more margin
    ac noise margin
        pulse width shorter than propagation time
            higher-amplitude pulse to output change, then no pulse be propagated thru
            change input to a different stage long enough
                -> sharp knee curve
        pulse amplitude 
            propagation delay: input into opposite state
            capacitive storage: store pulse, sequence trigger gate response
                time between pulses > pulse width, no buildup
                not short high-amplitude pulses
        0-to-1 worse than 1-to-0
            immunity NAND gate less: long propagation time to 1 state
    noise rejection
            triggerable storage eles
            fast noise as signal
        circuit impedance <- coupling impedence/stray cap <- noise source
            load impedence by gates
        tight coupling: source effect 
            cant assess, improve noise rejection -> ignored
        ramp input: dv/dt 0.4v/ns-0.8v/ns 
                time constant T: RC
                eo = T*(1-e(-t/T))
            pulse 4v rising at 0.4v/ns gate 2, 70ohm impedence, 70pF cap
                T = 4.9ns rise time = 4v/0.4/ns = 10ns 
                    norminal val multiple curve by 10, voltage by 4 
                    -> curve T=0.5ns -> peak 0.41v x 4 = 1.64v
                        curve pulse width 50% = 2.4ns/2 = 1.2ns x 10 = 12ns
                        -> 2v noise immunity, not affected
                1kohm impedence, T = 70ns
                    -> curve T=7ns -> peak 0.9v x 4 = 3.6v
                        pulse width 50% = 12/2 x 10 = 60ns
                            -> cause interference
# specialized gates

                
        


cache line 
        system
            l1 instruction cache l1 data cache
            l2 cache
        l3 cache
        main memory
    alloc cache line to same sized region in main mem
        hit/miss, cache line fill, replacement, writeback
    locality: avoid pollution/use infrequently/not at all
        temporal: used more than once in a short period of time
        spatial: adjacent address data
    prefetch

ripple counter
    assynchronous/not under one clock pulse
    8-4-2-1 bcd decade counter: cycle length differing from 2^N
        0-9 count in 4-digit binary, recycle for every 10 clock pulses
        reset logic 0 bcd 9
    NAND gate: not and 
        0 0 -> 1 
        0 1 -> 1
        1 1 -> 0
    eliminate decode spike/delay: strobe pulse
        decode occur only after all flip-flops in counter stable
    clock pulse max f
    connect NAND gate output to reset/preset input of counter
        2^(n-1) < N < 2^n   2^3 < 10 < 2^4 N=10 0101 
        flipflop Q=0 at N-1/Q=1 at N, reset to 0
    elimiate reset propagation time: latch/delay until stable/feedback loop
synchronous counter
    parallel carry
    serial/ripple carry: clock speed reduce 50ns -> 100ns
    count up/down
        ignore unused x state
three modes: A not internally connected
    bcd
    symmerical divide-by-10
    divide-by-5
binary divider:
    two counters, snd is the a relative prime of the fst

shift register
