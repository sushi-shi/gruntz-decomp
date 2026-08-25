# Wall reasons: code layout, control flow, frames, EH and switch lowering

A catalogue of the cl 5.0 decisions that produce the campaign's layout-shaped walls,
each reverse-engineered from the compiler and stated so a matcher can act on it.
Companion to [`cl5-crossjump-merges-suffixes-not-blocks`](cl5-crossjump-merges-suffixes-not-blocks.md)
(which this entry partly CORRECTS),
[`cl5-c2-register-picker-is-a-rotating-cursor`](cl5-c2-register-picker-is-a-rotating-cursor.md)
and [`cl5-inline-budget-is-arithmetic-you-can-compute`](cl5-inline-budget-is-arithmetic-you-can-compute.md).

All measurements 2026-08-18, pinned cl 5.0 SP3 under wine, `/O2 /MT /GX /GR` unless
stated. Probe TUs were scratch (never in the build graph), are embedded below, and are
deleted. `src/` unchanged.

---

## 0. Tooling correction: the pass-name oracle needs `mov`, not `push`

`cl5-crossjump-merges-suffixes-not-blocks` describes the assert-string recipe as
"find the `push <string VA>` sites". That form finds only **3** of the 44 files
(`optimize.c`, `reader.c`, `getattr.c`). The real form c2.exe uses is a
**register-argument assert that carries the LINE NUMBER**:

```asm
4679c7: mov  edx, 0x43          ; <- sizeopt.c LINE 67
4679cc: mov  ecx, 0x49e0c4      ; <- "E:\utc\src\\P2\sizeopt.c"
4679d1: call 0x486b3c           ; assert(file, line)
4679d6: jmp  0x410679           ; <- back into the HOT code: that is the pass
```

Scanning for `b9 <fileVA>` (opcode `mov ecx,imm32`), reading `ba <line>` five bytes
back and the `jmp` after the `call`, yields file+line+hot-address for 117 sites.
The anchors this entry uses:

| file | hot address | line |
|---|---|---|
| `sizeopt.c` | `0x00410679` | 67 |
| `lg.c` | `0x0043aafc` | 834 |
| `stack.c` | `0x0043f9a1` | 878 |
| `switch.c` | `0x0044957e` | 167 |
| `factor.c` | `0x00444f97` (assert `0x00470eef`) | 491 |
| `except.c` | `0x0046423b`, `0x0046426e` | 1053, 1059 |
| `ehgen.c` | `0x00464f18` | 297 |
| `fg.c` | assert INLINE at `0x004876ab`, function ends `0x004876bc` | 4285 |

Two structural facts fall out and both matter for further RE:

* **c2.exe is NOT laid out one-obj-per-contiguous-range.** `coff.c` sites appear at
  `0x41fa22`, `0x43435b`, `0x44a83f`, `0x453bd7`, `0x474c4b`. Do not bound a pass by
  address range; use the per-site anchors.
* `factor.c`'s only reachable function from `sizeopt`'s side is `FUN_00444e1e`, a
  two-operand equality predicate (`ret 8`), whose single caller `0x0040901f` builds
  NEW tuples (`0x004449b2`, opcode `0x151`) out of two predecessors. `factor.c` is
  **expression factoring across predecessors**, not block-level cross-jumping — see
  entry 1.

---

## 1. REASON — the unconditional common-tail cross-jump is `/Os`-GATED and is NOT RUNNING in Gruntz

**PASS + ADDRESS.** `sizeopt.c` — driver at `0x00410653` (called from `0x004133c4`,
once per function), iterating two tuple transforms `0x0041068e` and `0x00410a57` to a
fixpoint with a 50-iteration assert at `0x00410670`/`0x00410679`. Under `/Ot` this
driver's merging arm does not fire. What still merges under `/O2` is the global
optimizer's own factoring (`factor.c` predicate `0x00444e1e`, merge `0x004449b2`).

**WHAT DECIDES IT.** The size/speed flag, and nothing in the source. Same eight-arm
switch, same tail, only the flag changed (arms that merged, out of 7 possible):

