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
- **Caller SIZE alone does nothing.** 2 sites at the end of a caller padded with
  0 → 200 non-inlinable statements (86 → 3779 instructions): both sites inlined
  at every size. Only *other inline expansions* consume the budget.

`/O2` is `/Ob1` here: cl 5.0 does **not** auto-inline a function that is not
marked `inline`, at any definition position (before, after, or interleaved with
its callers — all three measured, all emit 2/2 calls). So a plain out-of-line
member never gives you the inline half.

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

## Evidence

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
