# Wall reasons: register allocation, spilling, instruction selection, scheduling

A catalogue of the cl 5.0 decisions that produce the REGALLOC/SCHEDULING wall
class, each named at its site in `c2.exe`, each with a verdict on whether source
can reach it. Negative verdicts are the point: a PARK entry with evidence stops
a sweep that cannot succeed.

Read `walls diagnose <rva> --asm` first; then find the entry whose DETECTION
SIGNATURE matches what you see. Entries are ordered by how often they decide a
Gruntz wall, not by how interesting they are.

Toolchain: pinned `$MSVC_DIR/bin/c2.exe` (VC5.0 SP3, 660240 B), imagebase
`0x400000` = Ghidra's default. `.text` VA 0x401000 / file 0x600.
`.data` VA 0x48e000 / file 0x8d200. Measured 2026-08-18.

---

## The machine, in one page

`main.c`'s per-function pipeline (hot code at `0x004534a0`-`0x004537a0`) runs
these three passes back to back:

| call site | callee | what it is |
|---|---|---|
| `0x0045360d` | `0x0042a8aa` | register-candidate setup; reaches the per-function descriptor init `0x0042b204` via `0x0042ac33` |
| `0x0045362a` | **`0x0042b3e2`** | **the register-assignment driver** (gated on `[0x48fb4c] && [0x48fb60]`, i.e. optimizing builds only) |
| `0x0045363a` | `0x0042b750` | the **frame/stack-slot** pass — `0x0042b7aa` calls `stack.c`'s `0x0043f93b`, `0x0043fd5a`, `0x004401cd` |
| `0x00453648` | `0x0042e689` | `x86\lower.c` |

**Registers are assigned BEFORE frame slots.** A frame-layout difference is
therefore always a *consequence* of the allocator (or of a different local/temp
count), never its cause. See S2.

The driver's shape, read at `0x0042b3e2`:

```
for each flow-graph node N (list head = *(func->[8]), next = *(N+0x00)):
    DAT_004911a8 = DAT_004911a4 - 1          # 0x0042b701: CURSOR RESET, per node
    for each tuple T in N->[0x20] .. N->[0x1c]   (next = T->[0x0c]):
        if !(T->[9] & 1): continue
        <update the live/used marks from T's two operand chains T->[0x1c], T->[0x18]>
        if !(word[0x00494400 + T->[4]*2] & 0x400): continue    # opcode gate
        r = pick()                            # 0x0042b617 -> 0x0042b2c4
        if r == 0: continue                   # NO register -> the value stays in memory
        for chain in (T->[0x1c], T->[0x18]):  # def chain, then use chain
            v = first node with [v+8] in {2,6}, type != 0x4000, not skip-flagged
            if (v->[0xa] & 0xfff) == 2: skip           # word class: never enregistered
            if (v->[0xa] & 0xfff) == 1 and r > 4:      # byte class
                r2 = pick(byte_only); if !r2: skip v
            bind(v, r)                        # 0x00435f77, regasg.c
            descriptor[r].flags |= 0x10       # mark USED
```

The picker `0x0042b2c4`, in full:

| landmark | VA |
|---|---|
| cursor read | `0x0042b2cc` (`DAT_004911a8`) |
| candidate = `(cursor+1) mod len` | `0x0042b2e1`-`0x0042b2ee` (`DAT_004911a4` = 8) |
| register at that index | `0x0042b34f` (`DAT_00491100` = `{1,2,3,7,8,4,6,0}`) |
| **pass 0 accepts only ALREADY-USED registers** | `0x0042b360` (`test byte [reg*0x50+0x49475d],0x10`) |
| two passes, then a linear fallback | `0x0042b2f5` (`cmp ebp,2`), fallback `0x0042b2fa` |
| eligibility predicate | `0x0040181e` (call sites `0x0042b371`, `0x0042b318`) |
| byte-only mode (`reg <= 4`) | `0x0042b388`, requested at `0x0042b6d8` |
| commit `cursor = index` | `0x0042b399`, `0x0042b3b5` |

Register numbering is ModRM+1 (`0x0042b1b4` and `0x0042b609` both bound to 1..8
and skip exactly 5 = ESP), so `{1,2,3,7,8,4,6}` reads
**EAX ECX EDX ESI EDI EBX EBP** — a rotation order, not a preference ranking.

`0x00494400` is a per-opcode word attribute table, 283 entries (matching the
`cmp eax,0x11b` bound at `0x0042b548`); **22** of them carry bit `0x400`. Only
those opcodes request a register.

**Pass attribution.** `0x0042b3e2` has no assert of its own. It sits between
`reader.c`'s last outlined assert (hot `0x00428b37`, cold `0x0046c0ce`) and
`x86\code.c`'s first (hot `0x0042c096`, cold `0x0046c91d`) in BOTH the hot and
the cold orders, and it is machine-dependent (ESP excluded by number, the
byte-register class, the x86 opcode attribute table). Its binder `0x00435f77`
IS `regasg.c` by the assert oracle (`0x0046f327`, reached from `0x00435edc`).
Confidence: region-bracketed for the driver, assert-proven for the binder.

---

## R1 — Which register a value gets is the rotation cursor's position at the tuple that requests it

1. **REASON.** c2 hands out registers by walking `{EAX,ECX,EDX,ESI,EDI,EBX,EBP}`
   from a single persistent cursor, ONE register per tuple, in tuple order
   inside a basic block. A value's register is decided by *where its requesting
   tuple sits*, not by any per-value preference.
2. **PASS + ADDRESS.** driver `0x0042b3e2`, picker `0x0042b2c4`, cursor
   `DAT_004911a8`, table `DAT_00491100`, binder `0x00435f77` (`regasg.c`).
3. **WHAT DECIDES IT.** The IL tuple order within the block, plus the cursor
   value at that point. The cursor is **reset to `len-1` at the top of every
   flow-graph node** (`0x0042b701`), so a block's colouring does not inherit the
   previous block's hand-outs.
4. **SOURCE-REACHABLE?** PARTIAL. Source controls the tuple order only where it
   controls which computation is written first (R3). It cannot address the
   cursor.
5. **DETECTION SIGNATURE.** Base and target have the same instruction count and
   the same operand positions, and the difference is a consistent permutation of
   two-to-four register roles across the whole body.
