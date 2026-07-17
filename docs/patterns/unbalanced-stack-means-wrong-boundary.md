# A "function" whose stack does not balance is NOT a function — check the boundary
tags: asm:push asm:pop asm:ret | topic:wall topic:boundary topic:scoring-artifact
symptoms: a small "function" that pushes N registers but pops N+k before `ret`; args that appear to "arrive already in callee-saved registers" with no loads; no rel32 caller, no vtable slot, no thunk pointing at it; a plateau in the 30-40% range that no source spelling moves
confidence: 10/10

**Before believing any "no source form reproduces this ABI" wall, balance the stack.**
It is arithmetic, it takes two minutes, and it is decisive. MSVC5 never invents a
calling convention. If a block's pushes and pops do not balance against its `ret N`,
the block is **not a function** — it is the interior/tail of a bigger one that some
tool (Ghidra, a trace) carved at the wrong address, and the reconstruction is modelling
a fragment.

## The mechanical test

Work **backwards** from the `ret`:

1. At `ret N`, `esp` must point at the return address (call it `E`).
2. Walk the epilogue backwards: each `pop` means `esp` was 4 lower before it.
3. That gives the `esp` the block must *reach* the epilogue with.
4. Walk forwards from the claimed entry (`esp = E`) through the pushes / `add esp,K` /
   callee-cleanup calls.
5. **Mismatch ⇒ the entry is wrong.** The size of the mismatch is the giveaway: a gap of
   `4*k` means the real function pushed `k` more registers, at an entry *earlier* than
   the one you were handed.

Then confirm it:

```sh
gruntz sema xref <rva>            # no rel32 caller at all?
# scan .rdata/.data for the pointer  -> not a vtable slot?
# scan the ILT band for a jmp        -> not a thunk target?
# scan BACKWARDS for a jcc/jmp landing on it  <-- rel8 jcc is easy to miss!
```
A block that **nothing references** but that is **jumped to from just above it** is an
interior block. Find the real entry by scanning back to the `cc cc` int3 padding; the
first byte after the padding is the true function start.

## Worked example (the one that produced this note)

`0x09356c` was carried for a long time as `CBrickzGrid::Serialize(i32,i32,i32,i32)`,
`RVA(0x0009356c, 0x38)`, parked `@early-stop` at ~38% behind a documented wall claiming
the four forwarded args "arrive already in the callee-saved registers, separate from the
stack args", and that "no natural C++ signature reproduces the register-resident-args
ABI". That pattern note was **fabricated** — it invented an ABI to explain the residue.

The bytes:
```asm
0009356c: push ebx ; push ebp ; push esi ; push edi   ; 4 pushes
00093570: call 0x17da                                  ; __cdecl
00093575: add  esp,0x10                                ; caller cleanup -> esp back to entry
...
0009357e: pop edi ; pop esi ; pop ebp ; pop ebx ; pop ecx   ; FIVE pops
00093583: ret  0x10
```
4 pushes, cancelled by `add esp,0x10`, then **five** pops: the block reaches its epilogue
`0x14` (five dwords) short of what `ret 0x10` needs. Impossible as an entry.

Assume instead it is a continuation of a function whose prologue pushed five registers —
and everything closes:
```asm
00093460: push ecx ; push ebx ; push ebp ; push esi ; push edi   ; esp = E0-0x14
          ... body loads ebx/ebp/esi/edi with the 4 args ...
00093562: jne 0x9356c                                   ; <-- rel8 jcc INTO the "function"
00093569: ret 0x10                                      ; an EARLY return, mid-function
0009356c: (the block)                                   ; esp = E0-0x14  -> balances exactly
```
`0x93460` is `CGruntzMgr::BroadcastCmd` — already reconstructed, in another unit, with
`RVA(0x00093460, 0x15c)` covering `0x93460..0x935bc`. The "function" at `0x9356c..0x935a4`
sat **entirely inside it**: the same 56 bytes claimed twice, by two names, in two units.

Every symptom then explains itself:
- args "arrive in callee-saved registers" — because **BroadcastCmd's earlier body loaded
  them**; that body is simply not in the fragment.
- `mov eax,[esp+0x10]; mov ecx,[eax+0x7c]` read as "arg3->m_7c" — under the real 5-push
  frame `[esp+0x10]` is the saved `ecx`, i.e. **`this`**, so it is plainly `this->m_7c`.
- no caller, no vtable slot, no thunk — because it was never a function.

Fix: delete the fragment; the real owner already models the bytes.

## The lesson

"No source form reproduces this" is a claim about the *compiler*. Before accepting it,
check the cheaper claim about the *tool*: that the address you were given is a function
at all.

## This is now gated (2026-07-17)

`verify_unique_names` used to enforce only one RVA per NAME — it never looked at
EXTENTS, which is how this fragment sat inside its parent, scored, indefinitely. It now
also fails on any two claims with overlapping `[rva, rva+size)` ranges (MSVC5 has no
`/OPT:ICF`, so no two functions ever share bytes). The first run found two more, both
over-declared sizes rather than phantoms:

| claim | was | is | what it ran into |
|---|---|---|---|
| `LaunchWebBrowser` 0x8f120 | `0x264` | `0x170` | its own int3 padding + all of `PollUnlessIdle` @0x8f2f0 (a real function) |
| `CMoviePlayer::InitMode` 0x17c3f0 | `0x14e` | `0x120` | 0x2e bytes into `Teardown` @0x17c510 |

So an overlap has two flavours, and the fix differs:
- **over-declared size** (common) — the `RVA()` size arg reaches past the function's real
  end. Find the end (the `ret` + the `cc cc` padding, or a 1-byte `90` alignment nop) and
  shrink the size.
- **phantom claim** (worse) — the inner "function" is a fragment of the outer one. Balance
  its stack per the test above; if it does not close, delete it.
