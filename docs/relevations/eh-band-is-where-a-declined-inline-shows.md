# A declined inline is invisible in the main band and loud in the EH band

**The revelation:** when our compiler declines to expand an inline body that retail expanded,
the main-band percentage barely twitches — the caller still does the same work, just through a
`call`. But if the declined body owned a **destructible temp**, that temp never entered the
caller's frame, so the caller is a whole **unwind state short**. Every funclet after the missing
one shifts by one index and compares against the wrong retail funclet, and the last one has no
partner at all.

So the funclet band answers a question the main band cannot: *is a body that retail expanded
here actually here?* Count the funclets before believing the percentage.

---

## Worked example: `CUserLogic::AttachToObject`

`CWarlord::CWarlord` and `CInGameIcon::CInGameIcon` are the two largest logic constructors
(0x750 B and 0x15f0 B). Both carried a `call` where retail has the body.

**What the score said:** nothing actionable. `CWarlord` sat at 70.99% — one of hundreds of
constructors in the "big divergence, park it" bucket. Whole-tree fuzzy was 92.94%.

**What the funclet count said:** we were exactly one short.

```
$ llvm-nm build/objdiff/normalized/base/warlord.obj   | grep -c 'ehunwind\$??0CWarlord@@QAE@PAUCGameObject@@@Z\$'
27
$ llvm-nm build/objdiff/normalized/target/warlord.c.obj | grep -c 'ehunwind\$??0CWarlord@@QAE@PAUCGameObject@@@Z\$'
28
```

### The missing state, in retail's own bytes

Retail's `??0CWarlord@@QAE@PAUCGameObject@@@Z` reserves 0x1c bytes of locals and builds a
`zBitVec` **on the stack** partway through:

```asm
95: 83 ec 1c              subl  $0x1c, %esp             ; 16 B more than ours
...
a8: 8d 5e 18              leal  0x18(%esi), %ebx        ; &this->m_actBits
b3: e8 ..                 calll ??0zBitVec@@QAE@XZ      ;   member, default-constructed
b8: 8b 7c 24 3c           movl  0x3c(%esp), %edi        ; obj
bc: c7 06 ..              movl  $0x0, (%esi)            ; ??_7CUserLogic@@6B@
c2: 89 7e 0c              movl  %edi, 0xc(%esi)         ; m_logicObject = obj
c5: 89 7e 10              movl  %edi, 0x10(%esi)        ; m_object      = obj
c8: 8b 47 7c              movl  0x7c(%edi), %eax        ; obj->m_animWorker
cb: 55                    pushl %ebp                    ; 0
cc: 68 ..                 pushl $??_C@_00A@?$AA@        ; ""
d1: 8d 4c 24 24           leal  0x24(%esp), %ecx        ; &tmp        <-- A STACK TEMP
d5: c6 44 24 3c 01        movb  $0x1, 0x3c(%esp)        ; EH state := 1
da: 89 46 14              movl  %eax, 0x14(%esi)        ; m_objAux
dd: e8 ..                 calll ??0zBitVec@@QAE@PBDH@Z  ; zBitVec tmp("", 0)
e2: 8d 4c 24 1c           leal  0x1c(%esp), %ecx
e6: c6 44 24 34 02        movb  $0x2, 0x34(%esp)        ; EH state := 2
```

The two `movb`s write the **same slot** — the callee is `__thiscall` with two pushed args and
returns `ret 8`, so `esp` at `0xe6` is 8 higher than at `0xd5`, and `0x34+8 == 0x3c`. State 1 is
the member, state 2 is the temp. That second state is the whole story.

Ours built the same object through a call, with a 0xc-byte frame — no temp, no state 2:

```asm
15: 83 ec 0c              subl  $0xc, %esp
...
3a: 8b ce                 movl  %esi, %ecx
3c: 57                    pushl %edi
42: c7 06 ..              movl  $0x0, (%esi)            ; ??_7CUserLogic@@6B@
48: e8 ..                 calll ?AttachToObject@CUserLogic@@QAEXPAUCGameObject@@@Z
```

