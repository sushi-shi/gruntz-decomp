# A 100%-exact function can still crash — the caller's argument protocol is unscored

**The revelation:** objdiff scores a function's **own bytes**. It never scores how its
*callers* set up the arguments. So a function can be byte-identical to retail and still be
handed garbage — and the ledger will show 100.00% right up to the access violation.

The first instance was also a **genuine cl 5.0 miscompile**: `FontRenderer::MeasureText` was
byte-exact, and the defect was in five call sites that emitted a *wrong stack protocol*.

---

## The crash

```
0178:trace:seh:dispatch_exception code=c0000005 (EXCEPTION_ACCESS_VIOLATION) addr=0x00473F03
```

`0x473F03` = `FontRenderer::MeasureText + 0x63`, which is:

```asm
473f03:  39 70 f8        cmp  DWORD PTR [eax-0x8], esi
```

`eax` is the by-value `CString` parameter's `m_pchData`, loaded from `[esp+0x28]`;
`[eax-8]` is `CStringData::nDataLength`, the length field MFC stores *behind* the character
pointer. The log says `eax = 0x32` and the faulting address is `0x2A`, i.e. `0x32 - 8`.

**`0x32` is 50.** The parameter was not a string at all — it was the integer 50, which is
`rc.top`. And `MeasureText` itself is **100.0000% byte-exact with retail**.

---

## The mechanism, in 27 lines

cl builds a by-value class argument **in place** in its outgoing-argument slot: reserve the
slot with a `push`, point `ecx` at it, run the copy-constructor. For a struct-returning
callee the hidden sret pointer must be pushed immediately *after* that, so the temp abuts
it. Break that adjacency and the callee reads the wrong slot.

Minimal reproduction — no headers, pinned cl 5.0, `/O2 /MT /GX`:

```cpp
struct TE { int w; int h; };
struct S  { char* p; S(const S& o); ~S(); };   // real copy-ctor => caller-side construction

struct R {
    int left; int right; int top;
    TE   Measure(S s);                          // sret + by-value class
    void Draw(S s, int x, int y);
    int  Width() { return right - left; }       // INLINE: nothing blocks a hoist
    void Wrapped(S& line);
};

void R::Wrapped(S& line) {
    int cx = left + Width() / 2 - Measure(line).w / 2;   // <-- one-liner
    Draw(line, cx, top);
}
```

What cl emits (the decisive window):

```asm
 c: 51              push ecx                  ; reserve the S slot -> the temp lives HERE
 d: 8b cc           mov  ecx, esp
13: 53              push ebx                  ; &line
14: e8 ..           call S::S(const S&)       ; temp constructed at the reserved slot
19: 8b 46 08        mov  eax, [esi+8]         ; this->top   <-- Draw's argument,
1c: 8b 3e           mov  edi, [esi]           ;                 HOISTED above the call
1e: 8d 4c 24 10     lea  ecx, [esp+0x10]
22: 50              push eax                  ; <-- lands BETWEEN the temp and its call
23: 51              push ecx                  ; sret
24: 8b ce           mov  ecx, esi
26: e8 ..           call Measure              ; ret 8 -> reads arg2 = [S-8] = top
```

`Measure` reads `top` as its `S`, and `ret 8` pops the hoisted value, so `Draw`'s own frame
shifts too. Change **only** the spelling to bind the result first:

```cpp
TE m = Measure(line);
int cx = left + Width() / 2 - m.w / 2;
Draw(line, cx, top);
```

and the same compiler emits the correct sequence — ctor, **one** push (the sret), call:

```asm
 c: 51              push ecx
 d: 8b cc           mov  ecx, esp
13: 57              push edi                  ; &line
14: e8 ..           call S::S(const S&)
19: 8d 44 24 10     lea  eax, [esp+0x10]
1d: 8b ce           mov  ecx, esi
1f: 50              push eax                  ; sret, immediately after the ctor
20: e8 ..           call Measure
```

### Why this is a compiler bug and not a reading

Established by **differential compilation**, not by staring at one listing. One source, one
callee, five flag sets:

| flags | temp vs. the sret push | verdict |
| :-- | :-- | :-- |
| `/Od /GX` | adjacent | correct |
| `/O2 /Ob0 /GX` | adjacent | correct |
| `/O2 /MT` (no `/GX`) | adjacent | correct |
| **`/O2 /GX`** | **8 bytes too high** | **wrong** |
| **`/O1 /GX`** | **8 bytes too high** | **wrong** |

