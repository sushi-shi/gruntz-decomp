# A wire cursor initialized `param + K` takes the param's register; a copy-first start folds the read
tags: cpp:local cpp:call | asm:inc asm:lea asm:mov | topic:codegen-idiom
symptoms: retail parses a buffer with `mov eax,[esp+4]; push esi; mov esi,eax; inc eax; mov dl,[eax]; inc eax; ...` (inc-then-read, cursor in the param's home register) while the base emits `push esi; mov esi,[esp+8]; mov dl,[esi+1]; lea eax,[esi+1]; inc eax` (first read folded into a displacement off the start local)
confidence: 8/10

Which local coalesces with the incoming pointer parameter decides the whole
head of a serializer. Written

```cpp
char* buf = static_cast<char*>(data);
char* start = buf;            // copy captured FIRST
buf++;                        // (or *++buf reads)
```

cl 5.0 coalesces START with the param (one load into esi), and the first
`*buf` read - provably at start+1 - FOLDS into `[esi+1]` with the cursor
materialized late by `lea`. Seven spellings of this order (pre/post
increment, by-reference read helpers, param re-reads) all produce the folded
form. Written

```cpp
char* buf = static_cast<char*>(data) + 1;   // cursor def CONSUMES the param
char* start = buf - 1;
```

the cursor's def is arithmetic on the param's last use, so the CURSOR takes
the param's home register and walks it with `inc` before every read -
retail's stream. `start` then materializes from the cursor.

Residue on the measured pair: retail derives start with `mov esi,eax` BEFORE
the first inc (start = buf's pre-increment value), ours with
`lea esi,[eax-1]` after - the `start = buf - 1` initializer names its own
computation. `start = data; buf = start + 1;` re-folds (the copy is first
again), so the mov-before-inc form was not reached from source; TU-state
probes are inert on this unit.

## Measured

- CGruntzMultiCommand::Parse 0x24000 83.20 -> 92.00
- CGruntzSingleCommand::Parse 0x23f90 86.45 -> 93.55
