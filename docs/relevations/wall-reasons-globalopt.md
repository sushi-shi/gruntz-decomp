# cl 5.0's global optimizer: the alias model is ONE byte, and every reload, pinned store order and loop shape follows from it

The `/Og` global optimizer (`globopt.c` / `globlopt.c` / `globdf.c` in `c2.exe`) is
where most of the surviving walls are decided, and the decisions are not moods.
This entry states the memory model, states the induction-variable rule, and gives
one catalogue entry per decision point in the six fields a matcher needs.

Two results up front, because they subsume a pile of one-off tricks:

* **The alias model is: an access is a `(BASE, constant byte offset, width)` triple.
  BASE is either a NAMED SYMBOL or ONE pointer pseudo-register. Two accesses are
  proven disjoint only when the bases are the same kind AND the offset ranges do
  not overlap. Everything else may alias — a pointer store vs any global, two
  different pointer pseudos, and any variable index.** That is why a member read
  after a call is always re-emitted, why a store pins the statement order of the
  next load, and why a loop containing any store never hoists an invariant load.
* **Induction variables coalesce on equal constant BYTE STRIDE, not on pointer
  type**, and only for cursors whose base is loop-INVARIANT. Incrementing a
  *parameter* destroys the invariant base and blocks the whole reduction. Which
  pointer survives as the IV is settled by **declaration order**.

Everything below was measured 2026-08-18 on the pinned cl 5.0 SP3 under wine,
`/O2 /MT /GX /GR`, with scratch probe TUs (never in the build graph, embedded
here, deleted). Static-only readings of `c2.exe` are labelled UNCONFIRMED.

---

## 0. The pass-name oracle, improved: the assert block names its own RETURN address

`cl5-crossjump-merges-suffixes-not-blocks.md` located passes by finding a `push
<string VA>` and then the hot `jcc` into the cold block. That is not the shape cl
emits, and it under-finds. The real shape is:

```asm
0046709e: ba cd 1c 00 00   mov  edx,0x1ccd            ; LINE NUMBER
004670a3: b9 60 e0 49 00   mov  ecx,0x49e060          ; "E:\utc\src\\P2\globopt.c"
004670a8: e8 8f fa 01 00   call 0x486b3c              ; the assert handler
004670ad: e9 db 4e fa ff   jmp  0x40bf8d              ; <-- THE HOT CONTINUATION
```

So scanning for `BA imm32 / B9 imm32 / E8 -> 0x486b3c` finds **241 assert cold
blocks** (not the handful the `push` scan finds), each carrying its file name,
its **source line number**, and — via the trailing `jmp` — the exact hot address
the pass runs at. Reproduce:

```sh
python3 - <<'EOF'
import os,struct
d=open(os.environ["MSVC_DIR"]+"/bin/c2.exe","rb").read()
T,F,S=0x401000,0x600,0x8c000; t=d[F:F+S]; DV,DF=0x48e000,0x8d200
for i in range(len(t)-20):
    if t[i]==0xBA and t[i+5]==0xB9 and t[i+10]==0xE8:
        fs=struct.unpack_from("<I",t,i+6)[0]
        if not 0x48e000<=fs<0x4a0000: continue
        if T+i+15+struct.unpack_from("<i",t,i+11)[0]!=0x486b3c: continue
        ret=T+i+20+struct.unpack_from("<i",t,i+16)[0] if t[i+15]==0xE9 else None
        o=fs-DV+DF; nm=d[o:d.index(b"\0",o)].decode().split("\\")[-1]
        print("%-12s line %5d  cold 0x%08x  hot %s"%(nm,struct.unpack_from("<I",t,i+1)[0],T+i,hex(ret) if ret else "-"))
EOF
```

Anchors for this domain (owner = the Ghidra function containing the hot address):

| pass | assert lines | hot continuation | owning function |
|---|---|---|---|
| `globopt.c` | 7373 | `0x0040bf8d` | `FUN_0040be34` (914 B) |
| `globdf.c` (global data flow) | 2944, 3901 | `0x00404261`, `0x00418b91` | `FUN_00403d90` (2218 B), `FUN_00418a34` |
| `globlopt.c` | 3025, 3066, 3693, 3948, 7990 | `0x0043c1a3`, `0x0043d883`, `0x0043e237`, `0x0044d5e3` | `FUN_0043c1a3`, `FUN_0043d4a4`, `FUN_0043de8b`, `FUN_0043e237`, `FUN_0044d334` |
| `optimize.c` | 1135 | `0x004417f9` | `FUN_004416b7` |
| `tuple.c` | 1427, 1446 | `0x00441bce`, `0x00441c21` | `FUN_00441b42` |
| `sdsu.c` | 165, 328, 419 | `0x004441a6`, `0x00426359` | `FUN_004440fe`, `FUN_004262d8` |
| `dag.c` | 847, 864 | (no ret) | `FUN_0045c62c`, `FUN_0045c953` |
| `lg.c` | 834 | `0x0043aafc` | `FUN_0043aad7` |

