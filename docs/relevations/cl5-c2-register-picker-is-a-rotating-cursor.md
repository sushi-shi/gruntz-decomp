# cl 5.0's register picker is a two-pass ROTATING CURSOR, and the scalar-pair ECX/EDX coin is its request order

The modal "two same-lifetime scalars, retail has them in the other order" wall
(`CCheckpointTriggerSwitchLogic::SwitchDown` 0x00112b70, its twin `SwitchUp`
0x00112bf0, the base-class pair 0x00110570 / 0x00110670, and the tileswitchlogic
x/y rotation family) is NOT a preference-table question. Read the score with
care: a full build scores 0x00112b70 at **75.3784%**, which is not distance —
the function is 37 instructions long and the two swapped registers touch 8 of
them, so one coin costs a quarter of a small function. This entry names the machinery in `c2.exe`, gives the
one source lever that is real, and bounds it with ~200 measured cells.

## The worked example: a 25-line probe reproduces retail EXACTLY except the coin

`src/Gruntz/TileTriggerSwitchLogic.cpp`'s `CCheckpointTriggerSwitchLogic::SwitchDown`
compiles (2026-08-17, `/O2 /MT /GX /GR`) to **90 bytes / 37 instructions** — retail
0x00112b70 is `0x5a` = 90 bytes / 37 instructions, and every instruction agrees
except that the two coordinate pseudos hold each other's register:

| | retail 0x112b70 | our build |
|---|---|---|
| `m_tileX` (`[esi+0x8]`, the add term) | **EDX** | ECX |
| `m_tileY` (`[esi+0xc]`, the colOffsets index) | **ECX** | EDX |

```asm
; retail                              ; ours
mov  eax,[edi+0x30]                   mov  eax,[edi+0x30]
mov  edx,[esi+0x8]     ; tileX        mov  edx,[esi+0xc]     ; tileY
mov  ecx,[eax+0x24]                   mov  ecx,[eax+0x24]
mov  eax,[ecx+0x5c]                   mov  eax,[ecx+0x5c]
mov  ecx,[esi+0xc]     ; tileY        mov  ecx,[esi+0x8]     ; tileX
mov  ebx,[eax+0x24]                   mov  ebx,[eax+0x24]
mov  eax,[eax+0x20]                   mov  eax,[eax+0x20]
mov  ebx,[ebx+ecx*4]                  mov  ebx,[ebx+edx*4]
add  ebx,edx                          add  ebx,ecx
```

The schedule is a CONSEQUENCE, not the cause: `ecx` is needed by the `m_level`
temp between the two loads, so whichever coordinate owns ECX must be loaded
*after* that temp dies — which is why the two loads sit in those two gaps in
both builds.

The whole wall reproduces in a self-contained probe with no repo headers
(`scratch`-only, compiled directly with cl):

```cpp
struct CDDrawWorkerHost { char p0[0x20]; int* m_tileGrid; int* m_rowOffsets; };
struct CTileGrid { void ComputeCellFlags(int x, int y, int v); };
struct CLevel { char p0[0x5c]; CDDrawWorkerHost* m_mainPlane; };
struct CWorld { char p0[0x24]; CLevel* m_level; };
struct CGruntzMgr { char p0[0x30]; CWorld* m_world; char p1[0x3c]; CTileGrid* m_tileGrid; };
extern CGruntzMgr* g_gameReg;
struct CSw { char p0[8]; int m_tileX; int m_tileY; int pad; int m_linkGate;
             int SwitchDown(); };

int CSw::SwitchDown() {
    int tileY = m_tileY;
    CGruntzMgr* reg = g_gameReg;
    CDDrawWorkerHost* layer = reg->m_world->m_level->m_mainPlane;
    int tileX = m_tileX;
    int v = layer->m_tileGrid[tileX + layer->m_rowOffsets[tileY]] + 1;
    CDDrawWorkerHost* layer2 = g_gameReg->m_world->m_level->m_mainPlane;
    layer2->m_tileGrid[tileX + layer2->m_rowOffsets[tileY]] = v;
    reg->m_tileGrid->ComputeCellFlags(tileX, tileY, v);
    m_linkGate = 1;
    return 1;
}
```

