# A struct cleared with `memset` gets its OWN `xor reg,reg`; field stores borrow an accumulator's zero
tags: cpp:local cpp:struct cpp:memset | asm:xor asm:mov | topic:codegen-idiom topic:regalloc
symptoms: base is one instruction short; retail has a `xor eax,eax` immediately before a run of `mov [esp+N],eax` stores that clear a small stack struct, and reuses a DIFFERENT already-zero register for a later single-field clear; ours writes every clear from one shared zero register
confidence: 8/10

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