Note also, and it matters for every A/B in the campaign: **`c2.exe` never sees the
`/O` flags.** `cl -Bd` prints the real invocations, and c2's line is only
`-il <tmp> -f <src> -W 1 -G5 -dos -Fo… -ML -Fd… -Gs4096 -Gy`. `/Og /Oa /Oi /Ot
/Oy /Ob1` all go to **`c1xx.dll`**. The optimization configuration rides in the
IL stream — see §1b.

---

## 1. REASON: what a store kills (the alias model)

1. **REASON** — an access is `(BASE, constant offset, width)`, BASE being a named
   symbol or one pointer pseudo. Disjointness is only provable inside one BASE
   kind. A store through a pointer therefore kills EVERY cached global/static
   load, and a store to a named global kills every cached pointer-based load.
2. **PASS + ADDRESS** — the `/Og` global optimizer (`globopt.c` `FUN_0040be34`,
   `globdf.c` `FUN_00403d90`). The mode word is `DAT_0049309c`, written by the IL
   option-record reader `FUN_0042fd30` (`0x0042fd54`, `0x0042fd6c`) and read at
   nine sites in eight helpers: `FUN_00431985`(+0x3a), `FUN_00431b0c`(+0x13),
   `FUN_00431b6a`(+0x53), `FUN_004322a0`(+0x1b), `FUN_00432385`(+0x13),
   `FUN_00432420`(+0x20), `FUN_004326dd`(+0x03), `FUN_0044654d`(+0x03), and the
   policy-table read at `0x0048286f`. UNCONFIRMED beyond the mode plumbing: the
   exact semantics of each helper.
3. **WHAT DECIDES IT** — nothing in the source *except* whether a value is a
   non-escaped local. The model itself is a compiler constant. Proof that it is
   the alias model and not scheduling: `/Oa` and `/Ow` flip every cell in §1a,
   and the flip is one byte in the C1→C2 IL (§1b) plus one table:
   `WORD[0x0049b8f8 + mode*2] = {6, 1, 0, 6}` — the "may alias" verdict returned
   by `FUN_00482862` is **6** at the default mode and **0** under `/Oa`.
4. **SOURCE-REACHABLE?** — **PARTIAL.** The model is fixed, but *what is memory*
   is fully source-controlled: a local whose address never escapes is a
   pseudo-register and is immune (probe `P01`/`P02`/`P03` below). Binding a
   value to a local before the store is the lever; there is no way to make two
   memory accesses disambiguate that the model does not already disambiguate.
5. **DETECTION SIGNATURE** — a load re-emitted where the value was provably still
   live (`mov edx,ds:g` twice around a store), or the reverse: retail carrying a
   register across a store/call where we reload.
6. **WORKED EXAMPLE** — the kill matrix, all one-line probes over
   `struct S{int a,b,c;}; extern int gA,gB; extern S gS,gS2,*gp; extern int arr[8];`
   compiled `/O2 /MT /GX /GR`, reading `int r = X; <STORE>; return r + X;` and
   asking whether `X` is loaded once (disjoint) or twice (killed):

   | cached load `X` | intervening store | verdict |
   |---|---|---|
   | `s->a` (param ptr) | `t->u = x` (`T*` param) | **KILLED** |
   | `s->a` | `t->a = x` (`S*` param) | **KILLED** |
   | `s->a` | `*c = x` (`char*` param) | **KILLED** |
   | `s->a` | `gInt = x` (named global!) | **KILLED** |
   | `s->a` | `arr[x] = x` (variable index) | **KILLED** |
   | `s->a` | `ext()` (any call) | **KILLED** |
   | `v->m` | `v->g()` (virtual call) | **KILLED** |
   | `gA` | `gp->b = x` | **KILLED** |
   | `gS.a` | `s->a = x` | **KILLED** |
   | `arr[2]` | `s->a = x` | **KILLED** |
   | `gA` | `gB = x` | disjoint |
   | `gS.a` | `gS2.a = x` | disjoint |
   | `gS.a` | `gS.b = x` (same symbol, other field) | disjoint |
   | `arr[2]` | `arr[3] = x` (same array, const index) | disjoint |
   | `arr[2]` | `arr[x] = x` (variable index) | **KILLED** |
   | `s->a` | `s->b = x` (same pseudo, other offset) | disjoint |
   | `s->a` | `p->b = x` where `S* p = s;` (copy) | disjoint |
   | `o->h` | `o->in.q = x` (nested subobject, const) | disjoint |
   | `o->h` | `q->p = x` where `Inner* q = &o->in;` | **KILLED** |
   | `s->a` | `*(int*)((char*)s + 4) = x` | disjoint |
   | `c->h` | `c->arr[2].a = x` | disjoint |
   | `c->h` | `c->arr[i].a = x` | **KILLED** |
   | `this->m` | `b->u = x` (`B*` param) | **KILLED** |
   | non-escaped `int loc` | any pointer store, any call | disjoint (folded) |
   | non-escaped `int la[4]` | any pointer store, any call | disjoint (folded) |

   The `(char*)s + 4` row is the sharpest: the cast is irrelevant, cl folded the
   `+4` into the addressing mode and kept BASE = `s`. **The disambiguator is
   type-blind.** The `&o->in` row is the other sharp one and it is the mechanism
   behind `docs/patterns/interior-subobject-pointer-is-a-source-local.md`: the
   interior pointer is not only a displacement device, it *creates a second BASE
   and therefore loses the disambiguation with the parent object* — which is why
   binding it changes far more than the displacements.

   Retail asm for the "same pseudo, other offset" row, from
   `CDDrawShadeBlit::ConvertRow` `0x0014ca7f`: `mov ecx,ds:0x6bf218` is emitted
   ONCE and `[ecx+8]` read from it, because nothing between them stores.