| tail statement | `/O2` | `/Ox` | `/Og` | `/Og /Ot` | **`/Og /Os`** | `/O1` |
|---|---|---|---|---|---|---|
| `g0 = g1;` | 0 | 0 | 0 | 0 | **7** | 7 |
| `g0 = 5;` | 0 | 0 | 0 | 0 | **7** | 7 |
| `g0 = ext();` | 0 | 0 | 0 | 0 | **7** | 7 |
| `g0 = b;` | 5 | 5 | 5 | 5 | **7** | 7 |
| `g0 = b+1;` | 2 | 2 | 2 | 2 | **7** | 7 |
| `++g0;` | 7 | 7 | 7 | 7 | **7** | 7 |

`/Os` merges every arm regardless of tail content; `/Ot` (which `/O2` implies) does
not. Gruntz is `/O2`, so **the "merge every common suffix" model does not describe our
compiler.** What we get instead is content-sensitive.

**Under `/O2` the survivor count is NOT decided by machine identity.** Three
refutations, all eight-arm dense switches, all arms written identically:

* `g0 = ext();` — every arm emits the byte-identical 5-instruction suffix
  `call sink / add esp,4 / call ext / mov [g0],eax / xor eax,eax / ret`.
  **Zero merges.** Byte-identical is not sufficient.
* `g0 = g1;` at 4 arms — arms 0 and 3 both allocate EAX and are byte-identical.
  **Not merged.** Meanwhile `g0 = b;` at 4 arms, whose arms 0 and 3 are also both EAX,
  **is** merged. The only difference in the two suffixes is `mov eax,[esp+0xc]` versus
  `mov eax,[g1]`.
* `g0 = 5;` at 8 arms — eight byte-identical 21-byte suffixes, **zero merges**; while
  `++g0;`, a *shorter* 22-byte suffix, merges all seven. Not size, not instruction
  count, not relocation count.

The full measured grid (8 arms, one statement unless noted; "copies" = surviving tail
blocks):

| tail | merges | copies | emitted suffix |
|---|---|---|---|
| `++g0;` / `--g0;` / `g0=g0+1;` / `g0^=1;` | 7 | 1 | load-modify-store on one global |
| `g0=b;` / `g0=c;` / `g0=g1+1;` / `g0+=g1;` | 5 | 3 | |
| `gp->x=g1;` / `g0=g1+g2;` | 4 | 4 | |
| `gp->x=b;` | 3 | 5 | |
| `g0=b+1;` / `gp->x=5;` / `g0=g1; g2=g3;` | 2 | 6 | |
| `g0=g1;` / `g1=g0;` / `g0=5;` / `g0=0;` / `g0=ext();` / `sink(99);` | 0 | 8 | |
| `g0=b; g1=b;` / `g0=5; g1=6;` (2 statements) | 7 | 1 | |

