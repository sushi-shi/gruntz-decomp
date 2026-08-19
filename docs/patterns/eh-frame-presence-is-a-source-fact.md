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

`gruntz walls eh-frame` classifies every scoring function on both sides and
reports the disagreements; `--states` is the wider secondary sieve (both sides framed,
different NUMBER of state stores). State count is a placement/flow lead, not an
object census by itself.

**Three causes, different work, and the tool separates them — only the last is what
the frame naively suggests.** A ctor/dtor COMDAT one side calls and the other never
calls at all is **INLINE_CUT**: same object, and cl picks the cut per `new`-site, so it
is a wall (see the variant). The SAME ctor/dtor called a different NUMBER of times is
**EXIT_MERGE**: cl gives every `if (...) return 0;` its own exit block but collapses all
of them when a `||`/`&&` guard sends them to a common destination, and each surviving
dtor copy carries its own state store — `CGruntSpawnConfig::SpawnVoiceDriver` calls
`??1CString` twice for us and eight times in retail off one `&&`. That is
the exit-merge lever, not this one. With frames on both sides and no ctor/dtor
call delta, the tool now reports **STATE_FLOW**: ordinary calls, duplicated blocks,
or lifetime placement can repeat the same state without adding an object. Only a
frame-presence mismatch with neither call difference proves one side owns a
destructible object the other never declared: a by-value `CString` where the other
wrote `LPCSTR`, a by-value `CRect`/MFC collection, or a stack helper whose dtor
releases something.

`CGrunt::StepArrivalDrop` at 0x4b370 is the negative control. Both sides have the
same state values `{-1,0,1}` and the same ctor/dtor call set, but retail stores one
state an extra time (8 versus 7) alongside its fourth `CPtrList::RemoveHead` flow
site. The single `CPtrList probe` scope is correct; a second object would contradict
the ctor/dtor, call, relocation, and branch census.

**Calibrate, then believe it.** Run `--calibrate`: the functions objdiff already scores
at 100.00% are byte-identical and must agree. Measured 2026-08-08, presence 0 of 3455
and state count 0 of the 510 EH-framed among them. That proves determinism and
side-symmetry only — it cannot prove a state-count row is semantic, because a
byte-identical pair agrees under any consistent rule. Two real detector bugs were caught
this way and both manufactured `±1` rows: reading only immediate state stores misses a
`mov %esi,<slot>` where the register holds 0, and llvm-objdump's trailing
`# imm = 0xFFFFFFFF` annotation defeats a naive slot match on exactly the leave-region
stores.

Tree-wide 2026-08-19: presence mismatches are closed. State-count rows are divided
into INLINE_CUT, EXIT_MERGE, and STATE_FLOW; none alone is called a missing or extra
object. Without that distinction, StepArrivalDrop is falsely reported as a missing
object despite identical object construction/destruction evidence.