### 1a. The `/Oa` control (a negative control you can run in 4 seconds)

```cpp
struct S { int a; int b; int c; };
extern int gA; extern S* gp;
int f(int x){ int r = gA; gp->b = x; return r + gA; }
```
```
/O2            mov edx,[gp]; mov eax,[gA]; mov [edx+4],ecx; mov ecx,[gA]; add eax,ecx
/O2 /Oa        mov edx,[gp]; mov eax,[gA]; add eax,eax;     mov [edx+4],ecx
/O2 /Ow        (identical to /Oa on this cell)
/O2 /Og-       no CSE at all, ebp frame
/O2 /Ol-       identical to /O2      <-- /Ol is NOT transmitted (see §1b)
```
and on a copy loop, `/Oa` turns the bias form into `rep movsd` + `rep movsb`.
If a suspected alias wall does not move under `/Oa`, it is not the alias model.

### 1b. Where the model is configured: ONE byte in the C1→C2 `ex` stream

Capture the IL with the campaign's tap (`/d1il<prefix>` writes `…ex/gl/in/sy`).
For the probe above, `/O2` vs `/O2 /Oa` differ in **exactly one byte** of `ex`
and nothing else:

```
ex: DIFFERS   1163   0   2        gl/in/sy: IDENTICAL
```

The byte lives in an option record you can find by tag on any TU (verified on two
unrelated TUs; the record sits at `ex` offset 1156 in both, framed `4F 1F` then a
5-byte payload):

| payload byte | meaning | measured values |
|---|---|---|
| +2 | `/Oy` frame-pointer omission | `08` default, `00` with `/Oy-` |
| +3 | speed/`/Og` bits | `a0` (`/O2`), `80` (`/Og-`), `20` (`/Os`, `/O1`) |
| +4 | **alias mode** | `00` default, `02` = `/Oa`, `03` = `/Ow` |

`/Oi-`, `/Ol-`, `/Gr`, `/GX-` do not change the record at all — **`/Ol` is not
transmitted to C2 in cl 5.0**, which is why turning it off never moves a loop.
`/Ob0` and `/Od` change the stream LENGTH (the function records change shape).

Consequences worth stating plainly: the alias model is a **C1↔C2 contract**, not
a c2 switch; the Gruntz build compiles at mode `00`, the most pessimistic
setting; and no source spelling can change the mode.

---

## 2. REASON: CSE is available-expressions — no partial-redundancy elimination

1. **REASON** — a memory load is reused only where it is available on *every*
   path from its definition. A load performed on one arm is NOT reused at the
   join; a load before a guarded block IS reused inside it.
2. **PASS + ADDRESS** — `/Og`; `globopt.c` `FUN_0040be34` / `globdf.c`
   `FUN_00403d90`. Attribution is behavioural (`/Og-` removes it); the specific
   availability computation is UNCONFIRMED.
3. **WHAT DECIDES IT** — the CFG, which is a source fact.
4. **SOURCE-REACHABLE?** — **LEVER**: hoist the read to a dominating point (or
   sink it) to change how many loads are emitted.
5. **DETECTION SIGNATURE** — a load in a guarded arm AND an unconditional load of
   the same thing after the join; or, in reverse, one load where you emit two.
6. **WORKED EXAMPLE**
```cpp
int f(A* p, int c){ int t = 0; if (c) t = p->a; return t + p->a; }
```
```asm
  test  eax,eax
  je    join
  mov   ecx,[eax]        ; the arm's load
join:
  mov   eax,[eax]        ; NOT reused - a second load on the taken path
  add   eax,ecx
```
   And the dominating direction (`int r=p->a; if(c){ gB = p->a; } return r+p->a;`)
   reuses `r` for the store inside the arm (`mov [gB],ecx`) and then reloads for
   the return, because the store killed it. Both halves in one 12-instruction
   function.