**SOURCE-REACHABLE?** **PARTIAL, and the domain is narrow.** Two statements in the
common tail merge everything in most spellings (that is R1's `T>=2` row and it holds);
one statement is a coin whose bias is set by the operand kinds above. The `/Os` gate
itself is not reachable — the project is `/O2` and changing it is not a modelling act.

**DETECTION SIGNATURE.** N identical arm tails in retail collapsed to k<N blocks with
per-predecessor `jmp` into the survivor, while our build keeps N (or the reverse), AND
the tail is a SINGLE statement. If the tail is two or more statements and we differ,
suspect R1's blockers (a join at the suffix head, a per-arm EH state — see entry 6) or
a CFG difference, not this.

**WORKED EXAMPLE.** `CGrunt::LoadGruntTypeTable` `0x0004dd50` — three
`PICKUP_HEALTH{1,2,3}` arms differing only in a string literal; cl merges three, retail
merges two. Under the grid above that is exactly the one-statement regime, and the
operand kind (a pooled `??_C@` string address vs. a stack value) is the kind of
difference that moves the count. Probe that reproduces the whole class in 0.2 s:

```cpp
extern void sink(int); extern int ext(); extern int g0,g1,g2,g3;
int f(int a,int b,int c){
    switch(a){
    case 0: sink(0); g0 = b; return 0;      // ... repeat for 8 arms
    }
    return 7;
}
```

**RESIDUE.** What selects the survivor count in the one-statement regime is still
open. It is NOT: machine identity, suffix bytes, suffix instruction count, relocation
count, arm count, register class, processor model (`/G3 /G4 /G5 /G6 /GB` all identical),
or `/Gy`. It IS below `/Og` and above the emitter. The next place to look is the
`factor.c` merge at `0x004449b2` and its caller `0x0040901f`, which is value-based:
it compares two predecessors' *operands* (`FUN_00444e1e`) and synthesizes a shared
tuple. That framing predicts exactly the observed content-sensitivity (a call result
and a fresh constant have no shared value; a load-modify-store of one global does),
and it is the hypothesis to test next.

---

## 2. REASON — block order is TOPOLOGICAL: a block is emitted after its LAST predecessor

**PASS + ADDRESS.** Flow graph / layout: `fg.c` (assert inline at `0x004876ab`,
line 4285) and `lg.c` (`0x0043aafc`, line 834). `lg.c` is the **loop** graph — the code
around its anchor walks a tree by `[node+4]`/`[node]`, compares `word [x+0x84]` nest
levels and writes `word [edi+0x86]`, and `0x0043aad7` splices a fresh block in front of
a loop header (pre-header insertion). Loops are what `lg.c` protects, and that is
exactly the exception in the rule below.

**WHAT DECIDES IT.** The CFG, and only the CFG. Measured on one probe, four regions
A/B/C/D marked by their first call argument, varying ONLY where the backward `goto`
comes from:

| source shape | emitted order |
|---|---|
| no backward goto | **A B C** |
| `goto A` from **B** only | **B A C** |
| `goto A` from **C** only | **B C A** |
| `goto A` from **D** only (4 regions) | **B C D A** |
| `goto A` from B **and** C **and** D | **B C D A** |
| `goto B` from D (B is the 2nd region) | **A B C D B** |
| `while` loop over B,C (a real cycle) | **A B C D** |
| A is in a **cycle** (A reaches its own backward predecessor) | **A B C A** |

`B A C` is the load-bearing cell: A is not sunk to the end, it is placed **immediately
after its last predecessor**. cl orders the block list so every non-loop edge points
forward. A backward `goto` in source is a *forward* edge in the CFG (the target
returns, so there is no cycle) and cl re-lays it out accordingly.

Two consequences, both measured:

* **The merge runs BEFORE layout, and that is what makes duplication a real lever.**
  Writing the region out at each exit instead of `goto`-ing back to it moves the order
  to retail's **A B C** — *provided the copies do not merge*. Region A of k statements,
  same three-region skeleton, `goto` vs. written-out:

  | k statements in A | 0 | 1 | 2 | 4 | 8 | 16 |
  |---|---|---|---|---|---|---|
  | two backward `goto`s | A B C A | B C A | B C A | B C A | B C A | B C A |
  | written out, copies **differ** | A B C | **A B C** | **A B C** | **A B C** | **A B C** | **A B C** |
  | written out, copies **identical** | A B C A | B C A | B C A | B C A | B C A | B C A |

  Identical copies merge first, the merged block inherits the backward predecessors,
  and it sinks — which is why the naive duplication test is byte-identical to the
  `goto` (`sha1 8e0aaf63…`, 10 relocations each, empty `llvm-objdump -d` diff). One
  statement of difference between the copies is enough to keep them apart and keep
  A first.
* **A destructible local does NOT flip this.** Adding `L probe(10);` (a class with a
  dtor) inside region A, or at function scope, leaves the order at `B C A`. The
  measured flip recorded in `backward-goto-sinks-its-target-region.md` is therefore
  not caused by the EH scope in the minimal shape and needs a different explanation in
  `StepArrivalDrop` specifically.

**The rule is measurable on retail, and it localizes the wall.** Build each retail
function's blocks from `gruntz sema disasm`, add branch and fall-through edges, and
count edges `b <- p` where `p > b` and `b` cannot reach `p`. Over 25 wall-inventory
functions (1994 blocks):

| violations | function |
|---|---|
| 11 | `CGrunt::StepArrivalDrop` `0x0004b370` (into `0x4b4ff`, `0x4b605`) |
| 9 | **`CTriggerMgr::PlaceObjectFull` `0x00078a50`** (all into `0x78e9b`) |
| 7 | `CGrunt::StepCompassMove` `0x00051c00` (all into `0x521e1`) |
| 1 | `CGrunt::LoadGruntCombatAnimations` `0x000597a0`, `CGrunt::StepDiggerBehavior` `0x000f36a0`, `CBattlezMapConfig::RepathAroundBlockedTiles` `0x0002a570` |
| **0** | the other **19**, including `CGrunt::StepSmartChaserBehavior` (385 blocks), `CGrunt::LoadGruntTypeTable` (502 blocks), `CPlay::ExecuteCommand`, `CMapMgr::ComputeCellFlags`, `CGrunt::StepGruntMovement` |

Zero violations is the normal case even at 500 blocks. A nonzero count is not noise:
the three heaviest are exactly the campaign's known layout walls, all violations funnel
into ONE or TWO target blocks, and `PlaceObjectFull` is a member of the family that
nothing had named. The count IS the worklist, and the named target block is where the
layout/factoring difference is concentrated.

**2026-08-19 scope correction:** the topological rule applies to the CFG **entering
layout**, not necessarily the final emitted CFG. Value factoring can create a backward
edge after the survivor block has already been placed. Therefore a violation in retail
localizes the residue but does not, by itself, prove that retail source contained a
cycle or even a `goto`. That reverse inference must be tested with the full function.

**SOURCE-REACHABLE IN THE MINIMAL PROBE?** **LEVER**, with two spellings that work and
a clear set that does not:

1. **Write the region out at each exit instead of `goto`-ing into it**, keeping the
   copies distinguishable so they do not merge before layout. Measured A B C at every
   region size from 1 to 16 statements.
2. **Close a real cycle**: `for(;;) { A ...; if (done) return r; B ...; }` where A is
   reachable from B *and* B from A puts A first, and cl then duplicates the rotated
   head — the `A B C A` cell.

Cosmetic spellings do not move it and every one was measured flat: un-nesting the `if`,
moving the label, `for(;;)`+`continue` that targets a *different* head, an `else` arm,
heap-vs-stack for a destructible local, and a dtor local anywhere in the function.

**DETECTION SIGNATURE.** Extract each side's block order and each block's predecessor
addresses. If retail places a region **before** blocks that jump back into it, while the
candidate sinks it after those predecessors, the region is a layout/factoring wall.
Do not infer the pre-layout source CFG from the final backward edge: test duplication
and cycle spellings against the full call, relocation, branch and instruction census.

**WORKED EXAMPLE.** `CGrunt::StepArrivalDrop` `0x0004b370`. Retail's `pathGate` region
head is `0x0004b4ff`; it is entered forward from the gate (`0x0004b4e1 je 0x4b4ff`) and
backward from the late regions above `0x0004b7a1` —

```asm
04bdd6: 8b 83 28 03 00 00      mov eax,DWORD PTR [ebx+0x328]
04bddc: 85 c0                  test eax,eax
04bdde: 0f 84 1b f7 ff ff      je 0x4b4ff        ; backward, into the region head
...
04b4ff: 8b 5c 24 10            mov ebx,DWORD PTR [esp+0x10]
04b503: c7 44 24 20 01 00 00   mov DWORD PTR [esp+0x20],0x1
04b50b: 8b 83 28 03 00 00      mov eax,DWORD PTR [ebx+0x328]   ; the same field, reloaded
```

— and it is emitted **before** them. In the final retail CFG, however, `0x4b4ff` cannot
reach any of its late predecessors: an SCC walk finds no cycle through the path region.
Those backward edges are compatible with post-layout factoring and are not a source
cycle oracle.

The full-function controls reject both naive applications of the minimal-probe levers:

* a `pathFound` loop with the two late successes expressed as `continue` restores the
  exact 26-call/68-relocation census, but cl still emits the path region last and
  rotates/duplicates the line-scan prefixes: `0xdd0`-`0xde8`, 1001-1007 instructions,
  130 branches, versus retail `0xb28`, 853, 129;
* writing the entire path region at all three success sites emits three independent
  `CPtrList` EH regions rather than merging them: `0x1088`, 44 calls, 106 relocations,
  189 branches.

The current clean reconstruction remains `0xb88`, 877 instructions, 25 calls, 64
relocations and 131 branches. Its missing fourth `RemoveHead` is a factored initial /
re-probe tail, not evidence of a missing call in source. The residual is bounded as a
full-function layout/factoring wall; neither a source cycle nor triplication is proved.
The minimal probe below still establishes the generic rule and its two levers:

```cpp
extern int Probe(int,int); extern int Count(); extern void Pop();
extern void A1(int); extern void A2(int); extern void B1(int); extern void B2(int);
extern void C1(int); extern void C2x(int);
int f(int a,int b,int n){                    // -> B C A   (region A sunk)
    if (Probe(a,b)) { if (Count()) Pop();
 pathGate: A1(0x1111); A2(0x2222); return 1; }
    B1(0x3333); B2(0x4444); if (n) goto pathGate;
    C1(0x5555); C2x(0x6666); goto pathGate;
}
int g(int a,int b,int n){                    // -> A B C A (region A first, head rotated)
    int t = Probe(a,b);
 pathGate: A1(0x1111); A2(0x2222); if (t) return 1;
    B1(0x3333); B2(0x4444); if (n) goto pathGate;
    C1(0x5555); C2x(0x6666); goto pathGate;
}
```

---

## 3. REASON — switch dispatch is a SIZE comparison between a direct table and a byte map, and the byte-map rule is exact

**PASS + ADDRESS.** `switch.c` — anchor `0x0044957e` (line 167), inside `FUN_0044956a`;
its caller `0x0044954c` calls `0x0044956a` then `0x0044965e` (three call sites:
`0x00449562`, `0x004497b3`, `0x004497d9`).

**WHAT DECIDES IT.** Two independent decisions, and only the second has a clean closed
form.

**(a) compare chain vs. table.** `n <= 3` distinct case values is ALWAYS a compare
chain. From `n >= 4` cl will take a table, up to a maximum span that grows roughly
linearly in the number of arms. Measured maximum `range` (`max-min+1`) that still
produces a table, evenly spread case values:

| n | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 10 | 12 | 16 | 20 | 24 | 32 | 48 | 50 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| max range | 1 | 2 | 255 | 255 | 425 | 620 | 594 | 764 | 934 | 1273 | 1612 | 1952 | 2630 | 3988 | 4157 |

The n>=8 series is linear at **~84.8 per arm** (`floor(509*(n-1)/6)+1` reproduces every
even-n cell exactly), but `n=7` (620) exceeds `n=8` (594) and `n=4,5` sit at a flat 255,
so the boundary is a cost comparison sensitive to the actual value distribution, not a
closed form. Treat the table as a bracket: below ~85·(n−1) expect a table, well above it
expect a chain, and read the retail dispatch for anything near the line.

**(b) direct table vs. two-level xlat byte map.** This one IS exact. With `range` the
span and `t` the number of distinct jump targets:

> **cl emits the byte map + small table iff `3·range > 4·t + 12`.**

i.e. it compares `4·range` (direct table) against `range + 4·t + 12` (byte map, target
table, and the extra `xor/mov cl/jmp` dispatch) and takes the smaller. Verified against
the first-XLAT boundary for seven group sizes, contiguous groups, dense range:

| t | 2 | 3 | 4 | 6 | 8 | 12 | 16 |
|---|---|---|---|---|---|---|---|
| predicted first XLAT range | 7 | 9 | 10 | 13 | 15 | 21 | 26 |
| measured TABLE at | 6 | 8 | 8 | 12 | 12 | 20 | 24 |
| measured XLAT at | 8 | 10 | **10** | 16 | 16 | 24 | 32 |

**SOURCE-REACHABLE?** **LEVER.** `t` is set by the source: `case A: case B: body` is one
target, `case A: body break; case B: body break;` is two — which is why the existing
`switch-case-order-is-arm-block-order.md` measured relocs collapsing 54 -> 28 when
per-case duplicate bodies were "cleaned up" into groups. Now it is predictable rather
than discovered: compute `t` and `range` from the retail dispatch and pick the case
grouping that lands on the same side of `3·range > 4·t + 12`.

**DETECTION SIGNATURE.** Retail has `mov al,BYTE PTR [edx+<map>]` +
`jmp DWORD PTR [eax*4+<table>]` and we have a bare `jmp DWORD PTR [eax*4+<table>]`
(or vice versa); relocation count changes by a large multiple; the dispatch head's
`cmp` bound is the same on both sides.

**WORKED EXAMPLE.** `CMapMgr::ComputeCellFlags` `0x00077790`:

```asm
077832: 81 fa 99 00 00 00      cmp edx,0x99                    ; range = 0x9a = 154
077840: 8a 82 e0 7b 47 00      mov al,BYTE PTR [edx+0x477be0]  ; 154-byte map
077846: ff 24 85 10 7b 47 00   jmp DWORD PTR [eax*4+0x477b10]  ; target table
```

Direct table would be `4·154 = 616` bytes; the map form is `154 + 4·t + 12`. With any
`t` under 150 the rule fires, and retail is XLAT. Prediction confirmed on retail without
a build.

---

## 4. REASON — frame slots are assigned first-declared-at-the-top, remainder bottom-up in FIRST-USE order

**PASS + ADDRESS.** `stack.c` — anchor `0x0043f9a1` (line 878).

**WHAT DECIDES IT.** Declaration order and first-use order together. Three
address-taken `int`s, all six declaration permutations crossed with two use orders,
offsets relative to the post-prologue frame base:

| decl | use | frame | a | b | c |
|---|---|---|---|---|---|
| abc | abc | 0xc | **-0x4** | -0xc | -0x8 |
| abc | cba | 0xc | **-0x4** | -0x8 | -0xc |
| bac | abc | 0xc | -0xc | **-0x4** | -0x8 |
| bac | cba | 0xc | -0x8 | **-0x4** | -0xc |
| cab | abc | 0xc | -0xc | -0x8 | **-0x4** |
| cba | cba | 0xc | -0x8 | -0xc | **-0x4** |

The first-declared local always takes `-0x4`; the remaining two are laid out from the
**bottom of the frame upward in first-use order** (first used gets the lowest address).
With four locals the split moves: the first **two** declared take `-0x4`/`-0x8` and the
remaining two are still bottom-up by first use (`abcd`/`abcd` gives
a=-0x4, b=-0x8, c=-0x10, d=-0xc; `abcd`/`dcba` gives c=-0xc, d=-0x10 — all four cells
predicted correctly).

**Frame SIZE never moved under any permutation**, and that is the separately useful
half. Measured across the 12 three-scalar cells above plus mixed kinds:

| shape | frame |
|---|---|
| `int a,b,c,d` / `int d,c,b,a` | `0x10` / `0x10` |
| `int a; int p[3]; int b` / `int p[3]; int a,b` / `int a,b; int p[3]` | `0x14` / `0x14` / `0x14` |
| `char x; int a; char y` / `char x,y; int a` | `0x8` / `0x8` |

**SOURCE-REACHABLE?** **PARTIAL.** Order is reachable (declaration order and the order
of first use both move slots, deterministically). Frame *size* is not a knob at all —
it is the sum of the objects that need a home. So a size mismatch is a modelling
question and an offset-order mismatch is a spelling question, and they must be
diagnosed separately.

**DETECTION SIGNATURE.** *Frame 8 bytes larger in retail* = retail homes two more
dwords: look for an unmodelled local, an aggregate we spelled as scalars, or a value
whose address retail takes. *Same frame size, permuted offsets* = our declaration or
first-use order differs; permute rather than re-model.

**WORKED EXAMPLE.** The wall family `CGrunt::StepGruntMovement` `0x0004c170`,
`CGrunt::Place` `0x0004d800`, `FontRenderer::MeasureWrapped` `0x0017ad10` is filed as
"retail's frame is 8 bytes larger and homes X on the stack where we hold it in a
register". Under the split above that is the SIZE half, not the order half: no
permutation of declarations changes frame size (measured flat across 12 permutations),
so these three are missing-object rows, not slot-order rows. Probe:

```cpp
extern void U(int*);
int f(){ int a,b,c; U(&a); U(&b); U(&c); return a+b+c; }   // a=-4, b=-0xc, c=-8
```

---

## 5. REASON — every destructible object gets its own EH state index, numbered in LEXICAL order across the whole function

**PASS + ADDRESS.** `except.c` (`0x0046423b`/`0x0046426e`, lines 1053/1059),
`ehexcept.c` (`0x00456993`, `0x00456b5b`, `0x004574e3`, `0x004597ad`),
`x86\ehgen.c` (`0x00464f18`, line 297).

**WHAT DECIDES IT.** Source lexical order of the objects, not scope nesting and not
mutual exclusivity. Measured state-variable writes (`mov dword ptr [esp+N], <state>`):

| source | states written |
|---|---|
| one `L a(1);` at function scope | `0`, `-1` |
| `L a(1); M b;` at function scope | `0`, `1`, `0`, `-1` (nested, unwound in reverse) |
| `{ L a(1); } { M b; }` — two DISJOINT scopes | `0`, `-1`, then **`1`**, `-1` |
| one local **per switch arm**, 3 arms | **`2`,`-1` / `1`,`-1` / `0`,`-1`** |
| one local **hoisted above** the switch | `0`, `-1` only |

Disjoint scopes do **not** reuse state 0. Three mutually exclusive switch arms consume
three state indices. That is the mechanism behind R1's measured blocker 2: per-arm
destructible locals give each arm a *different immediate* in the state store, so the
arms' tails are not identical and no merge is possible — the blocker is not "EH is
special", it is "the state number is part of the code".

**Why an EH funclet's exactness adjudicates frame layout.** The unwind funclet emitted
into `.text$x` addresses the object through the FRAME POINTER at its assigned slot:

```asm
Disassembly of section .text$x:
   0: lea ecx, [ebp - 0x10]     ; <- the destructible object's frame slot, verbatim
   3: jmp <dtor>
```

The funclet therefore encodes the slot offset from entry 4. A funclet that is byte-exact
proves the object's slot; a funclet that regresses under a source hoist (the measured
`__ehunwind$?Load@CTriggerMgr` 100 -> 99.80) is reporting that the hoist moved the slot.
Read the funclet first — it is a one-instruction oracle for a frame question that is
otherwise buried in a large function.

