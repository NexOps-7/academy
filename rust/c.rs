/* 2025 2032 2042 2052 2072
    preprocessor
        #ifdef compile-time code selection
            cargo[features]
                lib.rs top-level cfg: conditional compilation based on flag -> [features]
                cargo feature for each component
                    fir: finite impulse resopnse iir: infinite impulse response 
                    signal processing primitives compile slow/ large table of constants */
#[cfg(features="FIR")]
pub mod fir;
#[cfg(features="IIR")]          
pub mod iir;
[features]
FIR = []
IIR = []

/* compile time arr sizes/computations
    const fn: evaluable at compile time
        buf: buffer, store byte in mem */
const fn arr_size() -> usize {
    #[cfg(feature="use_more_ram")]
    { 1024 }
    #[cfg(not(feature="use_more_ram"))]
    { 128 }
}
static BUF: [u32; arr_size()] = [0u32; arr_size()];

/* c array access
        slow bounds check
    rust iterators 
        mem safety: check out-of-bound access on muanual arr 
        chaining, enumerating, zipping, find min/max, summing */
int16_t arr[16];
int i;
for (i=0; i<sizeof(arr)/sizeof(arr[0]); i++) {
    process(arr[i]);
}
let arr = [0u16; 16];
for ele in arr.iter() {
    process(*ele);
}