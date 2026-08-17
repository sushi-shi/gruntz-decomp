# cl 5.0 cross-jumps common SUFFIXES aggressively — a duplicate tail is your CFG's fault, not a compiler mood

The standing "block-placement coin" hypothesis reads the merge/duplicate split in
retail (`CPlay::ExecCommand` merges the whole `TOOL_AT` `res == -1` subtree into
`TOY_AT`'s with per-predecessor `push 2/3` + `jmp`, yet leaves byte-identical ON-pair
blocks duplicated; `CPlay::LoadCursorSprites` keeps 14 identical return epilogues)
as one unexplained C2 decision. It is not one decision, and most of it is not a coin.
The merge is a common-SUFFIX cross-jump with three measured blockers.

## What the pass does (and the retail signature it produces)

Probe: a `switch` with A arms whose bodies end in the same T-statement tail.

| arms | T=1 | T=2 | T=3 | T=4 | T=6 | T=8 | T=12 |
|---|---|---|---|---|---|---|---|
| 2 | 2 | **1** | **1** | **1** | **1** | **1** | **1** |
| 3 | 3 | **1** | **1** | **1** | **1** | **1** | **1** |
| 4 | 3 | **1** | **1** | **1** | **1** | **1** | **1** |
| 6 | 3 | **1** | **1** | **1** | **1** | **1** | **1** |

(copies of the tail that survive; **1** = fully merged). From two statements up, cl
merges every arm — and it does it by SPLITTING each arm at the divergence point,
leaving the differing prefix in place followed by a jump into the survivor. That
emitted shape is exactly retail's `ExecCommand` pattern:

```asm
; probe: four arms, arm 1 carries one extra store, arms 0/2/3 share the tail
0010: push 0x0            ; case 0 - only the differing prefix stays
0012: jmp  0x44           ;          ... then jump into the survivor
0014: push 0x1            ; case 1 - body differs, keeps its own copy
0016: call sink
...
003b: push 0x2            ; case 2
003d: jmp  0x44
003f: push 0x3            ; case 3 IS the survivor
0044: call sink
0049: mov  eax,[esp+0xc]
      mov  ds:g0,eax / ds:g1,eax / ds:g2,eax
      xor  eax,eax
      ret
```

So "per-predecessor `push <k>` + `jmp` into a shared tail" is not a special retail
construct to reverse-engineer — it is what this compiler emits whenever two paths
share a suffix.

## What BLOCKS it (three measured blockers)

**1. A join at the head of the common suffix.** If anything makes the block that
starts the identical tail have two predecessors, the merge does not happen. Five
controlled cells, four arms each, same tail:

| arm shape | tail block preds | result |
|---|---|---|
| straight line | 1 | **merged** |
| `if (a > k) { sink(k); return 5; }` then tail | 1 (branch exits) | **merged** |
| `if (a > k) goto out;` then tail | 1 | **merged** |
| two early `return`s then tail | 1 | **merged** |
| `if (a > k) { sink(k); }` then tail (**rejoins**) | 2 | **not merged** (4 copies) |
| `if/else` then tail | 2 | **not merged** (4 copies) |
| `for` loop then tail | 2 | **not merged** (4 copies) |

**2. Per-arm EH state.** A destructible local declared inside each arm gives every
arm its own unwind state and blocks the merge (4 copies, 400 B); the same object
hoisted to function scope — one unwind state for all arms — merges normally
(1 copy, 183 B).

**3. A suffix with no IL content.** The epilogue (`pop`s + `ret`) is emitted after
this pass, so two paths whose only common suffix IS the epilogue have nothing to
merge. **This is `CPlay::LoadCursorSprites` (retail `0x000d0120`) exactly**: each
arm is

```asm
0d02c9: test eax,eax
0d02cb: jne  0xd06bd        ; the shared FAILURE path - merged, once
0d02d1: pop  edi
0d02d2: pop  esi
0d02d3: pop  ebp
0d02d4: pop  ebx
0d02d5: ret  0x8            ; the "success" tail: pure epilogue -> 14 copies
```

The return value is already in EAX from the call, so `return 1`'s IL suffix is
EMPTY; there is no tuple to cross-jump, and the 14 identical epilogues are the
expected output, not a placement coin. Note the same function merges its shared
failure path into one block at `0xd06bd` — the pass is running, and working.

## What does NOT block it (refuted)

* **Adjacency.** Interleaved identical tails (arms 0/2 share tail A, arms 1/3 share
  tail B) merge exactly as well as adjacent pairs — both emit 116 B with one copy
  of each tail. The pass is not a peephole over neighbouring blocks.
* **Arm count** (2, 3, 4, 6 all merge), **case-label density** (dense `0..3` and
  sparse `0,37,74,111` merge identically), **dispatch form** (`switch` jump table
  and an `else if` chain both merge), **calls inside the tail**, and **tail length**
  above two statements.

## What this says about the three open Gruntz cases

* **`CPlay::LoadCursorSprites` `0x000d0120`** — SOLVED, and not a defect: blocker 3.
  Do not chase the 14 epilogues.
