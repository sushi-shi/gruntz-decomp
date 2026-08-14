# Retail has BOTH an OOL COMDAT and inline expansions: cl's per-caller inline budget

tags: cpp:inline cpp:call | asm:call | topic:codegen-idiom topic:wall
symptoms: one retail function both expands a body in place AND `call`s the
out-of-line copy of the same body; making the function `inline` in a header makes
cl inline it everywhere, emit no COMDAT, and the labels gate fails
`DROPPED N -> N-1`; the temptation to reach for an address-take or a pragma
confidence: 9/10

cl 5.0 keeps ONE inline-expansion budget **per calling function**, shared across
every inline it expands there. While the budget lasts each site is expanded; once
it is spent the remaining sites become real `call`s — and it is those declined
sites that make cl emit the out-of-line COMDAT. That is the whole mechanism behind
retail's "both shapes"; no source device is involved, so **write the ordinary call
at every site and let cl choose**.

```cpp
// header
inline void CMapMgr::Clip(const tagRECT* src) { ... }   // 170 B body
// caller - just call it, at every site
board->Clip(&box);
...
m_board->Clip(NULL);
```

Measured thresholds (probe TU, real unit flags `/nologo /c /O2 /MT /GX /GR`,
86-instruction inline body):

| caller content | Clip sites | expanded | real calls | COMDAT |
|---|---|---|---|---|
| nothing else | 1-3 | all | 0 | no |
| nothing else | 4 | 3 | 1 | **yes** |
| nothing else | 8 / 12 / 24 | 4 / 7 / 14 | 4 / 5 / 10 | **yes** |
| 4 expansions of a *different* inline | 2 | 1 | 1 | **yes** |
| 6+ expansions of a different inline | 2 | 0 | 2 | **yes** |

Two boundaries matter and both are measured:

- **The budget is per CALLER, not per TU.** 32 separate one-site callers in one
  compiland: every site inlined, no COMDAT. So you cannot buy the COMDAT by
  adding call sites in *other* functions.
- ~~**Caller SIZE alone does nothing.**~~ **RETRACTED 2026-08-11 — the caller's
  own size is the budget's PRIMARY input.** The original experiment (2 sites at
  the end of a caller padded 0 → 200 statements, both inlined at every size) was
  SATURATED: with only two sites nothing was being rejected at pad=0, so no
  amount of extra budget could change the outcome. Re-run at a configuration
  that actually rejects (callee S=8, N=12 sites), the coupling is flatly
  visible:

  | caller pad statements | 0 | 5 | 10 | 20 | **40** | **80** | 160 |
  |---|--:|--:|--:|--:|--:|--:|--:|
  | sites expanded (of 12) | 9 | 9 | 9 | 9 | **10** | **12** | 12 |

  The plateau at 9 is the 1000 floor; it breaks at ~36-40 caller statements,
  which is where `2 x cb(caller)` first exceeds 1000. See § "The rule".

`/O2` is `/Ob1` here: cl 5.0 does **not** auto-inline a function that is not
marked `inline`, at any definition position (before, after, or interleaved with
its callers — all three measured, all emit 2/2 calls). So a plain out-of-line
member never gives you the inline half.

## The rule (ported from the homm3 VC6 back-end RE, re-validated on cl 5.0)

The sibling homm3 project reverse-engineered this decision out of its pinned
back end (C2.DLL 12.00.8447) and published it as a model spec + address ledger
+ executable predictor: `homm3-decomp/docs/vc6/inliner.md`, `predict()` in
`homm3-decomp/scripts/homm3/vc6/inline_model.py`
(`--predict --spec sites.json`, `--measure-cb`, `--selftest`). **Do not
re-derive it. Feed it.**

```
budget  = clamp(2 * cb(caller), 1000, 35000)      # cb = the FRONT END's size
running = cb(caller)                              #      estimate of the caller
for each candidate site, in tuple order, n counting DOWN:
    reject if budget < cb(callee) and cb(callee) > 0x28     # cb<=0x28 is FREE
    reject if running > 35000
    accept: if cb > 0x28: budget -= cb ; running += cb
            recurse into the callee's body with budget / (sites REMAINING)
```

Consequences that decide real cases: the spend is **sequential and positional**
(the last sites lose); nested budgets shrink as `budget / sites-remaining` at
every level, so "depth-2 stops" are that division and not a depth limit; and a
callee with `cb <= 0x28` is inlined regardless of budget or site count.

**cl 5.0 (C2 11.00) shares the rule AND the cost function.** 25-site harness,
PAD=0 (budget pinned at the 1000 floor), statements `gA[i] = gA[i+1] + row;`,
`python3 -m gruntz.core.inline_model --gen-harness S N PAD` (S callee
statements, N sites, PAD caller statements ahead of them; count REJECTED
sites as `call` + tail `jmp` — cl tail-jump-optimizes a rejected final site.
`/Ob0` enumerates a real function's candidate set: every expansion becomes a
visible `call`):

