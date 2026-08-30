# A small clear written as `memset` owns its zero register and destination cursor
tags: cpp:local cpp:struct cpp:array cpp:member cpp:memset | asm:add asm:xor asm:mov | topic:codegen-idiom topic:regalloc
symptoms: retail has a dedicated `xor reg,reg` immediately before a short run of zero stores; for a member array it may also advance `this` in place, while a hand-written pointer loop keeps `this` and the zero in the opposite registers
confidence: 9/10

cl 5.0 expands a small `memset(&s, 0, sizeof s)` as a dedicated `xor reg,reg`
plus one `mov` per dword. That zero is *its own* value, so it does not get
coalesced with an accumulator that happens to hold 0. Writing the same clear as
individual field stores gives cl a set of independent `= 0` assignments, and it
happily services all of them from whichever register already holds zero:

```asm
;; retail - memset: fresh zero, then three stores
xor    eax,eax
lea    ecx,[esp+0x10]
mov    [esp+0x10],eax
push   ecx
mov    [esp+0x18],eax
mov    [esp+0x1c],eax
call   _heapwalk
...
mov    [esp+0x10],ebx        ; a LATER single clear borrows the accumulator's 0

;; ours - three field stores, all from the accumulator's zero (esi), no xor
mov    [esp+0x10],esi
mov    [esp+0x18],esi
mov    [esp+0x1c],esi
```

```cpp
// WRONG
hinfo._pentry = NULL;
hinfo._size = 0;
hinfo._useflag = 0;

// RIGHT
memset(&hinfo, 0, sizeof(hinfo));
```

The mixed pattern in one function is the strong signal: retail materializing a
zero for one clear while reusing a live zero register for another clear of the
SAME struct means the two clears came from different constructs - a `memset` and
a field assignment. `HeapStats` 0x118bf0 97.34 -> 100.00 EXACT (its sibling
`HeapCheckDump` already used `memset` and was exact).

The same distinction applies to a small fixed member array even when the loop
is fully unrolled and the instruction counts already agree. For two pointers at
`this+8`, retail `CVoiceManager::ClearVoiceIndicatorSlots` is:

```asm
add    ecx,8
xor    eax,eax
mov    [ecx],eax
mov    [ecx+4],eax
ret
```

The hand-written pointer loop emitted the same five operations with the roles
reversed (`lea eax,[ecx+8]`, then `xor ecx,ecx`) and scored 82.00. Direct stores,
post-increment, and indexed-pointer controls each scored 67.80. Restoring the
single aggregate operation closed the function:

```cpp
memset(m_indicators, 0, sizeof(m_indicators));
```

`CVoiceManager::ClearVoiceIndicatorSlots` 0x11af90: 82.00 -> 100.00 EXACT. The detection
signature is an already byte-complete clear whose only difference is that
retail destructively biases `this` to the member-array base and then creates a
fresh zero in the accumulator. Do not retain a pointer loop merely because its
stores and final state are equivalent.

The same signature can occur twice at the end of a larger method. Retail
`CVoiceManager::Clear` clears `m_indicators[2]` through an address local, then
advances `this` by `0x10` and clears `m_streamVoices[2]` with a second fresh
zero. Two hand-written two-iteration loops had the same stores but selected the
opposite cursors. Restoring the already-attested `CLEAR_VOICE_INDICATORS` macro
and `memset(m_streamVoices, 0, sizeof(m_streamVoices))` made the complete
0x95-byte function exact: 87.24 -> 100.00.