`llvm-nm build/objdiff/base/*.obj | grep AttachToObject` named **exactly two** objects —
`warlord.obj` and `ingameicon.obj`. The other 56 derived constructors expanded it fine, so this
was never a size problem: the chain is `CWarlord::CWarlord` → `CUserLogic::CUserLogic(obj,
INLINE_BASE)` → `AttachToObject`, and cl's greedy per-caller budget ran out at **depth 2** in the
two biggest callers.

### What the band showed — an off-by-one, not a mismatch

| state | ours (before) | retail |
| :-- | :-- | :-- |
| `$0` | `mov -0x10(%ebp),%ecx` → `~CUserBase` | `mov -0x20(%ebp),%ecx` → `~CUserBase` |
| `$1` | `mov -0x10(%ebp),%ecx; add 0x18` → `~zBitVec` | `mov -0x20(%ebp),%ecx; add 0x18` → `~zBitVec` |
| `$2` | `mov -0x10(%ebp),%ecx` → `~CUserLogic` | **`lea -0x1c(%ebp),%ecx` → `~zBitVec`** ← the temp |
| `$3` | `mov …; add 0x34` → `~CWapX` | `mov -0x20(%ebp),%ecx` → `~CUserLogic` |
| `$4` | `mov …; add 0x54` → `~CString` | `mov …; add 0x34` → `~CWapX` |
| `$5` | `lea -0x14(%ebp)` → `~CString` | `mov …; add 0x54` → `~CString` |
| … | every later state shifted one early | |
| `$27` | **does not exist** | `lea -0x24(%ebp)` → `~CString` |

Two things are wrong at once, and both are the same cause. The **targets** disagree from `$2`
on (the census calls this `different-targets`), and every `[ebp+disp]` is 16 bytes off (the
census calls this `frame-offset`) because the absent temp shrank the frame. One source change
moved both classes.

### The fix

MSVC 5.0 has no `__forceinline`, and `#pragma inline_depth` is not usable here
(`msvc5-inline-depth-zero-is-the-only-live-lever.md`). The period device for a block that must
expand at **every** site is a textual macro:

```diff
--- a/include/Gruntz/UserLogic.h
+++ b/include/Gruntz/UserLogic.h
-    void AttachToObject(CGameObject* obj);
-
-inline void CUserLogic::AttachToObject(CGameObject* obj) {
-    m_logicObject = obj;
-    m_object = static_cast<CWwdGameObjectA*>(obj);
-    m_objAux = obj->m_animWorker;
+#define USERLOGIC_ATTACH_TO_OBJECT(obj)                                        \
+    m_logicObject = (obj);                                                     \
+    m_object = static_cast<CWwdGameObjectA*>(obj);                             \
+    m_objAux = (obj)->m_animWorker;                                            \
     {                                                                          \
         zBitVec tmp("", 0);                                                    \
         m_actBits = tmp;                                                       \
     }                                                                          \
     ...
-}
```

expanded in **both** `CUserLogic` ctor entities — the header-inline one and the pinned
out-of-line one in `MotionState.cpp`.

### Measured, two full builds on the same tree

| | inline member | macro | Δ |
| :-- | --: | --: | --: |
| `AttachToObject` COMDATs | 2 | **0** | −2 |
| **EH funclets exact** | 2676 / 3034 | **2706 / 3034** | **+30** |
| main-band exact | 3465 / 4328 | 3466 / 4328 | +1 |
| main-band fuzzy | 92.94% | 92.89% | −0.05 |
| `CWarlord` ctor funclets | 27 | **28** (= retail) | +1 |

`+30 exact funclets for a 0.05 fuzzy dip.` The per-function picture is blunter still: at the
time of the change `CWarlord` went 70.99 → 78.11 while `CInGameIcon` went **92.16 → 79.86**.
One of the two callers got *worse* in the main band and the band still says the change is right,
because the funclet chain is now retail's chain. **Never trade funclets back for fuzzy.**

---

## The second reading of "short a state": a `new` whose ctor stayed a CALL

`AttachToObject` above is the *temp* case — a destructible object that never entered the
frame. There is a second, commoner producer of the same signature, and it points the opposite
way: **an `X* p = new X;` whose constructor was DECLINED needs an unwind state of its own**, to
hand the raw allocation back to `operator delete` if the ctor throws. Expand the ctor and the
state disappears; decline it and the state appears. So in a factory full of `new`, the funclet
count is not a census of temporaries — it is a census of *declined constructor calls*.