| callee statements S | 1-2 | 3 | 4 | 6 | 8 | 12 | 13 | 14 | 15 |
|---|--:|--:|--:|--:|--:|--:|--:|--:|--:|
| expanded (of 25), cl 5.0 | 25 | 20 | 16 | 11 | 9 | 6 | 5 | **5** | **5** |
| expanded (of 25), VC6 | 25 | 20 | 16 | 11 | 9 | 6 | 5 | **0** | **0** |

Identical for S = 1..13 - so `floor(1000/cb) = expanded` titrates `cb` the same
way on our compiler, and homm3's measured brackets transfer verbatim. **The one
divergence is the save-gate cliff:** VC6 stops saving the callee body at S=14
and every site becomes a call; cl 5.0 has no such cliff in this range and keeps
expanding 5. Treat homm3's §4 cliff as VC6-only.

## Calibrating a REAL caller: what charges the budget, and in what order

Measured on `CStatusBarMgr::BuildStatusBarTabs` (0xffde0) in its own TU, adding
K statements just before the last call. Only the *kind* differs:

| K statements added | ctor chains cut |
|---|---|
| `L55Burn(this, k)` — a 3-op `static inline` — ×6 | **4** |
| the same six calls with the helper **not** `inline` | **0** |
| the helper's body written out by hand, ×6 and ×20 | **0** |

So the charge is against **inline expansions**, not against calls and not
against code volume — 20 hand-written copies of the identical statements move
nothing while 6 expansions of them move four sites. The sweep 1..8 gives
0,0,0,1,3,4,4,5 cuts, and the cuts appear at the **earliest** deep chain first
and spread forward: the LAST `new` in the function keeps its full expansion
longest. (That is the shape retail's BuildStatusBarTabs has — sites 4-7 cut,
site 8 fully inlined — but see
[two-shapes-need-two-entities](two-shapes-need-two-entities.md): for that family
the real cut is per CLASS, and the budget reading was a coincidence.)

`/Ob0` is the cheap way to **enumerate** a caller's inline expansions: every one
becomes a visible `call`. BuildStatusBarTabs: 8 `SbGeom` + 5 `CSBI_MenuItem` +
3 `CSBI_RectOnly` + 1 `CObArray::GetAt` = 17. No plain flag reproduces an
intermediate cut, so flags are not a lever here: `/Ob0` calls *everything*
including `SbGeom`, `/O1` cuts at the OUTERMOST ctor (depth 1), and
`/Ox`, `/Gy`, `/Gf`, `/Gd`, `/Og /Oi /Ot /Oy /Ob1 /Gs` and dropping `/GR` are
all byte-identical to `/O2`.

Two candidate constructs for "the inline we are missing" were tested and both
are **refuted** — record them so they are not re-tried:

- **`CTypedPtrList<CPtrList, T>`** (its `AddTail` is a zero-code forwarder, so 8
  sites buy 8 free expansions). Retail says no: `~CStatusBarMgr` (0xc8980) hands
  the vector-dtor iterator `??1CPtrList@@UAE@XZ` (0x1b48c6, the MFC library's
  own) with element size 0x1c. A typed wrapper overrides CObList's virtual dtor
  and would have supplied its own COMDAT there. `m_tabLists` is a plain
  `CPtrList[8]`.
- **`CRect(l,t,r,b)`** instead of a local `RECT`-returning inline: byte-identical
  AND budget-identical (both cut nothing). Our `SbGeom` is a faithful stand-in.

## When the budget will not bite: what NOT to do

If your reconstruction of the caller inlines less than retail's did, cl will
expand every site and no COMDAT appears. Three devices *do* force it, and all
three are **fitted artifacts — do not land them**:

| device | COMDAT | sites still inlined | verdict |
|---|---|---|---|
| `static void (C::*p)(...) = &C::M;` | yes | yes | artifact: adds a .data slot with no retail counterpart |
| non-static file-scope PMF | yes | yes | artifact, and needs a bogus `DATA()` pin |
| `__declspec(dllexport)` | yes | yes | artifact: writes a `/EXPORT:` into .drectve |
| `#pragma auto_inline(off)` around the definition | **no** | yes | no effect on an explicitly-`inline` function |
| header `inline` + a second out-of-line definition | — | — | C2084 "already has a body" |

`#pragma inline_depth` is ignored by cl 5.0 (measured twice); do not confuse it
with `auto_inline`.

The legitimate ways to get the COMDAT without touching the callers are the
*address-take* ones, because they are real C++ the devs wrote:
[`??_H` on a member array](inline-ctor-comdat-via-vector-ctor-iterator.md) and an
[inline dtor called from EH funclets](eh-funclet-band-owns-the-inline-dtor-comdat.md).
If none applies, the honest state is that the caller is still under-inlined
relative to retail — that is a reconstruction gap in the caller, not a missing
device.

