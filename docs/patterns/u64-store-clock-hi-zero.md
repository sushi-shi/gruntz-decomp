# Adjacent {lo:=clock, hi:=0} timer pair the target writes lo-first is ONE 64-bit store
tags: cpp:store cpp:member | asm:mov | topic:codegen-idiom
symptoms: two int stores to adjacent fields get REORDERED by cl (const `hi=0` scheduled before `lo=eax`), structural diff ~93-94%, an arm-a-countdown-timer pattern
confidence: 8/10

A per-frame countdown/elapsed-timer field that the target arms with `lo = <clock>, hi = 0`
in lo-then-hi order is a SINGLE 64-bit zero-extending store, not two int stores. Written as
two `int` assignments (`m_lo = g_clock; m_hi = 0;`) MSVC5 /O2 reorders them — it schedules
the const `hi = 0` store *before* the `lo = eax` store → structural diff. Model the adjacent
pair as one 64-bit store of the zero-extended clock to get the exact lo-first emission.

```cpp
*(unsigned __int64*)&m_timerLo = g_645588;   // emits mov [obj+lo],eax; mov [obj+hi],0 in order
// NOT: m_timerLo = g_645588; m_timerHi = 0;  // cl reorders -> hi-store first
```
```asm
mov  [obj+lo], eax        ; lo = zero-extended clock
mov  dword [obj+hi], 0    ; hi = 0, emitted AFTER lo
```
STEERABLE. Evidence: CPlay::OnRegion0..3 (@0xd8aa0/0xd8a00/0xd8b20/0xd8bc0) — the lever that
took all four scroll one-shots BYTE-EXACT (99.0-99.3%); two-int form plateaued 93-94%.
