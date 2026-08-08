# The /GX EH frame is a SOURCE fact — sweep the tree for presence mismatches
tags: cpp:eh cpp:ctor cpp:local | asm:mov asm:push | topic:audit topic:eh
symptoms: retail's prologue has `push -1` / `push <handler>` / `mov fs:[0],esp` and ours does not (or vice versa); `mov [esp+N],<n>` unwind-state stores appear on only one side, or the two sides store a different NUMBER of them
confidence: 9/10
variants: new-site-eh-states-are-a-called-base-ctor.md

`/GX` is on project-wide, so cl 5.0 emits the registration-record prologue in a function
**iff** that function owns something whose destructor must run during an unwind. No
optimizer setting turns it on or off per function, so a presence disagreement between
base and target is a hard source fact rather than a codegen preference.

```asm
mov  eax, fs:[0]        ; cl 5.0 schedules the load first or third
push -1                 ; initial unwind state
push <__ehhandler$...>
push eax
mov  fs:[0], esp
...
mov  DWORD PTR [esp+0x38], 1   ; one unwind state per live destructible object
```

`python -m gruntz.audit.eh_frame` classifies every scoring function on both sides and
reports the disagreements; `--states` is the wider secondary sieve (both sides framed,
different NUMBER of state stores — one object's worth of resolution).

**Three causes, different work, and the tool separates them — only the last is what
the frame naively suggests.** A ctor/dtor COMDAT one side calls and the other never
calls at all is **INLINE_CUT**: same object, and cl picks the cut per `new`-site, so it
is a wall (see the variant). The SAME ctor/dtor called a different NUMBER of times is
**EXIT_MERGE**: cl gives every `if (...) return 0;` its own exit block but collapses all
of them when a `||`/`&&` guard sends them to a common destination, and each surviving
dtor copy carries its own state store — `CGruntSpawnConfig::SpawnVoiceDriver` calls
`??1CString` twice for us and eight times in retail off one `&&`. That is
`gruntz.audit.exit_merge_sieve`'s lever, not this one. Only when neither kind of call
difference exists does one side really own an object the other's source never declared:
a by-value `CString` where the other wrote `LPCSTR`, a by-value `CRect`/MFC collection,
a stack helper whose dtor releases something.

**Calibrate, then believe it.** Run `--calibrate`: the functions objdiff already scores
at 100.00% are byte-identical and must agree. Measured 2026-08-08, presence 0 of 3455
and state count 0 of the 510 EH-framed among them. That proves determinism and
side-symmetry only — it cannot prove a state-count row is semantic, because a
byte-identical pair agrees under any consistent rule. Two real detector bugs were caught
this way and both manufactured `±1` rows: reading only immediate state stores misses a
`mov %esi,<slot>` where the register holds 0, and llvm-objdump's trailing
`# imm = 0xFFFFFFFF` annotation defeats a naive slot match on exactly the leave-region
stores.

Tree-wide 2026-08-08: 750 EH-framed target functions, 3 presence mismatches (all
INLINE_CUT, all `CStatusBarMgr`) and 44 state-count rows — 23 INLINE_CUT, 9 EXIT_MERGE,
9 MISSING_OBJECT, 6 EXTRA_OBJECT. Presence is closed; without the cause split a lane
would have gone after 32 phantom objects.