## Worked example: `CGruntzMgr::ChangeState` 0x8fab0 - the divisor, not the mass

Retail `call`s `??0?$CArray@PAUPLAYLISTINFOSTRUCT@@PAU1@@@QAE@XZ` inside the
inlined `CMoviePlayer::CMoviePlayer`; we expand it. Diagnosed end-to-end with
the rule, three measurements, no guessing:

1. **`cb` of the callee.** A stand-in with the same shape (vptr stamp + 4 zero
   stores) at 25 sites, PAD=0: 15 expanded ⇒ `floor(1000/cb) = 15` ⇒
   **cb in [63, 66]**. Above 0x28, so it is charged - not a free callee.
2. **The candidate set of the caller,** enumerated with `/Ob0`: `??0CMoviePlayer`
   x1, `??1CMoviePlayer` x2, `Teardown` x2, `??BCString` x2 = **7 sites**. The
   construction is near the TOP, so the sites *after* it are what divide.
3. **The lever.** Adding throwaway *free* (`cb <= 0x28`) candidate sites, which
   raise the divisor without spending budget:

   | probe sites added AFTER the construction | 0 | **6** | 10 | 14 | 20 |
   |---|--:|--:|--:|--:|--:|
   | `??0?$CArray<PLAYLISTINFOSTRUCT*>` emitted as a CALL | no | **yes** | yes | yes | yes |

   Six extra sites flip it to retail's shape and it never flips back. Adding
   the same probes *ahead* of the construction does nothing (they lower its
   divisor, not raise it) - which is exactly the model's `budget /
   sites-REMAINING`, and is the trap to avoid when probing.
4. **What is NOT the lever:** caller statement mass. Padding `ChangeState` with
   20 and 60 statements never moved the decision, it only inflated the body
   (74.84 -> 30.01 -> 0.00 fuzzy).

Verdict, and it is a measurement rather than a wall: **our `ChangeState` is
missing roughly six inline call sites that retail's had after the
`CMoviePlayer` construction** - more `CString` conversions / `CMoviePlayer`
calls / scoped objects between the construction and the return. Finish the
caller and the CArray call follows; no spelling of the callee, no pragma and
no caller padding can substitute, and each was measured.

## Evidence

### Worked example: byte-identical setters close `CMenuPage::AddSubItem2`

`CMenuPage::AddSubItem2` at `0x183850` was parked at 63.1176% because our
constructor expanded `CMenuItem::Reset`, while retail called the standalone
COMDAT. The first three sibling factories already expanded the same body and
were exact, so moving `Reset` out of line merely exchanged one correct site for
three regressions.

The old explanation called this generic TU state. A 192-trial mixed declaration
sweep was completely flat, as were 128 dead locals and 16 redundant stores in
`Reset`. The missing charge was three semantic inline call sites whose expanded
instructions were already present as direct stores:

- `CMenuItem2::CMenuItem2` calls the header-visible virtual `SetFrame(0x64)`;
- `AddSubItem` and `AddSubItem2` use separate inline `SetCommandParam` and
  `SetSecondaryCommandId` setters.

The standalone `SetFrame` COMDAT remains exact through its vtable binding, both
sub-item factories retain the same store order, and all four factories are
exact. In the largest factory these three candidate sites make cl decline the
nested base `Reset`: 102 instructions, eight blocks, two returns, and every
ordered relocation now agree with retail. A combined command setter alone, the
two command setters without header-visible `SetFrame`, and header-visible
`CMenuItem2::Reset` were negative controls.

This is the reverse-use rule: when direct stores already match but one nested
inline decision does not, look for existing one-store methods or natural field
setters that the reconstruction flattened. Their bodies can be byte-neutral
after expansion while their call sites remain visible to the front-end budget.
Raw dead statements and parser-state noise do not substitute for those sites.

`CMapMgr::Clip` (0x2b340, 0xaa) has four retail rel32 callers and is expanded in
~13 more places. Making it a header `inline` and writing real calls: cl expanded
**every** site in every TU — including `CGrunt::PathScan`'s three and
`CBattlezMapConfig::RouteToNearbyPickup`'s four — so no obj emitted the COMDAT
(`battlezmapconfig` 39 → 38 labelled functions) and the two converted callers got
worse: HandleUnitContact 85.93 → 77.41, RouteToNearbyPickup 80.54 → 61.72.
Reverted. The blocker there is the callers' inline content, not the device.

variants: [inline-ctor-comdat-via-vector-ctor-iterator.md](inline-ctor-comdat-via-vector-ctor-iterator.md),
[inline-expanded-twice-costs-a-register.md](inline-expanded-twice-costs-a-register.md),
[shared-inline-transcribed-once-per-call-site.md](shared-inline-transcribed-once-per-call-site.md)