---

## 3. REASON: an indirect store reloads the GLOBAL POINTER itself

1. **REASON** — `g->a = x;` is a store through an unknown pointer, and the
   *global variable* `g` is a named datum in memory, so the store kills the
   cached value of `g`. A second `g->…` therefore re-loads `g`.
2. **PASS + ADDRESS** — same as §1.
3. **WHAT DECIDES IT** — whether the source names the global again or a local.
4. **SOURCE-REACHABLE?** — **LEVER.** `S* p = g;` once, then use `p`.
5. **DETECTION SIGNATURE** — count the `mov reg,ds:<global>` instructions in
   retail and in base. Retail with FEWER loads than statements that mention the
   global means retail bound a local; retail with MORE means we bound one and
   retail did not.
6. **WORKED EXAMPLE**
```cpp
extern S* g;
int f(int x){ g->a = x; return g->b + 1; }   // two loads of g
int f(int x){ S* p = g; p->a = x; return p->b + 1; }   // one
```
```asm
; global spelling                     ; local spelling
mov eax,[g]                           mov eax,[g]
mov ecx,[esp+4]                       mov ecx,[esp+4]
mov [eax],ecx                         mov [eax],ecx
mov edx,[g]        ; <-- reload       mov eax,[eax+4]
mov eax,[edx+4]                       inc eax
inc eax
```
   This is the mechanism `docs/patterns/cse-defeat-uncached-global-rewalk.md`
   exploits from the other side (it deliberately writes through the un-cached
   global to force the re-walk), and it bounds
   `same-value-distinct-tree-defeats-arm-unification.md`'s "the global LOAD is
   one tree and CSEs" — true only while nothing stores between the two walks.

---

## 4. REASON: a store pins the source order of the next load

1. **REASON** — the scheduler may not move a load above a store it may alias, and
   by §1 it may alias almost everything. So `store; load` in source is `store;
   load` in the object, and `load; store` is `load; store`.
2. **PASS + ADDRESS** — the constraint is created in `/Og`; the motion that it
   forbids is `x86\schedmd.c` (`FUN_004500d8`…`FUN_004639f6`).
3. **WHAT DECIDES IT** — statement order in the source, exactly.
4. **SOURCE-REACHABLE?** — **LEVER**, and it is the strongest one in this domain:
   any pending store you place before a load is guaranteed not to sink past it.
5. **DETECTION SIGNATURE** — a single store sunk past the next statement's
   load/compare in base while retail keeps it in place, or the reverse.