90 bytes, 37 instructions, `tileX=ECX`, `tileY=EDX` — identical to the full-TU
compile. A one-file probe is therefore a faithful oracle for this wall class,
and iterates in ~2.5 s instead of a build.

## What does NOT decide the pick (~200 measured cells, all flat)

Every cell below emits the SAME 90 bytes with `tileX=ECX`:

| axis | cells | result |
|---|---|---|
| all 12 permutations of the 4 local declarations (`reg` before `layer`) | 12 | `tileX=ECX` in 12/12 (the 4 that keep `tileY` before `reg` are the 90-byte shape; the other 8 spill) |
| index operand order (`tileX + col[tileY]` vs `col[tileY] + tileX`) | 2 | byte-identical |
| tail spellings (`return 1`, `return m_linkGate`, `int ok=1`, inline setter, `this->`, `return m_linkGate=1`, `1!=0`) | 10 | byte-identical |
| inlined accessors (`int&`/`int*`/get+set `TileAt(x,y)`, `CellIndex(x,y)`) | 4 | byte-identical |
| aggregate coord (`Coord m_tile` member, local `Coord` copy) | 2 | byte-identical |
| inline helper carrying the whole body (`Bump(1)`, params `Poke(x,y,1)`) | 3 | byte-identical |
| address-tree forms (pointer arithmetic, `(grid+x)[col[y]]`, cell pointer, index/colOffsets/grid temps) | 8 | byte-identical |
| chase splitting x `v` splitting x second-walk aliasing (cartesian) | 27 | byte-identical |
| single-kind TU-state sweeps (typedef/enum/struct/class-with-inline-member, N=0..8) | 36 | byte-identical |
| **MIXED-kind TU-state sweep** (the `tu-state-probe-family` recipe, 8 kinds, 2 insertion points, N=1..12) | 60 | byte-identical |
| processor model `/G3 /G4 /G5 /G6 /GB`, `/O2 only`, `/Ox`, `/Og /Oi /Ot /Oy /Ob1`, `/Gy`, `/Ob2`, `/Gr`, `/MD` | 16 | byte-identical |
| extra live values before/after the coords (k=1..3) | 6 | `tileX=ECX` (spills, but the pair never rotates) |

So: **for this shape the pick is C2-internal.** The mixed-kind TU-state result is
the important one — this is a register wall that the state probe cannot reach,
and `walls diagnose` should not send a matcher on that sweep again.

The IL tap proves where it is decided. Capturing `/d1il` for the two coord
declaration orders of the FULL function:

```
 stream    A size    B size   ndiff  clusters
     ex      1648      1648       8         8
     sy       140       140       2         2
IL DIFFERS: 2/4 streams (ex sy)          ... yet the two objects are BYTE-IDENTICAL
```

C1 hands C2 a different tuple stream and C2 emits the same code: the request
order that produces the coin is **canonicalized inside C2**, not carried in from
the front end.

## The machinery in c2.exe — CHECK IT IN GHIDRA

Pinned toolchain `$MSVC_DIR/bin/c2.exe` (VC5.0 SP3, 660240 bytes), imagebase
**0x400000** — Ghidra's default load address, so every address below is the one
you land on. `.text` VA 0x401000 / file 0x600 / size 0x8c000;
`.data` VA 0x48e000 / file 0x8d200.

