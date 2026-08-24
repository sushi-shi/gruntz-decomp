# Count the calls: a missing call is invisible to every percentage

**The revelation:** compare the **multiset of callees** between our object and the delinked
retail object, per function. A function that fails to *do* something still matches
everywhere it does — so a missing call costs almost nothing in fuzzy %, yet it is a
feature-dead or crashing bug. Three of this project's user-visible defects were exactly
this, and the metric never flinched.

The cheapest, highest-yield sieve we have. It found the bug that made the game unplayable.

---

## Worked example: the level was never loaded

**Symptom:** every way of starting a game failed with *"Unable to set the game state.
(1055)"* / *"(1056)"*, and the log showed an access violation at
`CPlay::LoadByMode + 0x858` reading through a NULL `m_mainPlane`.

**What the score said:** `LoadByMode` sat at **83.84%** — unremarkable among 880 functions
below 100%. Nothing pointed at it.

**What the call census said:** retail calls something we never call. At `LoadByMode+0x243e`
retail has:

```asm
8b 16                 mov  edx, DWORD PTR [esi]          ; vptr
55                    push ebp                           ; reload flag
8b ce                 mov  ecx, esi
ff 92 a8 00 00 00     call DWORD PTR [edx+0xa8]          ; <-- ABSENT ON OUR SIDE
85 c0                 test eax, eax
0f 84 ec 05 00 00     je   fail0
```

`vtable_hierarchy` resolves slot `+0xa8` of `??_7CPlay@@6B@`:

```
+0xa8  ?BuildWorldLevelPath@CPlay@@UAEHH@Z
```

That call drives the entire level load — `BuildWorldLevelPath` → `LoadFromSource` →
`CGameLevel::LoadWwd` → `ReadPlane`, and `ReadPlane` is the **only writer of
`m_mainPlane`**.

**Why nothing caught it.** Our `BuildWorldLevelPath` *existed*, was correct, and was
reachable through the vtable — so no unresolved-symbol check, no coverage gate, and no
"unmatched body" report could fire. It simply had **no caller**. `LoadByMode` then ran the
whole asset chain over a level that had never been loaded, and dereferenced NULL.

**The fix** restored the call; verified in the relinked binary at the same offset, with slot
`0xa8` resolving through the ILT thunk to `?BuildWorldLevelPath@CPlay@@UAEHH@Z`.
`LoadByMode` 83.84 → **87.45**, and its call sequence is now retail's **161/161**.

---

## How to build it (30 lines, no new infrastructure)

For each function present in both `build/objdiff/base/<unit>.obj` and the delinked
`build/objdiff/target/<unit>.c.obj`, extract the **ordered list of callee names** from the
relocations (`llvm-objdump -dr`), then diff the multisets.

```
retail-only callee   -> we never call it            <- THE DEFECT BUCKET
ours-only callee     -> we call something retail does not
count differs        -> usually cl cross-jumping, see below
```

`FontRenderer::DrawWrapped +7`, `CInGameIcon` ctor `+12`, `CWarlord` ctor `+6` …

---

## The two traps — read these before believing a row

**1. Adjudicate by NAME, not by COUNT.** This is the whole discipline. cl freely
**cross-jumps two argument-identical call sites into one**, so a count delta is usually
codegen, not a defect. Of **68 rows** adjudicated in one sweep, exactly **one** was a
genuine missing call; the dominant class was retail keeping two copies where our compiler
merged them. `CWwdSpatialMgr::Relocate` looked like five missing `RemoveAll` calls and had
all four sites present in source — cl had merged them.

**2. An unrelocated `call` in a *delinked target* object is an intra-symbol self-call.** The
delinker has already resolved the displacement, so no relocation remains; cl emits a `REL32`
for the same call. Naively dropping unrelocated calls makes every self-recursive function
look like "ours-only recursion". Acting on that took `CButeMgr::Parse` from **98.12 → 63.75**
before retail's own bytes at `0x13c/0x197/0x1ca` proved retail recurses too. Reverted; sieve
fixed.

A third, milder one: the delinked names carry **library-label ambiguity** —
`CImageList::Attach` vs `CGdiObject::Attach`, `ifstream::close` vs `ofstream::close`,
`CMapStringToOb` vs `CMapStringToPtr`. About 30 rows of one sweep were this, not defects.

---

## Yield

| defect | how the census showed it |
| :-- | :-- |
| `LoadByMode` never called `BuildWorldLevelPath` | retail-only indirect call at slot `+0xa8` — **game unplayable** |
| `UpdateEntranceAnim` called an **empty placeholder** | retail-only `LoadGruntAbilityTuning`; ability tuning never loaded after the toy-break anim |
| `StepArrivalCommit` never set up the toob animation | retail-only `SetupTubeAnim` after the actKey swap |
| `HandleTextInputKey` inlined what retail calls | retail-only out-of-line `Reset` ×2 |

Plus a whole-program complement worth running once: symbols retail's `.text` references and
we reference **nowhere**. Only 4 survived, all benign — a useful bound on how much of this
class is left.

---

## Where it sits among the sieves

Five sieves now exist because each is blind to what the others see. In yield order:

| sieve | catches | blind to |
| :-- | :-- | :-- |
| **call-name census** | a call we never make | everything not a call |
| the immediate multiset (`gruntz walls diagnose --asm`) | a wrong mask / divisor / tag | relocated operands |
| the store offsets (`gruntz walls diagnose --asm`) | a wrong or missing **member store** | reads; loaded-pointer bases |
| `gruntz verify assert-relocs` | a wrong **addend** into a datum | non-relocated values |
| `gruntz.delink.eh_band --census` | a wrong member/base **type** | non-destructible members |
| `gruntz walls diagnose` | a missing guard, inverted polarity | *poor hit rate* — mostly block layout |

**Corollary:** percentage measures *similarity of what is present*. It cannot measure
**absence**. Every "what did retail do that we don't" question needs its own census, and the
call census is the one with the best yield per line of code.

Related: `docs/patterns/missing-call-site-is-invisible-to-every-percent-view.md`,
`docs/relevations/byte-exact-can-still-crash.md` (the caller-side sibling: the calls are all
present, the *protocol* is wrong).