6. **WORKED EXAMPLE**
```cpp
void f(S* s, int x){ s->a = x; s->b = gA; }   // mov [eax],ecx ; mov edx,[gA] ; mov [eax+4],edx
void f(S* s, int x){ int t = gA; s->a = x; s->b = t; }  // mov eax,[gA] ; mov [ecx],edx ; mov [ecx+4],eax
```
   This is the general form of claim (2) in
   `docs/patterns/pair-store-run-off-a-rederived-element-pointer.md` ("cl cannot
   prove an `i64*` store does not alias a later MEMBER load"). **The `i64*` is
   not doing that work — any store does.** That pattern's `i64*` device earns its
   keep through its other two mechanisms (index-fold and IV anchor); the
   order-pinning half is free with a plain member store placed in the right
   statement position, and the pattern's own measured note that the pointee type
   is irrelevant is consistent with this.

---

## 5. REASON: dead-store elimination is same-BASE, same-offset, no call between

1. **REASON** — a store is deleted only when a later store to the *same*
   `(BASE, offset)` is reachable with no intervening call and no killing store.
2. **PASS + ADDRESS** — `/Og`; `sdsu.c` (**s**tore/**d**ead-**s**tore
   **u**se?) `FUN_004262d8`, `FUN_004440fe` — name-level attribution only,
   UNCONFIRMED.
3. **WHAT DECIDES IT** — the base identity and whether a call sits between.
4. **SOURCE-REACHABLE?** — **PARTIAL**: you control whether a call sits between.
5. **DETECTION SIGNATURE** — retail has a store you deleted (a call it makes and
   you inline away), or you have one retail deleted.
6. **WORKED EXAMPLE**
```cpp
void f(S* s){ s->a = 1; s->a = 2; }              // one store: mov [eax],2
void f(S* s){ s->a = 1; ext(); s->a = 2; }       // both stores survive
```
   A missing store in base is therefore evidence about the CALL SET, not about
   the store — which is `docs/relevations/call-count-is-a-defect-oracle.md`
   arriving from the optimizer side.

---

## 6. REASON: a loop-invariant memory load is hoisted ONLY if the loop is store-free and call-free

1. **REASON** — hoisting is an availability question, so §1's kill rule applies
   per iteration. A loop body containing any store through a pointer, any store
   to a global, or any call, re-loads every memory operand every iteration.
2. **PASS + ADDRESS** — `/Og` (`globlopt.c`, `FUN_0043d4a4` / `FUN_0043de8b` /
   `FUN_0044d334`). `/Ol-` does not disable it (§1b: `/Ol` is not transmitted).
3. **WHAT DECIDES IT** — the presence of ANY store in the loop body.
4. **SOURCE-REACHABLE?** — **LEVER, and it is a one-way lever.** cl will not
   hoist across a store, so **if retail hoisted, retail's source hoisted it into
   a local**; if retail re-loads and we hoist, our source hoisted it. The hoist
   is a source fact, never a compiler mood.
5. **DETECTION SIGNATURE** — a `mov reg,<invariant>` sitting in the loop body of
   one side and in the preheader of the other, with the loop containing a store.
6. **WORKED EXAMPLE**
```cpp
void f(int* d, int n){ for (int i=0;i<n;i++) d[i] = gA; }
```
```asm
  mov eax,[esp+4]
loop:
  mov edx,ds:gA          ; INSIDE the loop - d[i] may alias gA
  mov [eax],edx
  add eax,4
  dec ecx
  jne loop
```
   Source-hoisting it (`int t = gA;` before the loop) does not just move one
   instruction — it changes the whole loop to `rep stosd`. And the same probe
   with no store at all (`t += s->a;`) hoists the member load AND closes the loop
   to `imul eax,ecx`, so the machinery is present and strong; it is the kill rule
   that gates it. This is the pass-side explanation of
   `docs/patterns/call-killed-invariant-is-a-source-local.md` and
   `global-enregistered-in-loop-no-local-mirror.md`.

---

## 7. REASON: IVs coalesce on equal constant BYTE STRIDE — pointer type is irrelevant

1. **REASON** — every loop address of the form `INVARIANT_BASE + i*S` with the
   same constant `S` joins one IV family; cl keeps ONE member as the induction
   variable and expresses the others as `[bias + IV]`, computing `bias` in the
   preheader with a `sub`. Different `S`, or a non-constant `S`, means a separate
   IV that is not reduced.
2. **PASS + ADDRESS** — `/Og`, `globlopt.c` (assert lines 3025/3066/3693/3948
   → `FUN_0043de8b`, `FUN_0043e237`, `FUN_0044d334`, `FUN_0043d4a4`).
3. **WHAT DECIDES IT** — the byte stride of each cursor, and only that.
4. **SOURCE-REACHABLE?** — **LEVER** (choose the strides), and simultaneously a
   **correction**: `docs/patterns/u16-cursor-blocks-the-induction-variable-elimination.md`
   states the cause as "cl relates IVs only of the same pointer type". That is
   **FALSE**. Measured:

   | probe | cursors | strides | result |
   |---|---|---|---|
   | `int* d, long* s` indexed | different types | 4, 4 | **coalesced** (bias) |
   | `int* d, Q4* s` (struct) | different types | 4, 4 | **coalesced** |
   | `u16* d, Q2* s` | different types | 2, 2 | **coalesced** |
   | `u8* d[2*i]`, `u16* s[i]` | **u8\* vs u16\*** | 2, 2 | **coalesced** |
   | `u8* d[i]`, `u8* s[2*i]` | **same type** | 1, 2 | **NOT coalesced** |
   | `u8* d`, `u8* s`, variable stride `s[i*rd]` | same type | 1, var | **NOT coalesced** |

   And the decisive triple: three cursors written as all-`u8*`, all-`u16*`, and
   **mixed `u8*`/`u16*`** with identical byte strides compile to **byte-identical
   objects** (probes `U00`/`U01`/`U02` below). The pattern's measured score
   improvements stand; only its stated cause needs replacing with "equal byte
   stride and an invariant base" (§8).
5. **DETECTION SIGNATURE** — a `sub <reg>,<reg>` in the preheader whose two
   operands are different buffer bases IS a bias, i.e. retail reduced an IV. Two
   biases means three participants.
6. **WORKED EXAMPLE** — retail `CDDrawShadeBlit::ConvertRow` `0x0014caac`, the
   `SHADE_DST_BY_SRC_16` arm, is textbook: `dst` and `g_scratch` share stride 2
   and coalesce; `src` has stride 1 and does not.
```asm
0014caac: mov  esi,0x6bed08        ; g_scratch
0014cab1: mov  ecx,eax             ; eax = dst
0014cab3: sub  esi,ecx             ; BIAS = g_scratch - dst
0014cab5: mov  ecx,[esp+0x20]      ; src - its own cursor, stride 1
0014cab9: lea  edi,[edx+0x1]       ; downcounter
0014cac0: mov  dx,WORD PTR [eax+esi]   ; scratch, through the bias
0014cac4: add  eax,0x2                 ; THE one IV
0014cadc: inc  ecx                     ; src, not coalesced (stride 1 != 2)
0014cae2: mov  WORD PTR [eax-0x2],dx
```

---

## 8. REASON: incrementing a PARAMETER destroys the invariant base and blocks the reduction

1. **REASON** — `BASE + i*S` needs `BASE` to be a value the loop does not assign.
   `p++` on a parameter (or on any variable with no surviving unmodified copy)
   leaves cl no base to express the others against; every cursor stays its own
   `inc`/`add` IV and a separate down-counter is added.
2. **PASS + ADDRESS** — as §7.
3. **WHAT DECIDES IT** — whether the walked pointer is the parameter itself or a
   local copy of it.
4. **SOURCE-REACHABLE?** — **LEVER**, and it is the switch between the two retail
   loop shapes.
5. **DETECTION SIGNATURE** — N independent `inc`/`add reg,K` in the loop body and
   NO `sub` in the preheader ⇒ retail incremented the parameters. One `add reg,K`
   plus `sub` pairs in the preheader ⇒ retail walked local copies.
6. **WORKED EXAMPLE** (probes `S03` vs `S04`, identical semantics)
```cpp
// copies -> ONE cursor + bias  (and `ps`, stride 1, stays its own IV)
void f(u8* d, u8* s, int count){
    u8* pd = d; u8* ps = s; u8* sc = g_scratch;
    while (count-- > 0) { u32 i = g_pal[Load16(sc) + *ps];
                          Store16(pd, (u16)i); pd += 2; ps++; sc += 2; } }
//   mov esi,offset g_scratch ; sub esi,ecx ; loop: mov ax,[ecx+esi] ; add ecx,2 ; inc edx …

// parameters -> THREE independent IVs, no bias
void f(u8* d, u8* s, int count){
    u8* sc = g_scratch;
    while (count-- > 0) { u32 i = g_pal[Load16(sc) + *s];
                          Store16(d, (u16)i); d += 2; s++; sc += 2; } }
//   loop: add ecx,2 ; add esi,2 ; inc edx ; mov ax,[esi-2] …
```
   Retail `CDDrawShadeBlit::ConvertRow`'s `SHADE_DST_BY_SRC` arm (`0x0014ca4b`)
   is the second shape — three `inc` plus `dec edi` — and its `SHADE_DST_BY_SRC_16`
   arm (`0x0014cac0`) is the first. **The same retail file uses both, so the
   shape is a per-arm source fact you must read off the bytes, not a house style.**

---

## 9. REASON: which pointer survives as the IV is set by DECLARATION ORDER

1. **REASON** — with two or more coalescable cursors, one is kept as the IV and
   the rest become biases. The choice follows the order the cursor locals are
   declared: the FIRST-declared coalescable cursor is the survivor. Five measured
   cells, no exception: two-cursor `while(n--)` probe; two-cursor + a stride-1
   third; three-cursor probe; and both declaration orders of the real
   `ConvertRow` arm.
2. **PASS + ADDRESS** — as §7.
3. **WHAT DECIDES IT** — source declaration order of the cursor locals.
4. **SOURCE-REACHABLE?** — **LEVER**, measured both in a probe and in the real
   tree. This is the missing half of
   `docs/patterns/scratch-loop-is-one-cursor-plus-biases.md`, which says "which
   pointer is the surviving cursor is per-arm — read the preheader's `sub`
   operands" without giving a way to steer it.
5. **DETECTION SIGNATURE** — the `sub` operand order in the preheader, and which
   access is register-free (`[reg]` / `[reg-K]`) rather than `[reg+bias]`.
6. **WORKED EXAMPLE** — real, in `src/DDrawMgr/DDrawShadeBlit.cpp`,
   `CDDrawShadeBlit::ConvertRow` `0x0014c9f0`, `SHADE_DST_BY_SRC_16` arm. Three
   spellings, one full `gruntz build` each:

   | spelling | ConvertRow fuzzy | surviving IV | retail's survivor |
   |---|---|---|---|
   | in tree: `i32 sc = g_scratch - dst;` (a hand-written bias) | **78.7589** | `dst` | `dst` |
   | `u8* sc = g_scratch; u8* pd = dst;` (sc first) | 78.2203 | `g_scratch` | ✗ |
   | `u8* pd = dst; u8* sc = g_scratch;` (pd first) | 78.2637 | `dst` | ✓ |

```asm
; sc declared first  -> scratch is the IV, dst is the bias
  sub  esi,ecx                     ; esi = dst - g_scratch
  mov  di,WORD PTR [ecx]           ; scratch, register-free
  add  ecx,2
  mov  WORD PTR [ecx+esi-2],ax

; pd declared first  -> dst is the IV, scratch is the bias   (== retail's shape)
  mov  esi,offset g_scratch
  sub  esi,edx
  mov  di,WORD PTR [ecx+esi]       ; scratch, through the bias
  add  ecx,2
  mov  WORD PTR [ecx-2],ax
```
   Retail `0x0014cac0`: `mov dx,WORD PTR [eax+esi]` / `add eax,2` /
   `mov WORD PTR [eax-0x2],dx` — the `pd`-first shape exactly.

   **Two things follow, and the second is why the edit was reverted.** (a) The
   bias is COMPILER OUTPUT: plain parallel cursors with invariant bases produce
   it, so `i32 sc = g_scratch - dst;` is a compiler artifact transcribed into the
   source and is not needed to get the form — the probe pair `S01` (hand bias)
   and `S02` (copies) have byte-identical loop bodies. (b) The hand bias also
   *pins the survivor*, which the copy form only does through declaration order,
   and at this site it still wins by 0.5 because the copy form spills the trip
   counter (`mov [esp+0x1c],eax` / `dec` / `mov` per iteration) where retail and
   the hand form keep it in `edi`. **The residue is register pressure, not
   form.** The A/B is reported, not kept; `src/` is unchanged on this branch.

---

## 10. REASON: the loop TERMINATOR shape follows from whether an integer index stays live

1. **REASON** — cl emits `dec ctr / jne` when the source has no integer index
   that must survive to the test (a `while (n-- > 0)`, or a `for(i…)` whose `i`
   appears only inside addresses), and `inc i / cmp i,n / jl` when the source
   walks explicit cursors *and* keeps a counted `for (i = 0; i < n; i++)`.
2. **PASS + ADDRESS** — `/Og`, `globlopt.c`; `lg.c` (`FUN_0043aad7`) is the loop
   graph. UNCONFIRMED which of the two performs the reversal.
3. **WHAT DECIDES IT** — the loop form and whether `i` is used for anything other
   than addressing.
4. **SOURCE-REACHABLE?** — **LEVER** (this is the existing
   `counted-loop-plus-cursor-vs-indexed-loop.md`; the rule above is its
   generalisation and explains its two failure modes).
5. **DETECTION SIGNATURE** — `jl` ↔ `jne` on the back edge.
6. **WORKED EXAMPLE**

   | source | emitted |
   |---|---|
   | `for (i=0;i<n;i++) d[i]=s[i];` | bias + `dec esi / jne` |
   | `pd=d; ps=s; while (n-- > 0){ *pd=*ps; pd++; ps++; }` | bias + `dec esi / jne` |
   | `pd=d; ps=s; for (i=0;i<n;i++){ *pd=*ps; pd++; ps++; }` | **index form** `[eax+esi]`/`[eax+edi]` + `cmp eax,ecx / jl` |
   | `for (i=0;i<n;i++){ *d=*s; d++; s++; }` (params) | two `inc` + `dec esi / jne` |
   | `u8* e=d+n; while (d<e){ *d=*s; d++; s++; }` | two `inc` + `cmp eax,esi / jb` |

---

## 11. PARK: which of the coalesced family gets the register-free access, once more than two cursors exist

With two cursors, §9's declaration-order rule held in every cell measured. With
three or more, plus real register pressure, the survivor choice interacts with
allocation: `ConvertRow`'s arm above reproduces retail's survivor but still
spills the trip counter. No probe separated "survivor choice" from "counter
spill" — they move together. **PARK** the residual until the value-list ordering
in `FUN_0042b3e2` (the open question in
`cl5-c2-register-picker-is-a-rotating-cursor.md`) is named; the two are the same
unknown seen from two sides.

## 12. PARK: partial redundancy

cl 5.0 has no PRE (§2). A load that appears in one arm and again at the join is
emitted twice, and there is no spelling that makes cl reuse it — only moving the
read to a dominating point does, which changes semantics if a store can
intervene. **PARK** any "retail has one load where we have two across a
diamond": the answer is that retail's source read it once, at a dominating
point, and the fix is the CFG, not a knob.

## 13. PARK: `RepathAroundBlockedTiles` / `RouteToNearbyEnemy` and the battlezmapconfig loop family

`gruntz walls diagnose 0x0002a570` classifies as **CFG** (base 37 branches / 1
ret, target 38 / 1), but the first classification is misleading here: base is
one branch and 13 instructions short with the same 18 calls because retail homes
the already-declared `coordList` pointer at entry. Around `RemoveAll`, retail
reloads that home on both paths; the taken path calls and jumps over the other
path's reload before the paths merge at the following list walk. Base derives
`&unit->m_coordList` at each use, so it needs neither the two-path reload nor the
merge jump. The extra branch is therefore downstream of allocation, not a
missing guard or arm. §6 still applies to the loop-family hoists themselves:
any store in those loop bodies forbids the hoist, so a hoist we emit is a hoist
our source wrote.
`RouteToNearbyEnemy 0x0002e3a0` initially had a superficially similar closure
signature, but its late placement was source-reachable. The failure-first source
wrote `if (RouteUnitTo(...) == 0) { Clip; return 0; }` before the success work;
retail says `test eax,eax; je <failure-tail>`, runs the success/voice/final-Clip
path, returns success, and only then emits the Clip-and-failure tail. Reconstructing
that as the positive result arm with an `else` failure tail preserved 26/26 calls
and 37/37 relocations and moved 69.3533 -> 84.88. The remaining 65/66 branch
delta is downstream of zero placement around the optional voice call: retail
materializes the timer-clear zero on both predecessors and jumps over the second
definition after the call; the base materializes it once at the join. No semantic
arm, call, return, or referent is missing at that residual.

---

## The refutable predictions

* **P1.** Adding ANY store (to a global, or through any pointer) between two reads
  of the same memory location in a cl 5.0 `/O2` function forces the second read to
  be re-emitted, *unless* both accesses have the same named symbol or the same
  pointer pseudo as their base and provably disjoint constant offsets. Falsify it
  with any counter-cell in §1's table.
* **P2.** Two cursors in one loop are coalesced (one `add`, one preheader `sub`)
  **iff** their byte strides are equal constants and each has a loop-invariant
  base. Pointer type never participates. Falsify with equal-stride cursors that do
  not coalesce, or unequal-stride cursors that do.
* **P3.** With two coalescable cursors, swapping their declarations swaps which one
  is the surviving IV and which becomes the bias. Proven in probe and in
  `ConvertRow` at `0x0014c9f0`.
* **P4.** No source change moves a wall whose diff does not change under
  `/O2 /Oa` — that wall is not an alias-model wall.

## Bounds

Measured 2026-08-18, pinned cl 5.0 SP3 under wine, `/O2 /MT /GX /GR`, against
retail `0x0014c9f0` and `0x0002a570`; c2.exe imagebase `0x400000` (Ghidra's
default), `.text` VA `0x401000` / file `0x600`, `.data` VA `0x48e000` / file
`0x8d200`. Function boundaries are Ghidra 12.0.4 headless. The score numbers in
§9 are three full `gruntz build` runs on this worktree; `src/` is unchanged
(the A/B was reverted). Probe TUs were scratch, embedded above, deleted. The
semantics of the eight alias helpers in §1 field 2 beyond "they read the mode
word" is a static reading and is UNCONFIRMED; the mode word's identity, its IL
byte, and the policy table `{6,1,0,6}` are proven behaviourally by the `/Oa`
and `/Ow` A/Bs.

## The probes, in full

```cpp
// §1 kill matrix (one function per cell)
struct S { int a; int b; int c; };  struct T { int u; int v; };
struct Inner { int p; int q; };     struct Outer { int h; Inner in; int t; };
struct C { int h; S arr[8]; int t; };
extern int gA, gB; extern S gS, gS2, *gp; extern int arr[8], arr2[8];
void ext();  int f(S* s, T* t, int x){ int r = s->a; t->u = x; return r + s->a; }
// ... vary the middle statement per the table; "disjoint" == one load + `add eax,eax`

// §7 stride vs type (the decisive triple - all three are BYTE-IDENTICAL objects)
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32;
extern u8 g_scratch[8192];
inline u32 Load16(u8* p){ return *(u16*)p; }
void f(u8* d, u8* s, int n){ u8* pd=d; u8* ps=s; u8* sc=g_scratch;
  while(n-->0){ u32 a=Load16(ps); u32 b=Load16(sc); *(u16*)pd=(u16)(a+b);
                pd+=2; ps+=2; sc+=2; } }
void f(u8* d, u16* s, int n){ u16* pd=(u16*)d; u16* ps=s; u16* sc=(u16*)g_scratch;
  while(n-->0){ u32 a=*ps; u32 b=*sc; *pd=(u16)(a+b); pd++; ps++; sc++; } }
void f(u8* d, u16* s, int n){ u8* pd=d; u16* ps=s; u8* sc=g_scratch;
  while(n-->0){ u32 a=*ps; u32 b=Load16(sc); *(u16*)pd=(u16)(a+b);
                pd+=2; ps++; sc+=2; } }
```

```sh
# the harness (2.5 s per cell, no build)
wine "$MSVC_DIR/bin/cl.exe" /nologo /c /O2 /MT /GX /GR probe.cpp
wine "$MSVC_DIR/bin/dumpbin.exe" /disasm probe.obj     # objdump cannot read MSVC5 COFF
```