6. **WORKED EXAMPLE.** Behavioural confirmation of the per-block reset: three
   probes whose guarded block is identical but whose *preceding* block consumes
   1, 2 or 3 registers emit that guarded block **byte-identically**
   (`mov eax,[g+44] / lea edx,[eax+eax*4] / mov eax,[g+40] / push edx / lea
   eax,[eax+eax*2] / push eax / call`). Register pressure earlier in the
   function does not rotate a later block.

```cpp
extern int g[64]; extern void s2(int,int); extern void s1(int);
void k1(int c){ int a=g[0]*7; s1(a); if(c){ int p=g[10]*3, q=g[11]*5; s2(p,q); } }
void k2(int c){ int a=g[0]*7,b=g[1]*9; s2(a,b); if(c){ int p=g[10]*3, q=g[11]*5; s2(p,q); } }
void k3(int c){ int a=g[0]*7,b=g[1]*9,d=g[2]*11; s2(a,b); s1(d);
                if(c){ int p=g[10]*3, q=g[11]*5; s2(p,q); } }
```

---

## R2 — Call-crossing values take ESI, EDI, EBX, EBP in DEFINITION order; post-call USE order is inert

1. **REASON.** N values that must survive a call are bound, in the order their
   defining statements appear, to **ESI, EDI, EBX, EBP**; the fifth and later get
   a frame slot.
2. **PASS + ADDRESS.** `0x0042b3e2` walking the block's tuples; eligibility
   `0x0040181e` rejects the caller-saved half for a value live across a call, so
   the rotation's first four *acceptable* stops are ESI(3), EDI(4), EBX(5),
   EBP(6).
3. **WHAT DECIDES IT.** Statement order of the definitions. EBP drops out of the
   pool when the function needs a frame pointer (`DAT_00491080` -> `DAT_00491118`
   at `0x0042b3e5`-`0x0042b3fe`).
4. **SOURCE-REACHABLE?** LEVER, where the definitions are reorderable: write the
   value that retail holds in ESI first.
5. **DETECTION SIGNATURE.** A cyclic permutation of ESI/EDI/EBX(/EBP) across the
   whole body with everything else identical; or (5+ values) a frame that is 4
   bytes larger on one side with one value homed.
6. **WORKED EXAMPLE.** `compute()`/`sink()` probes, `/O2 /MT /GX /GR`:

```cpp
extern int compute(int); extern void sink(int);
int d1(){ int a=compute(1); int b=compute(2); int c=compute(3); sink(0); sink(a);sink(b);sink(c); return 0; }
int d2(){ int c=compute(3); int a=compute(1); int b=compute(2); sink(0); sink(a);sink(b);sink(c); return 0; }
int d3(){ int b=compute(2); int c=compute(3); int a=compute(1); sink(0); sink(a);sink(b);sink(c); return 0; }
```

| probe | 1st defined | 2nd | 3rd |
|---|---|---|---|
| `d1` (a,b,c) | a = **ESI** | b = EDI | c = EBX |
| `d2` (c,a,b) | c = **ESI** | a = EDI | b = EBX |
| `d3` (b,c,a) | b = **ESI** | c = EDI | a = EBX |

and the width sweep, same shape with 2/4/5 values:

| N | binding, in definition order |
|---|---|
| 2 | ESI, EDI |
| 4 | ESI, EDI, EBX, EBP |
| 5 | ESI, EDI, EBX, EBP, **frame slot** |

### R2a — CORRECTION to `cl5-callcrossing-ebx-first-by-use-schedule.md`

That entry's rule ("the first call-crossing value USED after the call takes EBX;
reorder the post-call statements so that value's use leads") is **not
reproducible as a lever**, and its probe pair confounded two variables.

The original pair was `a*7 + b*11 + c*13` versus `c*7 + b*11 + a*13` — the term
ORDER *and* each value's coefficient both changed. Controlled cells:

| tail | a | b | c |
|---|---|---|---|
| `a*7 + b*11 + c*13` | **EBX** | ESI | EDI |
| `c*7 + b*11 + a*13` (the doc's probe2, order+coefficients) | ESI | EDI | **EBX** |
| `c*13 + b*11 + a*7` (order swapped, coefficients kept) | **EBX** | ESI | EDI |
| `a*13 + b*11 + c*7` (coefficients swapped, order kept) | ESI | EDI | **EBX** |

The coefficient travels with EBX; the source term order does not move it. And in
every non-product tail the assignment is definition-ordered and use-order-inert:

| tail | a,b,c |
|---|---|
| `a+b+c` / `c+b+a` / `b+c+a` | ESI, EDI, EBX (all three identical) |
| `a-b-c` / `c-b-a` | ESI, EDI, EBX |
| `a\|b<<8\|c<<16` / `c\|b<<8\|a<<16` | ESI, EDI, EBX |
| `sink2(a,b,c)` / `sink2(c,b,a)` | ESI, EDI, EBX |
| `sink(a);sink(b);sink(c);` / `sink(c);sink(b);sink(a);` | ESI, EDI, EBX |

Use R2 (definition order) instead. The tail expression *tree shape* can still
perturb the trio, but that perturbation is a scheduling consequence and is not
addressable by writing the terms in a different order.

---

## R3 — `this` versus a call result: whichever requests first wins, and the source decides which

1. **REASON.** `this` is not privileged. If the first statement defines a local
   from a call, that call result requests before `this` and takes the earlier
   rotation slot; if the first statement stores through `this`, `this` does.
2. **PASS + ADDRESS.** `0x0042b3e2`, same tuple walk as R1.
3. **WHAT DECIDES IT.** Which statement contains the first register-requesting
   tuple. Source-visible.
4. **SOURCE-REACHABLE?** **LEVER.** Make a member access the first statement
   (do not introduce a named local for it — see the negative control below).
5. **DETECTION SIGNATURE.** `mov esi,ecx` on one side and `mov edi,ecx` on the
   other, at the same offset, with the two registers swapped everywhere else and
   the instruction count equal.
6. **WORKED EXAMPLE — `CMulti::FrameSyncWait` `0x000bc070`, 98.50 -> 100.00 EXACT.**

```asm
; ours (before)                        ; retail
mov  edi,ecx        ; this  -> EDI     mov  esi,ecx        ; this  -> ESI
call ds:timeGetTime                    call ds:timeGetTime
mov  edx,[edi+0x5e4]                   mov  edx,[esi+0x5e4]
mov  esi,eax        ; now   -> ESI     mov  edi,eax        ; now   -> EDI
```

```cpp
// before: `delta` is defined from the call BEFORE anything touches `this`,
// so the call result requests first and takes ESI.
    u32 now = timeGetTime();
    u32 delta = now - m_lastFrameSyncTime;
    u32 ret = 0;
    m_accumTime = delta;
    m_lastFrameSyncTime = now;
    if (delta <= 0x1e) { ActiveWait(0x1f - delta); ... }
    else if (delta > 0x28 && m_syncGate) { ... }

// after: there is no `delta` local at all - the member IS the value. The store
// to m_accumTime is the first register-requesting tuple, so `this` takes ESI.
    u32 now = timeGetTime();
    u32 ret = 0;
    m_accumTime = now - m_lastFrameSyncTime;
    m_lastFrameSyncTime = now;
    if (m_accumTime <= 0x1e) { ActiveWait(0x1f - m_accumTime); ... }
    else if (m_accumTime > 0x28 && m_syncGate) { ... }
```

Measured ladder against the delinked target obj (40 instructions each side):

| variant | matching instructions |
|---|---|
| baseline | 30/40 |
| `u32 ret = 0;` moved first | 28/40 |
| `m_accumTime` stored before `delta` is read back | 30/40 |
| the two member stores swapped | 30/40 |
| `this->` written explicitly | 30/40 |
| `i32 delta` instead of `u32` | 28/40 |
| **member as the value (no `delta` local)** | **38/40** |
| + `u32 m_accumTime` instead of `i32` | **40/40 EXACT** |

The last two bytes were a type, not an allocation: `m_accumTime` is unsigned, so
the two range tests are `ja`/`jbe`, not `jg`/`jle`.

**Negative control, same function:** hoisting the member into a named local
(`u32 last = m_lastFrameSyncTime;` first) does NOT swap the pair — it creates a
THIRD long-lived value that takes EBX and costs a push: 98.50 -> 93.50. The
lever is "the member is the value", not "read the member early".

---

## R4 — A coin against a compiler-materialized constant is not source-reachable

1. **REASON.** When one of the two swapped values is a hoisted literal (see I1),
   it has no source definition site, so nothing in the source can move its
   request relative to the other value's.
2. **PASS + ADDRESS.** Same driver; the constant's pseudo is created by the
   earlier constant-hoisting pass, not by a source statement.
3. **WHAT DECIDES IT.** C2-internal.
4. **SOURCE-REACHABLE?** **PARK.**
5. **DETECTION SIGNATURE.** The permutation involves a register that is written
   exactly once, by `xor r,r` (or `mov r,<literal>`) near the top, and thereafter
   only read — as a store source and/or a `cmp` operand.
6. **WORKED EXAMPLE — `CTriggerMgr::DestroyAllAnims` `0x0007d330`, 98.79%.**

```asm
; ours                        ; retail
mov  ebp,0x4    ; loop count  mov  ebx,0x4
xor  ebx,ebx    ; the zero    xor  ebp,ebp
...
cmp  ecx,ebx                  cmp  ecx,ebp
mov  [ecx+0x200],ebx          mov  [ecx+0x200],ebp
```

Six controlled spellings of the loop that owns the counter (declaration order
swapped, `if (g)` vs `if (g != NULL)`, deref inlined, `for` instead of
`do`/`while`, pre-decrement) all emit **70 instructions / 211 B / 54 matching**
— identical. The loop counter is source-visible, the zero is not, and the pair
does not move.

---

## R5 — Pass 0 hands out only registers the function has ALREADY spent

1. **REASON.** The picker makes two passes. Pass 0 will only return a register
   whose descriptor already has the `0x10` ("used") bit; only pass 1 touches a
   fresh one.
2. **PASS + ADDRESS.** `0x0042b360` (`test byte [eax+0x49475d],0x10`), pass
   counter at `0x0042b2f5`, mark at `0x0042b6c8` / `0x0042b1dd`.
3. **WHAT DECIDES IT.** How many callee-saved registers the function has already
   committed to at that point — a whole-function property.
4. **SOURCE-REACHABLE?** PARTIAL: only through the total number of long-lived
   values, i.e. through R2/S1.
5. **DETECTION SIGNATURE.** One side pushes one more callee-saved register than
   the other in the prologue, and several *unrelated* values re-colour. This is
   why adding a single live value can reorganize a whole body instead of
   consuming one more register.
6. **WORKED EXAMPLE.** The `n5` probe under R2: with five call-crossing values,
   the prologue pushes ECX/EBX/EBP/ESI/EDI and the fifth value is homed;
   with four, only four registers are pushed and no slot exists.

---

## R6 — Byte-typed values: only EAX/ECX/EDX/EBX, and at most one survives a call

1. **REASON.** A value whose type field (`[v+0xa] & 0xfff`) is 1 (byte) may only
   take a register number `<= 4` — EAX, ECX, EDX, EBX, i.e. exactly the
   byte-addressable set. If the rotation's pick is higher, c2 re-picks in
   byte-only mode; if that also fails, the value is SKIPPED and stays in memory.
   Values whose type field is 2 are skipped outright.
2. **PASS + ADDRESS.** `0x0042b696`-`0x0042b6ee` (`cmp ax,1` -> `cmp esi,4` ->
   re-pick `0x0042b6dd` with `edx=1` -> `0x0042b388` `cmp edx,4`); the type-2
   skip is `0x0042b690`.
3. **WHAT DECIDES IT.** The declared width of the local/member. Across a call,
   the byte-capable set minus the caller-saved half is `{EBX}` — one register.
4. **SOURCE-REACHABLE?** **LEVER** (the declared type), inside the constraint
   that the type must be what retail's code actually uses.
5. **DETECTION SIGNATURE.** `mov BYTE PTR [esp+N],al` right after a call, with a
   frame 4 bytes larger per homed byte value; or `mov bl,al` for the one that fits.
6. **WORKED EXAMPLE.**

```cpp
extern int compute(int); extern void sink(int); extern void bsink(char);
int c3() { char a=(char)compute(1); char b=(char)compute(2); char c=(char)compute(3);
           sink(0); bsink(a); bsink(b); bsink(c); return 0; }
int b1() { char a=(char)compute(1); sink(0); return a; }
int b2() { unsigned char a=(unsigned char)compute(1); sink(0); return a; }
int b5() { int  a=(char)compute(1); sink(0); return a; }
```

| probe | result |
|---|---|
| `c3` — three `char` across a call | ALL THREE homed, `sub esp,0xc`, no callee-saved register pushed |
| `b1` — one `char` across a call | `mov bl,al` — **EBX** |
| `b2` — one `unsigned char` | **homed** (`mov [esp],al` … `mov eax,[esp]; and eax,255`) — cl 5.0 declines `movzx` here |
| `b5` — the same byte widened to `int` in the local | `movsx esi,al` — ESI, no slot |

So a `char`/`BYTE` local live across a call costs a frame slot from the second
one onward, and `unsigned char` costs one from the first.

---

## R7 — EBP is in the pool unless the function needs a frame pointer

1. **REASON.** The driver computes the register number 6 (EBP) or 0 from a global
   flag and stores it as the "extra allocatable register".
2. **PASS + ADDRESS.** `0x0042b3e5`-`0x0042b3fe`:
   `mov eax,[0x491080]; neg eax; sbb eax,eax; and eax,6; mov [0x491118],eax`.
3. **WHAT DECIDES IT.** Whether the function is frameless (`/Oy` is implied by
   `/O2`; `_alloca`, some EH shapes and `__declspec(naked)` force a frame).
4. **SOURCE-REACHABLE?** PARTIAL — only by not doing the things that force a
   frame pointer.
5. **DETECTION SIGNATURE.** One side uses EBP as a general register (`push ebp` in
   the prologue with no `mov ebp,esp`), the other has `mov ebp,esp`.
6. **WORKED EXAMPLE.** The `n4` probe (R2) is frameless and binds the fourth
   call-crossing value to EBP rather than a frame slot — the cl-5.0 refinement
   over the VC6 model already recorded in
   `cl5-callcrossing-ebx-first-by-use-schedule.md`.

### Open RE case — `CDDSurface::Blit824` parameter rotation

`Blit824` at `0x00140110` is a bounded example where the known rules do not yet
explain which rotation stop a parameter receives. Retail loads the palette into
EBX before saving EBP; the current compile loads the same parameter into EBP.
Both are frameless, save all four callee-saved registers, and have identical
calls (2), branches (22), returns (3), relocations (1), frame size (`0x2c`), and
semantic homes for all six byte/widened channel values. The base is 16 bytes and
six instructions longer solely through the downstream register schedule.

Personally measured negative controls: a typed palette alias defined before or
after `Lock`, a `void*` parameter plus typed local, implicit versus explicit byte
narrowing at the destination store, and four semantic palette identifier names
all compile byte-identically at 69.83012%. A bounded 243/384 syntax campaign
found 12 islands; its best 71.25869% state only reordered one commutative sum in
the second duplicated arm, contrary to retail's unchanged square/add order, so
it is not retained. The existing file-scope TU-state sweep is flat for this
function. Further work belongs in the allocator tuple-walk/rotation-cursor RE,
not in additional source-order guesses. `Blit816` is a negative sibling: both
sides bind its palette to ESI, so its separate residue is spill scheduling.

### Open RE case — `CGrunt::ClaimSwitchTile` aggregate coalescing

`ClaimSwitchTile` at `0x00052c70` has an exact source-level switch oracle: the
retail jump table enters the middle of the northwest and southwest arms for the
north and south cases, proving the two fall-throughs. After those are restored,
retail carries the input and output `Coord` fields in EBX/EDI and sends all six
arms directly to the join. The current compile carries the input in EAX/ECX,
homes the output `Coord`, and routes the northeast arm through the northwest
suffix. Calls (2), returns (2), and relocation identities (14) agree; the
12-versus-13 branch difference is downstream cross-jumping, not missing logic.

Measured controls bound the source-visible levers. Merging or moving the scalar
declarations is byte-flat; modeling the destination as the complete `Coord`
changes 69.24286% to 69.23572% while correcting the source model; separate
lexical scopes are flat. A post-call snapshot of the old coordinate falls to
67.30%. Splitting the owner construction around the occupied-flag store falls to
68.14%, and C2 still hoists both member loads ahead of that store. The unresolved
state is aggregate coalescing and the resulting cross-jump choice, not another
switch spelling.

### Open RE case — `CDDSurface::FlipVertical` locked-buffer home

`FlipVertical` at `0x0013ebb0` is semantically aligned at five calls, two
returns, and three ordered relocations. Retail keeps `this` in EBX, initially
holds the locked buffer in EDI and homes it, leaves width in ECX, and reloads the
buffer at the outer-loop latch. That reload creates the sole extra branch: the
first iteration jumps over it because EDI still holds `Lock`'s result. The
current compile keeps the buffer in EBP, assigns width to EDI, homes `this`, and
strength-reduces `height - i - 1` into a decrementing height slot. Its latch
therefore restores five values without the entry jump.

The source-visible controls are bounded. `const` height, a lexical loop scope,
a natural outer `for`, and assignment-inside-null-guard forms for both allocated
pointers are byte-flat. Rewriting all three proven pointer-copy loops as one
reused array index falls from 70.552635% to 44.47%. Earlier local/member
height-width, bottom-row, and hoisted-pitch controls were inert or worse. A
classified campaign compiled 260 variants (four source shapes crossed with 65
TU states) and found one compiler island at baseline; no state or mutation
changed one target byte. The remaining question is which C1 tuple ordering makes
C2's per-block rotating picker home the locked buffer rather than `this`; repeat
source permutation only if that tuple distinction becomes understood.

### Open RE case — `CRezImage::FlipVertical` row-swap homes

`CRezImage::FlipVertical` at `0x00176840` is complete at two calls, ten
branches, one return, and two ordered relocations. Retail emits 100
instructions in `0x11f` bytes; the current member-direct source emits 91 in
`0x109`. Retail assigns `this` to EBX and the scratch row to EBP, homes the
scratch pointer, and carries separate height, top-offset, decreasing-bottom,
outer-index, and pair-count states in a `0x18` frame. The current compile
assigns `this` to EBP and the scratch row to EBX and coalesces the same loop
state into a `0x10` frame. This is the register/frame consequence of a
different tuple ordering, not a missing copy arm.

The lower current score is an evidence-backed source correction. Retail loads
`m_pixels` from `[ebx+0x42c]` inside each of the three byte-copy loops. Direct
member indexing reproduces those alias-barrier reloads. The historical 71.0707%
shape caches explicit `top` and `bot` pointers, loads `m_pixels` before the
loops, and is only 246 bytes; it is a useful compiler frontier but contradicts
the retail load placement. Its MAX remains banked while the faithful
member-direct source currently scores 52.1616%.

The personal re-audit exhausted the source-visible lifetime axes. A 198-cell
campaign (six generated source shapes crossed with 33 TU states) produced one
island at the current bytes. A 100-cell Cartesian matrix of scratch assignment,
dimension-declaration order, row pointers, explicit top offsets, scoped byte
indices, and staged bottom arithmetic produced six islands; the best faithful
member-direct state remained baseline, while captured-height pointer rows
reached only 56.7778%. Six complete height/row models reproduced the historical
71.0707% pointer frontier and put member-height/member-direct second at
61.8586%. Moving dimension captures across allocation produced six worse
states (best 53.1818%), and all eight `const` combinations for height, width,
and pair count were byte-identical. Finally, crossing the historical pointer
shape with 65 TU states yielded one island at 71.0707%.

Reopen this wall only with evidence for a real inlined row primitive or a C1
tuple distinction that makes the per-block rotating picker home scratch and
height without caching `m_pixels`. Parser-state noise, declaration order, and
more row-pointer permutations are bounded.

### Open RE case — `WarpTextureBlit` edge-cursor allocation

`WarpTextureBlit` at `0x00146a20` is structurally aligned after replacing its
two-break power-of-two scan with the bounded `while` form documented in
`docs/patterns/loop-rotates-on-the-CONDITION-not-the-first-break.md`. That
source correction raises 71.78242% to 74.44835% and gives both sides 16 calls,
31 branches, and two returns. Retail emits 456 instructions in `0x5b7` bytes
with a `0x28` frame and 84 relocations; the current compile emits 467
instructions in `0x5d6` bytes with a `0x20` frame and 85 relocations.

The remaining difference begins at the prologue and propagates through one
allocation decision. Retail keeps `src` in ESI, immediately homes `minY` and
`maxY`, carries the common `minY * sizeof(ClipVtx)` byte offset across both
surface locks, and materializes independent left/right edge bases afterward.
The current compile keeps `src` in EBX, initially carries `minY` in ESI, and
materializes the left-table base before the locks. In each of the three mode
arms it then derives the right cursor through the left base. That accounts for
the one extra `_g_rasterEdgeL` relocation overall; the relocation identity is
not a missing or misnamed global.

The personal search bounded the source-visible axes. A classified campaign
compiled 198 candidates (six syntax-aware source shapes crossed with 33 TU
states) and found two islands: baseline and a worse 73.80% state caused by
swapping the independent `top`/`bot` assignments. A 32-cell bounds-initializer,
pitch/lock-order, and cursor-order grid found no state above baseline. Moving
the pitch load before the second lock reached 73.84615% and changed the bounds'
stack order toward retail, but rotated the rest of the function away from it.
Five natural cursor constructions were byte-identical in each lock-order state.
Six explicit one/two-stage mask spellings formed one island. A one-variable
combined shift reached 74.60879% but emits one `shl` where retail has a constant
shift followed by a variable shift, so it is rejected as a score-only topology
change.

Reopen this wall only with a mapped C1 tuple distinction or allocator evidence
that selects retail's `src=ESI` state without changing the proven call, branch,
return, mask-shift, or cursor semantics.

---

## S1 — When a value gets no register at all

1. **REASON.** Three distinct causes, all visible in the driver:
   (a) the picker returned 0 — nothing eligible (`0x0042b624`);
   (b) the value is byte class and no byte register was free (R6);
   (c) the value is word class (`type == 2`) and is never a candidate.
2. **PASS + ADDRESS.** `0x0042b617`-`0x0042b6ee`.
3. **WHAT DECIDES IT.** The number of simultaneously live values of each class.
4. **SOURCE-REACHABLE?** PARTIAL: the live-value count and the declared widths.
5. **DETECTION SIGNATURE.** `(target frame size - base frame size) / 4` equals the
   number of extra dword homes on the retail side; a homed value shows as
   `mov [esp+N],reg` right after its definition and `mov reg,[esp+N]` at each use.
6. **WORKED EXAMPLE.** The `n5` probe: `mov DWORD PTR _d$[esp+20],eax` for the
   fifth call-crossing value while the first four are in registers.

---

## S2 — Frame layout: aggregates take the top; a frame-size delta is a COUNT question

1. **REASON.** The frame pass places aggregate locals at the highest local
   addresses regardless of where they are declared, then the scalars below.
   Because it runs AFTER register assignment, the set of homed scalars is already
   fixed when it runs.
2. **PASS + ADDRESS.** `0x0042b750` -> `0x0042b7aa` -> `stack.c` `0x0043f93b`
   (walks a stride-0x14 table from `[0x4910c0]-1` DOWNWARD), `0x0043fd5a`,
   `0x004401cd`. `stack.c` is assert-proven at hot `0x0043f99b` (cold
   `0x00470315`, string `0x0049e12e`).
3. **WHAT DECIDES IT.** The number and kind of locals/temps needing a home. Slot
   ORDER within the scalars follows declaration order but is perturbed by the
   emitted read order of the last group.
4. **SOURCE-REACHABLE?** PARTIAL. Do not chase the ORDER; chase the COUNT. If the
   frames differ in size, one side has a local, a temporary object, or a homed
   value the other does not.
5. **DETECTION SIGNATURE.** `sub esp,N` differs, every parameter reference shifts
   by the same delta, and at least one local offset moves by something OTHER than
   that delta.
6. **WORKED EXAMPLE — `FontRenderer::MeasureWrapped` `0x0017ad10`, 95.78%.**
   Base and target have **308 instructions each and not one opcode differs**;
   the entire wall is `sub esp,0x50` vs `sub esp,0x54` plus a slot reshuffle:

   | slot (frame-relative) | base | target |
   |---|---|---|
   | first saved param | +0x00 | +0x00 |
   | ctor'd object | +0x04 | +0x04 |
   | zeroed dword | +0x0c | **+0x1c** |
   | `mov [..],esp` marker | +0x14 | **+0x10** |
   | second object | +0x3c | **+0x40** |

   `(0x54-0x50)/4 = 1`: retail homes exactly one more dword. Look for a missing
   local or temporary, not for a scheduling difference.

   Frame-order probe backing the "aggregates first" rule:

```cpp
extern void take(int*); extern void taker(struct R*); extern void use(int);
struct R { int l,t,r,b; };
void p1() { int a,b,c; R r1; int d;  take(&a);take(&b);taker(&r1);take(&c);take(&d); use(a+b+c+d+r1.l); }
void p2() { R r1; int a,b,c,d;       take(&a);take(&b);taker(&r1);take(&c);take(&d); use(a+b+c+d+r1.l); }
void p3() { int a,b,c,d; R r1;       take(&d);take(&c);taker(&r1);take(&b);take(&a); use(a+b+c+d+r1.l); }
```

   `r1` lands at `-16` (the top of a 32-byte frame) in ALL THREE, whether it is
   declared 4th, 1st or last; the scalars occupy `-20 -24 -28 -32`.

### Worked example — `CLightFxRender::ComputeRect` `0x000a3820`

The inherited `@early-stop` called this a flat 156-instruction scheduling
braid. A personal source-hash audit disproved that bound. Two real source
entities move the wall:

* retaining `RECT* dstRect = &m_dstRect` across `BltEx` raises the result from
  68.87% to 70.12%, gives retail's 0x14-byte frame and 156-instruction count,
  and explains why retail materializes `&m_dstRect` before the call and reuses
  that address afterward;
* expressing the surface through `m_surface` instead of an asserted local cache
  reaches 76.38% and restores `this=ESI`. It also exposes the remaining count
  question: the base is 400 bytes while retail is 398, and the base reloads
  `m_surface` where retail retains one early load.

The second result is structural evidence, not exact closure. The bounded
negative controls were:

* 198 syntax/TU-state cells and a separate 65-state sweep; compiler state was
  flat within each source shape;
* six destination-store/call/post-call pointer shapes, before and after the
  surface correction;
* four edge/extent declaration pipelines crossed with `RECT` versus `CRect`;
* four POD `MakeRect` builder shapes (best alternative 67.76%);
* assignment, `memcpy`, fieldwise copy and source/destination alias spellings
  for the `m_srcRect` copy; and
* four post-call left/top offset-source forms.

These controls produced discrete structural islands rather than a continuous
schedule frontier. Reopen at the C1 tuple/lifetime level: retail keeps the
surface in caller-saved `ECX`, `this` in `ESI`, the destination address in
`EBX`, and one additional scalar home. Do not re-run TU-state permutations or
reorder the four extent declarations; those cells are proven flat.

---

## I1 — The hoisted zero register: `test r,r` versus `cmp r,<zreg>`

1. **REASON.** Once a function materializes the literal 0 into memory enough
   times, c2 dedicates a register to it — and then EVERY `== 0` test in the
   function becomes `cmp r,<zreg>` instead of `test r,r`. Below the threshold the
   zero is re-materialized per store and the tests stay `test r,r`.
2. **PASS + ADDRESS.** The constant is a normal value by the time
   `0x0042b3e2` sees it (which is why it competes for ESI/EDI/EBX/EBP — see R4);
   the hoist itself is a `globopt`/`globlopt` decision (asserts at hot
   `0x0043c1d4`, `0x0043d62a`, `0x0043e15e`, `0x0043e3b8`).
3. **WHAT DECIDES IT.** The COUNT of source sites that need 0 as a *value*.
   Measured thresholds (`/O2 /MT /GX /GR`, one basic block, `g[i] = 0;` stores
   and `if (g[j] == 0)` tests):

   | zero stores | zero compares | result |
   |---|---|---|
   | 0-1 | any | no shared zero; `test r,r` |
   | 2-3 | 0-1 | shared zero in **EAX** (caller-saved), still `test r,r` |
   | 2-3 | >= 2 | no hoist; `test r,r` |
   | **>= 4** | **>= 2** | **dedicated callee-saved zero** (`xor esi,esi`), all tests become `cmp r,esi` |

   `push 0` call arguments do NOT count (measured 0..6 argument zeroes: never
   hoists) — the zero must be *stored*.
4. **SOURCE-REACHABLE?** **LEVER** — through the number of `= 0` / `= NULL`
   stores the reconstruction contains. It is a correctness question, not a
   spelling trick: if retail hoists and we do not, our body is missing zero
   stores (or folded some into a `memset`/ctor); if we hoist and retail does not,
   we have written stores retail did not.
5. **DETECTION SIGNATURE.** `test r,r` on one side against `cmp r,<callee-saved>`
   on the other, on the SAME operand register, plus a stray `xor r,r` near the
   top of whichever side hoisted.
6. **WORKED EXAMPLE.** Corpus census over the 755 below-100 functions whose
   base/target instruction streams align: **67** sites where base has `cmp` and
   target has `test`, **34** the other way. Real rows:

   | function | base | target |
   |---|---|---|
   | `CGrunt::WanderStep` `0x000ed9f0` 84.34 | `cmp eax,ebp` | `test eax,eax` |
   | `CTriggerMgr::PlaceObjectFull` 72.22 | `cmp ecx,ebp` | `test ecx,ecx` |
   | `CMapMgr::ComputeCellFlags` 97.66 | `test ebx,ebx` | `cmp ebx,ecx` |
   | `CGrunt::ScanNearestTarget` 69.31 | `test eax,eax` | `cmp eax,ebp` |

   `WanderStep` `0x000ed9f0` is the clean instance: WE hoist a zero into EBP and retail does
   not, six times in one function. Count the zero stores before touching anything
   else there.

---

## I2 — `sar` versus `shr` is the operand's static type; cl 5.0 NEVER narrows

1. **REASON.** The shift kind follows the C type of the shifted expression, full
   stop. There is no mask-driven or range-driven narrowing peephole.
2. **PASS + ADDRESS.** Selected during lowering, `x86\lower.c` (assert-proven at
   hot `0x0042e2c4`) / `x86\code.c` (`0x0042c096`-`0x0042c9d2`); the selector
   itself was not isolated — this entry is behavioural.
3. **WHAT DECIDES IT.** Signedness of the shifted expression.
4. **SOURCE-REACHABLE?** **LEVER in general, PARK for the one Gruntz instance.**
5. **DETECTION SIGNATURE.** A single opcode byte differs (`C1 /7` vs `C1 /5`) with
   identical operands and identical surrounding instructions.
6. **WORKED EXAMPLE — `CStatusBarMgr::StartChipMachineCycle` `0x00107d00`, 98.48%.**
   The ENTIRE residue below the tail is four `sar reg,0x10` where retail has
   `shr reg,0x10`, all four inside expansions of `GetRandomNumber()`
   (`include/Gruntz/GameRand.h`).

   Why it is PARK, not "make `holdrand` unsigned":

   * Retail image census of every reference to the seed `0x2c1288`: **19 `sar`
     expansions and 4 `shr` expansions**. All four `shr` are inside this one
     function; every other expansion in the image is `sar`. Changing the type
     would break the 19.
   * Inside this one function the split is by ARM, from the same seed:

   ```asm
   ; the `range == 0` arm            ; the `% range + 1` arm
   mov  edx,ecx                      mov  eax,ecx
   mov  ds:0x6c1288,ecx              mov  ds:0x6c1288,ecx
   shr  edx,0x10                     sar  eax,0x10
   and  edx,0x1                      and  eax,0x7fff
                                     cdq ; idiv ebp ; inc edx
   ```

   * Negative controls, all `sar`, none reproduced retail's `shr`: a 28-cell
     mask x shift sweep (`>>1/8/16/24` x `&1/3/0x7f/0xff/0x3fff/0x7fff/0xffff`);
     ten spellings of the zero arm (`& 1`, `% 2`, `(unsigned)x % 2`,
     `(int)((unsigned)x & 1u)`, `/2 & 1`, `!= 0`, ternary, `& 3`, `>>1 & 1`);
     `/G3 /G4 /G5 /G6 /GB`, `/O1 /O2 /Os /Ot /Ox`; and a value cl can prove
     non-negative (`(x & 0x7fffffff) >> 16 & 1`) — still `sar`.

   The probe that reproduces the whole shape, one file, no repo headers:

```cpp
extern "C" unsigned long timeGetTime();
__inline int GetRandomNumber() {
    static long holdrand = timeGetTime();
    return (((holdrand = holdrand * 214013L + 2531011L) >> 16) & 0x7fff);
}
static __inline int WapRand(int range) {
    if (range == 0) return GetRandomNumber() & 1;
    return GetRandomNumber() % range + 1;
}
extern int tab[64];
int probe() { return WapRand(tab[2]); }
```

   Next lane: the selector lives in `x86\lower.c`/`x86\code.c`; find where the
   right-shift opcode is chosen and what per-expression state distinguishes the
   two arms. Do NOT sweep source spellings again — 40+ cells are flat.

---

## I3 — Byte-width memory forms name the field's declared width

1. **REASON.** A read-modify-write of a BYTE-typed lvalue is a single
   memory-form instruction; the same operation on a dword field is a
   load/modify/store through EAX whose middle instruction still uses the *byte*
   register when the constant fits in 8 bits.
2. **PASS + ADDRESS.** `x86\lower.c` / `x86\code.c` addressing selection.
3. **WHAT DECIDES IT.** The declared width of the member.
4. **SOURCE-REACHABLE?** **LEVER** (the member's type in the class header).
5. **DETECTION SIGNATURE.**

```asm
or   BYTE PTR [X],4          ; <- X is a BYTE/char/bool field
mov  eax,[X] ; or al,4 ; mov [X],eax   ; <- X is a 32-bit field |= a small const
```

   Note the second form: do NOT read `or al,4` as evidence of a byte field.
6. **WORKED EXAMPLE.** 384 `or`/`and BYTE PTR [reg+disp],imm8` sites exist in
   retail `.text`, so the form is common and reliable. The corpus census finds
   26 wall sites whose only local difference is a BYTE-vs-DWORD `mov`, e.g.
   `CSpriteRef::Build` 83.19 (`mov BYTE PTR [esp+0x14],0xc0` vs `mov bl,0xc0`:
   the value is byte-typed on both sides but only retail gets the register — R6),
   and `CInGameIcon::CInGameIcon` 98.26.

```cpp
struct S { int a; char b; unsigned char c; short d; int e; };  extern S s;
void t1() { s.b |= 4; }          // 80 0d .. 04        or BYTE PTR [s+4],4
void t2() { s.a |= 4; }          // a1 .. / 0c 04 / a3 ..
void t3() { s.c &= 0xf7; }       // 80 25 .. f7        and BYTE PTR [s+5],0xf7
```

---

## I4 — `lea` versus `add`, and how addresses are materialized

1. **REASON.** cl uses `add` when the operand's register is dead and can be
   overwritten, and `lea` when the result must land in a *different* register
   from its operand. An interior address of a static is an absolute immediate
   with the offset folded into the relocation addend, never a `lea`.
2. **PASS + ADDRESS.** `x86\addr.c` (asserts at hot `0x00411225`, `0x0041145e`)
   and `x86\code.c`.
3. **WHAT DECIDES IT.** Whether the operand is still live — an allocation
   consequence, not a spelling.
4. **SOURCE-REACHABLE?** PARTIAL: only via the value's lifetime.
5. **DETECTION SIGNATURE.** `add reg,K` on one side and `lea reg2,[reg+K]` on the
   other, with a register role difference nearby. Beware: in a difflib-style
   alignment, most `mov`/`lea` "replacements" are schedule shifts, not selection
   differences — check the surrounding context before believing one.
6. **WORKED EXAMPLE.**

```cpp
int  t9 (int* p)  { return (int)(p + 3); }              // add eax,12
int  t12(int  x)  { return x + 3; }                      // add eax,3
int  t13(int  x)  { int y = x + 3; sink(y); return y; }  // lea esi,[eax+3]
int* t14()        { return &arr[4]; }                    // mov eax,OFFSET arr+16
void t15()        { sink((int)&s.e); }                   // push OFFSET s+8
```

---

## I5 — Zero tests are always `test r,r`, at the operand's own width

1. **REASON.** `x ? 1 : 0`, `x != 0` and `if (x)` all lower identically; the
   operand width follows the value's type.
2. **PASS + ADDRESS.** `x86\lower.c`.
3. **WHAT DECIDES IT.** The value's type only.
4. **SOURCE-REACHABLE?** LEVER for the WIDTH (`test cl,cl` vs `test ecx,ecx`
   names a byte-typed value); PARK for the form — no spelling produces
   `cmp r,0`. When retail shows `cmp r,<reg>` instead, that is I1, not this.
5. **DETECTION SIGNATURE.** `84 /r` (byte) vs `85 /r` (dword) on the same value.
6. **WORKED EXAMPLE.**

```cpp
int t5(int x)  { return x ? 1 : 0; }   // xor eax,eax; test ecx,ecx; setne al
int t6(int x)  { return x != 0; }      // byte-identical to t5
int t7(int* p) { return p ? 1 : 0; }   // byte-identical to t5
int t8(char x) { return x ? 1 : 0; }   // ... test cl,cl ...
```

---

## C1 — Store order, and the true domain of the alias-opacity lever

1. **REASON.** The Pentium pairing scheduler (`x86\schedmd.c`) may sink a single
   pending store past a following load/compare to fill a pairing slot. A store
   through a pointer whose aliasing c2 cannot disprove is not sinkable, so
   materializing the address and storing through it pins source order.
2. **PASS + ADDRESS.** `x86\schedmd.c`, assert-proven, 18 outlined sites: hot
   `0x0045011f` and the dense band `0x00463372`-`0x004639fd`. It runs AFTER
   register assignment and after `x86\code.c`, so it can only move already-bound
   instructions — it never changes which register a value holds.
3. **WHAT DECIDES IT.** Alias information at the store, plus pairing pressure.
4. **SOURCE-REACHABLE?** PARTIAL, and narrower than it looks. Measured: in small
   bodies cl 5.0 does NOT sink stores at all — seven controlled probes (store to
   a global struct then read another global; two stores then a read; store
   through a possibly-aliasing pointer then read a global; store then read a
   sibling member; store across a call; a bound-pointer pair run; the same run as
   member statements) all preserve source store order, and the bound-pointer and
   member-statement forms are **byte-identical**. The lever documented in
   `docs/patterns/pair-store-run-off-a-rederived-element-pointer.md` needs the
   pressure of a real body; it is not a general "pointer pins order" rule.
5. **DETECTION SIGNATURE.** Same instructions, same registers, one store moved
   past one or two instructions. If the registers also differ, it is R1-R4 and
   the schedule is the consequence, not the cause.
6. **WORKED EXAMPLE.** The established one stands:
   `CStatusBarMgr::UpdateDestructButtonStatusBar` `0x0010b320` 94.67 -> 100.00
   EXACT by storing through an `SbiClockPair*`, with `SetHudRectA/B`
   `0x001066f0`/`0x00106740` 71.83 -> 100.00 on the same change. The negative
   control is in that pattern file: converting `Sync`'s nested walk to a flowing
   cursor scored 90.43 -> 83.19.

---

## An immediately actionable worklist

Scanning all 1024 functions between 60% and 100% for bodies whose ENTIRE
divergence is a consistent permutation of `{esi,edi,ebx,ebp}` (same instruction
count, every differing instruction equal after the substitution) finds exactly
five. Each is an R2/R3/R4 decision and nothing else:

| function | rva | score | permutation |
|---|---|---|---|
| `CMulti::FrameSyncWait` | `0x000bc070` | ~~98.50~~ **100.00** | esi<->edi — **closed**, see R3 |
| `CGrunt::StepWarpExit` | `0x00064540` | 98.77 | esi<->edi (18/79 insns) |
| `CTriggerMgr::DestroyAllAnims` | `0x0007d330` | 98.79 | ebx<->ebp (16/70) — R4, PARK |
| `CTimer::HandleEvent` | `0x0009c1c0` | 99.43 | ebx<->ebp (12/106) |
| `CMulti::DispatchRecvMsg` | `0x000b9750` | 99.97 | esi<->edi (3/526) |

Reproduce with the scanner recipe in "Bounds" below; it is 20 lines over the
`compare-new` normalized objs.

---

## Bounds

Every number above is from the pinned cl 5.0 SP3 under wine, `/O2 /MT /GX /GR`,
measured 2026-08-18 in the `matcher-3` worktree at `0c8b20da6`. Probe TUs were
scratch, never in the build graph, and are embedded above rather than kept.
Corpus figures come from `build/objdiff/compare-new/{base,target}` after a full
green `gruntz build`.

Static readings that are NOT behaviourally confirmed and are labelled as such:
the per-block cursor reset (the reset instruction is unambiguous at
`0x0042b701`; what the outer list contains is inferred from the node shape), and
the pass attribution of `0x0042b3e2` itself (region-bracketed, not assert-proven).

Reproduce the compiler side without a build:

```sh
# the rotation table, its length, the cursor
python3 -c "import os;d=open(os.environ['MSVC_DIR']+'/bin/c2.exe','rb').read();\
print('table ',d[0x90300:0x90320].hex());print('len   ',d[0x903a4:0x903a8].hex());\
print('cursor',d[0x903a8:0x903ac].hex())"
objdump -d -Mintel --start-address=0x42b2c4 --stop-address=0x42b3e2 $MSVC_DIR/bin/c2.exe
objdump -d -Mintel --start-address=0x42b3e2 --stop-address=0x42b750 $MSVC_DIR/bin/c2.exe
objdump -d -Mintel --start-address=0x4534a0 --stop-address=0x4537c0 $MSVC_DIR/bin/c2.exe  # the pipeline
```

Pass attribution, generalized (this supersedes hand-searching for `push <str>`:
cl 5.0's asserts load the file name with `mov ecx,<VA>`, not `push`): find every
`E:\utc\src\\...` string, find `\xb9` + its VA in `.text`, then for each hot->cold
jump walk the cold block until it reaches one of those loads. That yields 273
assert sites and a hot-address -> pass map for the whole image.