**SOURCE-REACHABLE?** **LEVER.** The state index is a pure function of how many
destructible objects precede this one lexically. Move a declaration, or add/remove one,
and every later state renumbers. This is also the reason a single wrong `CString` or
container local shifts every state immediate in the rest of the function.

**DETECTION SIGNATURE.** Retail's state immediates are a contiguous `0..k` and ours are
`0..k'` with k != k' -> the SET of destructible objects differs (count is exact evidence).
Same count, permuted immediates -> declaration order differs. `-1` writes always
outnumber the distinct states by the number of exits.

**WORKED EXAMPLE.** The blocker case, reproduced in six lines:

```cpp
struct L { L(int); ~L(); int v; }; extern void use(int);
int f(int k){ switch(k){                 // states 2 / 1 / 0 : arms cannot merge
  case 0:{L a(1);use(a.v);}break; case 1:{L a(2);use(a.v);}break; case 2:{L a(3);use(a.v);}break;}
  return 0; }
int g(int k){ L a(1); switch(k){         // one state 0 : arms merge normally
  case 0:use(a.v);break; case 1:use(a.v+1);break; case 2:use(a.v+2);break;}
  return 0; }
```

---

## Verdict on the "block-placement coin" hypothesis

**It is not one coin, and most of it is not a coin at all.** The family that
`c2-block-placement-coin` collects splits into four distinct, separately diagnosable
mechanisms:

1. **Block ORDER (entry 2)** — determined by the pre-layout CFG: after the last
   predecessor, cycles exempt. **19 of 25 sampled retail wall functions obey it with ZERO
   violations**, including two above 200 blocks; the three heaviest violators are the
   three known layout walls and every violating edge funnels into one or two named
   blocks. Deterministic and a measured source-level lever in the minimal probe, but a
   final retail edge may have been introduced by factoring after placement.
2. **Tail MERGE (entry 1)** — content-sensitive under `/O2` because the `/Os`
   cross-jumper is off and only value-based factoring runs. A coin only in the
   one-statement regime; two-statement tails are deterministic.
3. **Epilogue duplication** — `LoadCursorSprites` `0x000d0120`'s 14 identical returns.
   Already solved by R1 (an empty IL suffix), and entry 1 explains why no `/O2` pass
   would touch it either.
4. **Frame slot ORDER (entry 4)** — deterministic in declaration and first-use order;
   distinct from frame SIZE, which is a modelling question.

The remaining genuinely open residue is narrow and named in entry 1's RESIDUE: what
selects the survivor count for a ONE-statement common tail under `/O2`.

## Bounds

Every table above is a fresh compile of an embedded probe with the pinned cl 5.0 SP3;
the c2.exe addresses are statically checkable in Ghidra at the default `0x400000` base.
The chain-vs-table series in entry 3(a) uses evenly spread case values and is a bracket,
not a formula; the xlat rule in 3(b) and the frame rule in entry 4 are exact over their
measured grids. The old `StepArrivalDrop` prediction ("retail must contain a real
cycle") is falsified by its emitted SCCs and the full-function loop controls above.

## Reproduce it

```sh
# the pass-name + LINE map (no build, no Ghidra)
python3 - <<'PY'
import os,re,struct
d=open(os.environ['MSVC_DIR']+'/bin/c2.exe','rb').read()
TVA,TOFF,TSZ=0x401000,0x600,0x8beff; DVA,DOFF=0x48e000,0x8d200
text=d[TOFF:TOFF+TSZ]
for m in re.finditer(rb'E:\\utc\\src[ -~]{2,40}\.c\x00', d[DOFF:DOFF+0x10400]):
    va=DVA+m.start(); nm=m.group()[:-1].decode().split('\\')[-1]
    off=0
    while True:
        i=text.find(b'\xb9'+struct.pack('<I',va),off)
        if i<0: break
        off=i+1
        line=struct.unpack('<I',text[i-4:i])[0] if text[i-5]==0xba else None
        j=i+10; back=TVA+j+5+struct.unpack('<i',text[j+1:j+5])[0] if text[j]==0xe9 else None
        print(f"{nm:12s} line={line} assert={TVA+i:#x} hot={back and hex(back)}")
PY

# any probe in this entry, ~0.2 s each
wine $MSVC_DIR/bin/cl.exe /nologo /c /O2 /MT /GX /GR probe.cpp
llvm-objdump -d -M intel --no-show-raw-insn probe.obj
```

The entry-2 census is ~40 lines over `gruntz sema disasm` and needs no build: split the
listing into blocks at every branch target and after every `jmp`/`jcc`/`ret`; add an
edge for each `jmp`/`jcc` target, for each conditional's fall-through, and for each
block whose last instruction is neither `jmp` nor `ret`; then report every `b <- p`
with `p > b` where a forward walk from `b` never reaches `p`. Indirect (`jmp DWORD PTR
[reg*4+table]`) successors are not followed, so a function with a jump table can in
principle report a false positive — none of the six violators above is explained that
way (`LoadGruntTypeTable` and `ComputeCellFlags` both have tables and score zero).