* **`CPlay::ExecCommand` `0x000d1b60`** — RECLASSIFIED. Scanning our fresh compile
  for repeated instruction runs, the AT-pair duplicates are **not common suffixes**:
  the longest repeats (14-26 instructions, at `0x0344`/`0x089c`, `0x0350`/`0x04ef`/
  `0x069c`/`0x08a8`, `0x0574`/`0x0939`) are identical PREFIXES that then diverge —
  several end on `je` to the same target and continue differently. Cross-jumping
  merges suffixes, so no pass would ever fold these. A missing merge here is
  therefore evidence of a **CFG reconstruction difference** (our arms converge
  somewhere retail's do not), which is a source question, and the earlier refuted
  experiment — hoisting shared locals to force C1-tree identity — failed because it
  addressed neither blocker.
* **`CGrunt::LoadGruntTypeTable` `0x0004dd50`** — this one IS the residual coin, and
  it is narrow: three `PICKUP_HEALTH{1,2,3}` arms differing only in a string
  literal, all with a rejoining `if` INSIDE the merged region (which is fine — only
  a join at the HEAD blocks). cl merges all three; retail merges two. The probe
  reproduces partial merging only at the very bottom of the size range (T=1 tails:
  4 and 6 arms both collapse to 3 copies, not 1), so what is left is a
  pairwise savings/threshold decision, not the merge rule itself.

## Where to look in c2.exe — and a general tool for the campaign

`c2.exe` ships its own **source file names** (assert strings), so any c2 code
region can be attributed to a NAMED compiler pass. 44 names are present:
`regasg.c`, `color.c`, `globopt.c`, `globlopt.c`, `sizeopt.c`, `factor.c`,
`optimize.c`, `inline.c`, `dag.c`, `fg.c`, `lg.c`, `switch.c`, `stack.c`,
`except.c`, `ehexcept.c`, `emit.c`, `coff.c`, `coffemit.c`, `reader.c`,
`p2symtab.c`, `tuple.c`, `sdsu.c`, `hash.c`, `x86\code.c`, `x86\lower.c`,
`x86\schedmd.c`, `x86\fppeeps.c`, `x86\ehgen.c`, `x86\MDmisc.c`, …

Recipe (verifiable in Ghidra at the default 0x400000 base): find the string, find
the `push <string VA>` sites — those are the asserts, sitting in the OUTLINED COLD
region above `0x00465000` — then find the hot `jcc`/`jmp` that lands in that cold
block. The hot side is the pass:

| pass file | string VA | assert site | hot code that jumps to it |
|---|---|---|---|
| `sizeopt.c` | `0x0049e0c4` | `0x004679cd` | `FUN_004104dd` (`0x004104f7`, `0x0041050e`, `0x00410541`, `0x00410573`, `0x00410673`) |
| `factor.c` | `0x0049e0fc` | `0x00470ef5` | `FUN_00444e1e` (`0x00444e53`, `0x00444e8a`, `0x00444f06`, `0x00444f97`) |
| `regasg.c` | `0x0049ddcc` | `0x0046f327`, `0x004784ee` | `FUN_00435b1d` (`0x00435b3f`, `0x00435c0d`, `0x00435e95`) and `FUN_0045d33f` |
| `color.c` | `0x0049dfb4` | `0x004676b8`, `0x0046915f`, `0x004692bc` | `0x0040ecfe`, `0x0041a945`, `0x0041bee2` |
| `inline.c` | `0x0049e144` | `0x004721f7`, `0x004722c5` | `0x0044b759`, `0x0044b909` |
| `globlopt.c` | `0x0049e07c` | `0x0046fb74`, `0x00472761` | `0x0046fe08`, `0x0046ff1d` |

Two things follow immediately. `FUN_00435b1d` is a **`regasg.c`** function, and it
is one of the readers of the register table `DAT_00491100` (`0x00435c02`,
`0x00435dbe`, `0x00435dcc`) — independent confirmation that the rotating-cursor
machinery in
[`cl5-c2-register-picker-is-a-rotating-cursor`](cl5-c2-register-picker-is-a-rotating-cursor.md)
is the register-ASSIGNMENT pass and not something else. And the cross-jump this
entry characterizes lives in `sizeopt.c` / `factor.c` — "factoring" being the
period name for exactly this transformation — which is where the residual
partial-merge threshold has to be read.

## The refutable prediction

**A duplicated tail in our output where retail merged (or the reverse) is NOT a
placement coin — it is one of: (a) a join immediately before the tail that retail
does not have, (b) a destructible local scoped per-arm where retail scoped it once,
or (c) the "identical" runs are prefixes, not suffixes, i.e. the CFG differs.**
Check (c) first: if the repeated runs end on branches to different continuations,
no compiler setting would ever fold them. Only after (a), (b) and (c) are excluded
is a residual merge difference a real coin — and the only case in this corpus that
survives all three is `LoadGruntTypeTable`'s third arm.

## Bounds

Measured 2026-08-17, pinned cl 5.0 SP3, `/O2 /MT /GX /GR`. Probe TUs were scratch
(never in the build graph) and are deleted; the repeated-run scan reads a fresh
compile of `src/Gruntz/Play.cpp` and needs no build. The partial-merge threshold
(what makes cl fold 4 tiny tails into 3 copies instead of 1, and retail fold two
of three `PICKUP_HEALTH` arms) is NOT derived — that is the open residue, and
`sizeopt.c`/`factor.c` above is where it is.