A callee's argument layout cannot depend on the *caller's* optimization flags. Three cells
agree, two disagree — so the two `/GX`-optimized cells are the wrong ones. Retail's own
binary sides with the correct cells: `MeasureText` reads its `CString` at `entry_esp+8` and
does `ret 8`.

Trigger isolated with three further `/O2 /GX` cells: an `int`-returning inner callee is
**correct** (no hidden sret push), a by-value **POD** is correct (no ctor-materialized
temp), and using constants instead of `rc.top`/`z` is still **broken** (so it is not about
which values get hoisted). `/GX` is required.

---

## Why retail never hit it

Retail's `CRect::Width` is **out of line** at `0x17b500` — a call inside the expression
blocks the hoist, exactly as `/O2 /Ob0` does. We inline it.

**Refuted, and worth recording:** switching the TU to no-inline MFC does *not* work.
`NAFXCW.LIB` exports neither `?Width@CRect@@QBEHXZ` nor `??0CRect@@QAE@HHHH@Z`, so the
candidate dies with `LNK1120: 2 unresolved externals` and nine further TUs report the
`CRect` constructor. That failure is itself evidence: retail's `0x17b500` is a **COMDAT
from retail's own font compiland**, not a library import — so reproducing the out-of-line
call needs that *body* defined on our side, not a header switch.

---

## The fix

```diff
--- a/src/Font/Font.cpp
+++ b/src/Font/Font.cpp
                 if (hcenter) {
-                    i32 cx = rc.left + rc.Width() / 2 - MeasureText(line).width / 2;
+                    TextExtent le = MeasureText(line);
+                    i32 cx = rc.left + rc.Width() / 2 - le.width / 2;
                     DrawLine(line, surf, cx, rc.top, z);
                 } else {
```

Five sites in `FontRenderer::DrawWrapped`. It also moves *toward* retail, which reads the
extent from its stack slot (`mov eax,[esp+0x40]`); we now do too. `DrawWrapped`
73.27 → 74.29.

**Tree-wide screen, mechanical:** disassemble the linked candidate, find every `mov ecx,esp`
followed within a few instructions by a `call`, and flag any site with **≥ 2 pushes** before
the consuming call. **Zero real hits remain.** The 7 residual matches are benign —
`CGruntzMgr::BuildLevelRezPath(int,int,int,int,CString)`, where the class parameter is the
callee's *last* declared parameter, so the temp legitimately sits highest.

---

## Why every metric missed it

1. **objdiff scores the callee, never the caller's argument protocol.** `MeasureText` was
   100.00% EXACT throughout. No ledger row can point at a function whose *callers* set it up
   wrong.
2. **A caller's fuzzy % dilutes a three-instruction defect.** `DrawWrapped` sat at 73.27%,
   dominated by legitimate schedule and inline divergence. A two-push stack-protocol error
   is invisible in that, and the function was already parked in the "big divergence, later"
   bucket.
3. **`--diff` masks address operands.** Both sides print `push <reg>` / `call <tgt>`. The
   defect is the *count and position* of pushes relative to a ctor call — a shape no
   per-instruction diff flags.
4. **Nothing in the pipeline models the ABI contract.** `insn_seq --seq` keeps only
   reloc-carrying instructions, so a plain `push edi` is dropped entirely.
5. **The EH cascade hides the primary fault.** The log shows the real access violation once,
   then hundreds of `RtlUnwindEx` frames and secondary faults through the MSVC EH band.
   Reading from the end of the log tells you nothing.

**Corollary, and the reason this file exists:** *byte-exactness of a function is not
evidence that its callers are correct.* When a 100% function faults, do not re-examine the
function — go read its call sites, and read them as a **protocol** (push order, push count,
who pops), not as instructions.

---

## The general class

This is the caller-side sibling of a defect family that has now produced several
user-visible bugs, all invisible to the score:

* a **missing call** — a function that fails to *do* something still matches everywhere it
  does (`docs/patterns/missing-call-site-is-invisible-to-every-percent-view.md`);
* a **wrong argument** — a direction index passed where a pixel coordinate belonged;
* a **wrong receiver** — a one-slot member slip leaving a pointer uninitialised, which then
  crashed a 100%-exact `StopAndRewind`;
* and this: a **wrong protocol**, where the arguments are right and the *stack shape* is not.

Full mechanism and the tree-wide screen:
`docs/patterns/by-value-class-temp-stranded-above-the-sret-slot.md`.