Measured on `CStatusBarMgr::LoadTabSprites` `0x102250`, 37 `new` sites, 2026-08-21:

| | out-of-line `??0CSBI_RectOnly` | out-of-line `??0CStatusBarItem` | sum | EH funclets |
| :-- | --: | --: | --: | --: |
| ours | 7 | 10 | **17** | **17** |
| retail | 10 | 15 | **25** | **25** |

The identity is exact on both sides, and it holds per site: aligning the two streams by the 37
`??2@YAPAXI@Z` calls, exactly the eleven sites whose declined depth differs carry the funclet
difference, and every other segment is instruction-for-instruction equal. The `imm 0x13..0x17`
exclusives `walls semdiff` reports there are the state counter running further, not constants.

**The trap.** Read "we are 8 funclets short" as "we are missing 8 destructible temporaries" and
you will go looking for eight `CString`s that do not exist. The check that separates the two
cases costs one command: count the out-of-line base-ctor calls on each side
(`llvm-objdump -dr` + the `??0` relocations). If that count already explains the delta, the
funclets are a *consequence* of the inline decision and the work is
[cl5-inline-budget-is-arithmetic-you-can-compute.md](cl5-inline-budget-is-arithmetic-you-can-compute.md)'s
front-end-mass problem, not a missing object.

A second, independent axis on the same function confirms the mechanism is the nested share
`trunc(budget / sites-remaining)` and not the top-level budget: inserting K calls to an empty
`static inline` (cost-exempt, so they raise `nrem` without spending budget) after the last
construction moves the declines monotonically — K=0 → 7/10, K=2..4 → 8..9/9..10, K=10 →
12/8 plus one declined `??0CSBI_Image`, and the score 93.13 → 94.41. Neither axis alone reaches
retail's 10/15: raising `nrem` and lowering caller mass move the two buckets differently, which
is the same "the caller carries ~250-300 cb units too much" instruction stated from the other
side. Probes were disposable and are not in the tree.

## Why the main band cannot see this

A declined inline is not a wrong instruction. The caller loads the same arguments, the callee
does the same work, and the only new byte is a `call`. Everything downstream still matches. The
cost is spread thin enough that a 70%-ish constructor absorbs it invisibly, and constructors of
that size are exactly the rows nobody looks at.

The funclet band has the opposite property: it is **one funclet per destructible object in the
frame**, so a body that did not expand deletes a whole entry and the comparison desynchronises.
It is a count, not a percentage, and counts can measure absence.

---

## The counter-example — when a state-count delta is NOT a defect

`eh_band --check` compares our `maxState` against retail's straight out of both FuncInfo
records, and 11 groups differ. **Two of them are not bugs:** `CKeyedList::AddNode` and
`~CMoviePlayer` are byte-exact bodies whose extra/missing state has a **NULL action** — dead
metadata cl emitted around a scope with nothing to destroy. There is no code cost and no source
change that would help. Park them.

The other direction has a second false alarm: two states that unwind the same object share a
funclet **address**, so the funclet-address census silently collapses them. That is why
`--check` reads the integers out of FuncInfo instead of counting funclets — the count-based
instrument found 3 groups where the real number is 11.

---

## How to run it

```
gruntz.delink.eh_band --census --top 40   # identical / different-targets / frame-offset / permuted
gruntz.delink.eh_band --check             # our maxState vs retail's, per group
```

Read a `--check` row like this:

* **we are SHORT a state** → retail constructs a destructible object in this frame and we do
  not. Suspect a declined inline first; confirm with
  `llvm-nm build/objdiff/base/*.obj | grep <mangled callee>` — if only a handful of objects
  name the callee and retail expands it in those, it is being declined, not mis-modelled.
* **we are LONG a state** → we materialize a temp retail does not, usually a `CString` from an
  expression retail wrote to avoid one.
* **targets differ but counts match** → the type is wrong, not the count; that is
  [funclet-is-a-type-oracle.md](funclet-is-a-type-oracle.md).
* **targets match but `[ebp+disp]` differs** → local layout / construction order.

Full mechanism and the four-spelling comparison (splitting the inline in two does **not** buy
depth; flattening it by hand costs more than the macro):
`docs/patterns/inline-depth-two-declines-in-the-largest-caller.md`.