FUNCTION ENTRIES (each verified: preceded by a `ret`/`jmp` and starting on a
prologue, so Ghidra's function boundary agrees):

| entry (VA) | file offset | what it is | how you recognize it |
|---|---|---|---|
| **`0x0042b2c4`** | 0x02a8c4 | **the register picker** | prologue `push ecx/ebx/ebp/esi`, then `mov ecx,[DAT_004911a8]` + `mov edi,[DAT_004911a4]` at 0x42b2cc/0x42b2d3 |
| **`0x0042b3e2`** | 0x02a9e2 | **the allocation driver** (calls the picker at 0x42b61b and 0x42b6dd) | prologue `sub esp,0x14; mov eax,[DAT_00491080]; neg eax; sbb eax,eax; and eax,6` — that is the **EBP-in-pool** decision (register number 6 = EBP), i.e. frame-pointer omission feeding the allocator |
| **`0x0042b204`** | 0x02a804 | per-function register-descriptor init | walks DAT_00491100 until it hits value 7 (ESI), writing cost `0x1000000` into each descriptor, then writes 0 into the rest — i.e. **caller-saved get one cost, callee-saved another** |
| `0x0042ac33` | 0x02a233 | the caller of that init | `sub esp,0x1c; push ebx; push esi; mov esi,ecx; push edi` |
| `0x00435f77` | 0x035577 | the binder: writes the picked register into the value | `push ebx/ebp/esi; mov esi,edx; push edi; mov edi,ecx` |
| `0x0040181e` | 0x000e1e | the generic sparse-set membership test used as the picker's per-value eligibility query | `mov eax,[ecx]; push esi; mov esi,edx; push edi; and esi,0xffffffe0` |

DATA (all statically visible in Ghidra — no debugging needed):

| address | initial bytes | what it is |
|---|---|---|
| `DAT_00491100` | `01 00 00 00  02 00 00 00  03 00 00 00  07 00 00 00 …` | the table `{1,2,3,7,8,4,6,0}` |
| `DAT_004911a4` | `08 00 00 00` | table LENGTH = 8 (7 registers + the 0 terminator); never written, so it is a static constant |
| `DAT_004911a8` | `07 00 00 00` | the **rotating cursor**, pre-set to length-1 = 7, so the first hand-out lands on index 0 |
| `DAT_0049475d` | — | register descriptor array, stride **0x50** (`lea eax,[edx+edx*4]; shl eax,4`); flag byte at +0, cost dword at +0x2f (`0x0049478c`) |
| `DAT_00491080` | — | the flag the driver turns into "EBP joins the pool" |

The register NUMBERING is verifiable from the code, not assumed: `0x0042b1b4`
and `0x0042b609` both bound a register number to `1..8` and skip exactly **5**
(`cmp esi,5; je <skip>`). Numbering is therefore ModRM-encoding + 1 — 5 = ESP,
excluded — which makes the table `{1,2,3,7,8,4,6}` read
`EAX ECX EDX ESI EDI EBX EBP`, independently confirming the ordering the earlier
callcrossing-EBX entry asserted.

| in-function landmark | VA |
|---|---|
| cursor read (start of the search) | `0x0042b2cc` |
| candidate = `(cursor+1) mod length` | `0x0042b2e1`-`0x0042b2ee` |
| candidate register fetched from the table | `0x0042b34f` |
| pass-0 filter: descriptor flag `0x10` ("already used") | `0x0042b360` |
| two passes, then fail | `0x0042b2f5` (`cmp ebp,2`) |
| commit: `cursor = chosen index` | `0x0042b399` and `0x0042b3b5` |
| mark register used (`or cl,0x10`) | `0x0042b6c8`, `0x0042b1dd` |
| cursor reset to `length-1`, once per outer-list iteration | `0x0042b703`-`0x0042b70f` |

The picker, in full:

```asm
0042b2c4: mov  ecx,ds:0x4911a8      ; cursor
0042b2d3: mov  edi,ds:0x4911a4      ; length
0042b2db: xor  ebp,ebp              ; pass = 0
0042b2e1: lea  eax,[ecx+1]          ; candidate = cursor+1 ...
0042b2e6: cmp  eax,edi; setge dl; dec edx; and edx,eax   ; ... wrapped to 0
0042b34f: mov  edx,[esi*4+0x491100] ; the register at that index
0042b360: test byte [eax+0x49475d],0x10 ; pass 0 accepts ALREADY-USED regs only
0042b371: call 0x40181e                 ; eligibility predicate for this value
0042b399: mov  ds:0x4911a8,edi          ; commit: cursor = chosen index
0042b2f5: cmp  ebp,2                    ; two passes, then fail
```

Two facts follow directly, and they are the reusable part:

1. **The order registers are handed out is round-robin from the cursor**, not a
   fixed preference ranking. `{EAX,ECX,EDX,ESI,EDI,EBX,EBP}` is the ROTATION
   ORDER; after a reset the first hand-out is EAX, and each hand-out advances the
   cursor past the register it gave.
2. **Pass 0 will only hand out a register that is already marked used** (flag
   0x10); only pass 1 touches a fresh one. That is why cl 5.0 crowds values into
   the registers it has already spent a push on, and why adding one live value can
   reorganize a whole body instead of just consuming one more register.

The driver is register-driven, not value-driven: it picks a register and then
scans a tuple list (`[ebp+0x1c]`, walked via `[edi]`, operand kinds 2/6) for the
first eligible value to bind. So the coin is decided by **the order of that list**
relative to the cursor — a C2-internal ordering that the front end cannot address.

## The one real lever, and its exact domain

The pick becomes source-controllable exactly when the two scalars do NOT have to
survive to the end of the body. Measured A/B, same probe, only the `m_linkGate = 1;`
statement removed (so `this` dies before the call):

| variant | bytes | `tileX` | `tileY` | decl order lever |
|---|---|---|---|---|
| full body | 90 | ECX | EDX | **dead** (XY and YX are byte-identical) |
| `m_linkGate` write removed | 82 | **EDX** (XY) / ECX (YX) | **ECX** (XY) / EDX (YX) | **live** |
| second grid walk removed | 64 | EAX (XY) / ECX (YX) | ECX (XY) / EAX (YX) | **live** |

and independently, on a clean pointer-chase ladder (chase depth 0..3, coords read
from a parameter, no post-call member write) the lever is live at every depth:

| declaration order | `a` | `b` |
|---|---|---|
| `layer, a, b` | ECX | EDX |
| `layer, b, a` | **EDX** | **ECX** |
| `a, b, layer` | EAX | ECX |
| `b, a, layer` | ECX | EAX |

**Rule.** Two same-lifetime scalars take rotation-order registers in the order
C2 requests them. Declaration order controls that request order only while the
values are short enough for the request to happen inside the block that defines
them; once both are live across the whole body (a post-call member write keeps
`this` and the coords alive to the epilogue), C2 canonicalizes the order and no
source spelling — declaration order, operand order, accessor, aggregate, inline
helper, temp splitting, TU state, or processor flag — moves it.

## Refutable predictions, and what was measured

* **P1 (positive, proven):** in a body where the two scalars die before the only
  call, swapping their declarations swaps their registers. Proven on the two
  probe families above (8 cells).
* **P2 (negative, proven on 4 tree functions at their RVAs):** the tileswitchlogic
  switch bodies cannot be fixed from source. All four share the same first four
  statements and all four show the SAME rotation — retail `mov edx,[esi+0x8]` /
  `mov ecx,[esi+0xc]`, ours the reverse, everything else in the head
  byte-identical. Swapping the two coordinate declarations in the REAL TU (4
  sites, one `sed`-scale edit, compiled with the real cl) does not rotate the
  pair — it only pushes every one of them into the spill regime:

  | retail | size | current source | decls swapped |
  |---|---|---|---|
  | `0x00110570` `CTileTriggerSwitchLogic::SwitchDown` | 251 B | 250 B, X=ECX Y=EDX | 260 B, X=ECX **Y=EDI** |
  | `0x00110670` `CTileTriggerSwitchLogic::SwitchUp` | 244 B | 244 B, X=ECX Y=EDX | 254 B, X=ECX **Y=EDI** |
  | `0x00112b70` `CCheckpointTriggerSwitchLogic::SwitchDown` | 90 B | 90 B, X=ECX Y=EDX | 102 B, X=ECX **Y=EDI** |
  | `0x00112bf0` `CCheckpointTriggerSwitchLogic::SwitchUp` | 94 B | 94 B, X=ECX Y=EDX | 106 B, X=ECX **Y=EDI** |

  One mechanism, four functions; the current source already reaches retail's exact
  byte count in three of the four.
* **P3 (falsified):** "the pair is C1 handle state, reach it with the mixed-kind
  probe." 60 mixed states, zero movement. The mixed-kind family does not reach
  this class; a flat mixed sweep here is evidence about C2, not about the probe.

## Where the RE continues — ANSWERED 2026-08-18

`FUN_0042b3e2` is read in
[wall-reasons-allocation.md](wall-reasons-allocation.md). There is no single
"value list": the driver walks the flow graph's BLOCKS (resetting the cursor at
each one, `0x0042b701`), and inside a block it walks TUPLES, picking ONE
register per tuple whose opcode carries bit `0x400` in the attribute table at
`0x00494400` and binding it to the first eligible operand of the tuple's def
chain (`[T+0x1c]`) and then of its use chain (`[T+0x18]`). **The tuple order IS
the coin**, so the request order is the IL statement order — reachable from
source exactly where the source decides which computation is written first
(R3's worked example flips `CMulti::FrameSyncWait` 0x000bc070 to EXACT), and
unreachable when the competing value is a compiler-materialized constant (R4).
The original paragraph, kept for its field offsets:

The unknown was one function: **`FUN_0042b3e2`**, specifically the ordering of the
value list it scans after each hand-out (list head `[ebp+0x1c]`, walked via
`[edi]`; node kinds `[edi+8] == 2` or `6`; type field `[edi+0xa] & 0xf000` where
`0x4000` is skipped; skip flag `[edi+0x10] & 0x20`; binder call `0x00435f77` at
`0x0042b6ad`). The driver is register-driven — it picks a register and then binds
the first eligible value in that list — so **the list order IS the coin**. Name
what orders that list and the pair becomes predictable; and because the cursor is
one global advanced per hand-out, the same RE explains why adding one live value
can re-colour a whole body.

Until then this class is BOUNDED, not open: park it with the evidence above
rather than sweeping it.

## Bounds

Measured 2026-08-17 on the pinned `c2.exe` under wine, `/O2 /MT /GX /GR`,
against retail 0x00112b70 / 0x00112bf0 / 0x00110570. The probe sources are
embedded above; the sweep harness was scratch and is deleted. `build/objdiff/base`
in a stale worktree can disagree with the current source — every number here is
from a fresh compile of `src/Gruntz/TileTriggerSwitchLogic.cpp` and of the probe.

## Reproduce it yourself

Compiler side, no build needed (statically checkable in Ghidra):

```sh
# the table, the length and the cursor, straight out of .data
python3 -c "import os;d=open(os.environ['MSVC_DIR']+'/bin/c2.exe','rb').read();\
print('table ', d[0x90300:0x90320].hex());\
print('len   ', d[0x903a4:0x903a8].hex());\
print('cursor', d[0x903a8:0x903ac].hex())"
# -> table 0100000002000000030000000700000008000000040000000600000000000000
# -> len   08000000        cursor 07000000
objdump -d -Mintel --start-address=0x42b2c4 --stop-address=0x42b3e2 \
    $MSVC_DIR/bin/c2.exe     # the picker
objdump -d -Mintel --start-address=0x42b3e2 --stop-address=0x42b750 \
    $MSVC_DIR/bin/c2.exe     # the driver
```
(file 0x903a4 / 0x903a8 are `DAT_004911a4` / `DAT_004911a8`: `.data` VA 0x48e000
sits at file 0x8d200.)

Target side, one TU, no full build:

```sh
wine $MSVC_DIR/bin/cl.exe /nologo /c /O2 /MT /GX /GR \
    /I<repo>/include /I<repo>/vendor/... src/Gruntz/TileTriggerSwitchLogic.cpp
```
then read `?SwitchDown@CCheckpointTriggerSwitchLogic@@EAEHXZ` out of the object
and compare with `gruntz sema disasm 0x00112b70`. The only differing operands are
`[esi+0x8]`/`[esi+0xc]`'s registers and the two `push`es and two `add`s that use
them.
