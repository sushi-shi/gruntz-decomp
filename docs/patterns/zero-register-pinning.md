# Retail pins `0`/`1` in a callee-saved register across the body — register-allocation wall
tags: cpp:local | asm:xor asm:mov | topic:wall topic:regalloc
symptoms: structure byte-exact, a 1-instr phase shift cascading through every =0/=1 store + null test, ebx/edi/ebp zero-reg
confidence: 7/10

In bodies with many `member = 0;` / `member = 1;` stores, `push 0` args and null tests, retail
PINS the constant `0` (and sometimes `1`) into a callee-saved register (`ebx`/`edi`/`ebp`) at
the top and reuses it everywhere; under the resulting register pressure it may re-read a value
from memory instead of caching it. Our `cl` defers the zero / caches differently, producing a
1-instruction phase shift that cascades through every such store — structure and offsets stay
byte-exact, only the register choices differ. No source lever (`int z=0;`, comparison-order
flips) reliably forces the pinning under /O2.

WALL (register allocation). Evidence: CGrunt::LoadEntranceConfig 78.8%, BuildEntranceAnimation 71%, CBattlezMapConfig::LoadConfig 88%.
