# A repeated one-line container call in a builder is an inline MEMBER — its expansions are what cut the ctor chains out-of-line

tags: cpp:inline cpp:ctor cpp:new cpp:member | asm:call asm:eh | topic:codegen-idiom topic:comdat
symptoms: retail `call`s a header-inline constructor (or a mid-chain base ctor) at some
`new T` sites and flattens the chain at others; our build flattens every site, so the
constructor's COMDAT is emitted by no object, its `RVA()` label drops (`gruntz labels
--all` FATAL, "not a code symbol in <unit>.obj"), and the caller's `/GX` unwind-state
count is short. `gruntz walls diagnose` says INLINE/CALL-SET with matching branch and
`ret` counts.
confidence: 9/10 (2026-08-16, one A/B on the pinned cl 5.0 against the real tree)
variants: inline-budget-emits-ool-comdat.md, ctor-inline-cut-depth-varies-per-new-site.md,
zero-emission-statements-cross-the-ob1-cb-exemption.md

## The corollary that reframes the problem: the out-of-line ctor body is FREE

A derived constructor's standalone COMDAT reproduces retail **byte-for-byte with no
source device at all**. cl expands the base ctor into it and dead-store-eliminates the
base's vptr stamp and any field the derived body rewrites:

```cpp
class CStatusBarItem { public: CStatusBarItem(); virtual ~CStatusBarItem(); /*...*/ };
inline CStatusBarItem::CStatusBarItem() {
    m_enabled = 0; m_kind = SBI_KIND_BASE; m_host = NULL; m_redrawFrames = 0;
}
class CSBI_RectOnly : public CStatusBarItem { public: CSBI_RectOnly(); /*...*/ };
inline CSBI_RectOnly::CSBI_RectOnly() { m_kind = SBI_KIND_RECT_ONLY; }
```
```asm
; ??0CSBI_RectOnly@@QAE@XZ, retail 0x101fa0, 0x1b B - reproduced verbatim
8b c1  mov eax,ecx        ; base body: its vptr store and its m_kind=0 are dead
33 c9  xor ecx,ecx
89 48 04 / 89 48 24 / 89 48 28    ; m_enabled / m_host / m_redrawFrames
c7 00 <??_7CSBI_RectOnly@@6B@>    ; derived vptr
c7 40 08 01 00 00 00              ; m_kind = 1
c3
```
So "what source shape inlines the base INTO the derived while both bodies still exist?"
is a non-question. **Emission is the only variable**: the COMDAT appears iff some site
DECLINES the inline. Stop looking for a spelling of the constructor and go find the
missing decline.

## The lever: expansions spend the budget, hand-written statements FUND it

cl 5.0 `/O2` implies `/Ob1`: `budget = clamp(2*cb(caller), 1000, 35000)`, spent
sequentially, nested expansions get `budget / sites-remaining`
(`gruntz walls inline-model`). The asymmetry that matters:

| source form | effect on the caller's budget |
|---|---|
| N statements written out in the caller | **+2N** (they raise `cb(caller)`) |
| the same N statements behind an inline call | **-N** (charged when expanded) |

A repeated one-line member call is therefore a ~3x budget swing per site while emitting
**identical bytes**. That is what recovers retail's out-of-line ctor population - not a
cost knob on the constructor.

## The recipe

Find a statement the builder repeats once per constructed object that is a plain member
operation on a member container, and give it the member function the devs wrote:

```cpp
// include/Gruntz/StatusBarMgr.h
void AddTabItem(i32 tab, CStatusBarItem* item) { m_tabLists[tab].AddTail(item); }
```
```cpp
- m_tabLists[0].AddTail(statzTab);
+ AddTabItem(0, statzTab);
```
`lea ecx,[this+0x2c]; push item; call ?AddTail@CPtrList@@` is emitted either way.

## Measured (71 sites in src/Gruntz/SBI_RectOnly.cpp)

| caller | fuzzy before | after | ctor `call` referents before -> after | retail |
|---|---:|---:|---|---|
| `CStatusBarMgr::BuildStatusBarTabs` 0xffde0 | 85.55 | **94.42** | {} -> {base 2} | {base 4} |
| `CStatusBarMgr::BuildGameMenu` 0x101580 | 81.02 | **86.05** | {base 1} -> {RectOnly 1, base 5} | {RectOnly 5, base 3} |
| `CStatusBarMgr::LoadTabSprites` 0x102250 | 83.77 | **89.58** | {base 3} -> {RectOnly 5, base 12} | {RectOnly 10, base 15} |
| `CStatusBarMgr::BuildTabzDialog` 0x10a340 | 79.66 | **86.47** | {base 2} -> {RectOnly 2, base 7} | {RectOnly 4, base 8} |

`??0CSBI_RectOnly@@QAE@XZ` (0x101fa0) materialises in `sbi_rectonly.obj` and scores
**100.00 EXACT**; `??0CStatusBarItem@@QAE@XZ` (0x1005d0) stays 100.00. Every
`__ehunwind$?BuildStatusBarTabs...` funclet goes 0 -> 100 because the kept ctor calls
are throwing and restore retail's unwind-state count. Whole-image 93.97 -> 94.13%.

## Negative controls (all disposable, none retained)

* **Caller `cb` padding is real but cannot reach retail.** `((void)0)` x64/x256 in
  `BuildStatusBarTabs` removes 1 / all 3 cuts that a callee-cost probe had produced -
  so the budget is NOT capped there - but reaching retail's 4 cuts from the natural
  chain needs the equivalent of ~340 caller statements REMOVED. The caller is not the
  defect.
* **Constructor cost knobs pick the wrong referent.** `((void)0)` xN inside
  `CSBI_RectOnly::CSBI_RectOnly` gives 0/0/1/3 RectOnly-calls at N=8/16/24/32 but only
  after piling base-ctor calls on first; the same probe in `CSBI_Image` behaves
  identically; in `CStatusBarItem` it produces base calls ONLY. No single knob
  reproduces the mixed population (confirms the retraction in
  ctor-inline-cut-depth-varies-per-new-site.md).
* **Declaration POSITION of the helper is inert.** Declaring `AddTabItem` first in the
  class or beside `ClearTabGroup()` gives byte-identical objects; the +1 declaration's
  TU-state ripple (3 rows in `play` / `statusbartabbuilders` / `triggermgrgrid`) is
  identical either way.
* **`SbGeom` (the sibling inline at every site) is a live but weaker lever** - budget
  slices at one site are coupled, so its cost also cuts chains. It is already in the
  source; do not tune it.

## How to find the missing member

The deficit is `retail calls - our calls` and it is EXACTLY the ctor-call deficit when
the reconstruction is otherwise complete (branch and `ret` counts already agree - check
that first with `gruntz walls diagnose`). Then grep the caller for the statement it
repeats once per object: a container `AddTail`/`AddHead`/`SetAt`, a two-field store, a
one-line setter. `docs/patterns/inline-budget-emits-ool-comdat.md`'s `CMenuPage::AddSubItem2`
closure is the same shape (header-visible `SetFrame` plus two one-store command setters,
63.12 -> 100 EXACT).

## Follow-up 2026-08-16: the deficit is CANDIDATE SITES, and it is now measured

Re-running the four builders after `AddTabItem` landed, with the referent population read
per constructed object (split the call stream at `??2@YAPAXI@Z`, compare group-by-group
against the delinked target):

| caller | ours | retail | short by |
|---|---|---|---|
| BuildStatusBarTabs 0xffde0 | {base 3} | {base 4} | 1 cut |
| BuildGameMenu 0x101580 | {base 4, RectOnly 2} | {base 3, RectOnly 5} | 2 cuts |
| LoadTabSprites 0x102250 | {base 11, RectOnly 6} | {base 15, RectOnly 10} | 8 cuts |
| BuildTabzDialog 0x10a340 | {base 7, RectOnly 2} | {base 8, RectOnly 4} | 3 cuts |

The `__ehunwind$...$N` funclets sitting at **0.00** are a free readout of the same number:
2 / 2 / 8 / 3 dead funclets, exactly the missing throwing ctor calls, no disassembly needed.

**Titration (disposable, all removed).** A zero-emission candidate site is
`static inline i32 ProbeSite(i32 v) { return v; }` called as `(void)ProbeSite(0);` - it is
an inline call for the front end and emits nothing.

* **BuildStatusBarTabs: SOLVED, 100.00 EXACT.** The titration said the caller was short
  exactly 2 zero-emission candidate sites anywhere after the 7th `new` (`static inline i32
  ProbeSite(i32 v){return v;}` called as `(void)ProbeSite(0)`: K=2 gives retail's `{base 4}`
  and 99.72, K=1 and K=3 do not). The two missing sites were not unnameable - they are two
  ordinary one-store setters this class family is already known to have
  (inline-budget-emits-ool-comdat.md closed `CMenuPage::AddSubItem2` 63.12 -> 100 on exactly
  this shape, and `CMenuItem::SetCommandParam` / `SetSecondaryCommandId` / `CMenuItem2::SetFrame`
  are the surviving in-tree examples):

  ```cpp
  // include/Gruntz/StatusBarItem.h          // include/Gruntz/SBI_Image.h
  void SetEnabled(i32 on) { m_enabled = on; }  void SetFrame(CImage* frame) { m_frame = frame; }
  ```
  ```cpp
  -        if (f != NULL) { multiTab->m_frame = f->GetAt(IDX(MENUITEM_DISABLED)); }
  -        multiTab->m_enabled = 0;
  +        if (f != NULL) { multiTab->SetFrame(f->GetAt(IDX(MENUITEM_DISABLED))); }
  +        multiTab->SetEnabled(0);
  ```
  Identical bytes at both sites, 33 calls and `{base 4}` exactly as retail, **97.35 -> 100.00
  EXACT**. The function is now its own negative control: `multiTab->m_state = MENUITEM_DISABLED`
  in the SAME block must stay a raw store, because a third site there breaks it. That also
  retires the "the `m_tabSpriteN` / `m_notifyN` stores are setters" hypothesis for good - Tabs
  is exact with all five of them written as plain assignments.

  **The general lesson, and it is the important one: a population that is short is a
  MEASUREMENT that the source is not yet the devs' source.** It is not a residue to name and
  park. Read it as "N entities missing" and go find them.

* **cb reduction alone is inert here.** Factoring the whole `if (SINGLE) {...}` disable block
  (5 statements) into one file-static inline changed nothing, so for this caller the lever is
  the `budget / sites-remaining` divisor and not `2*cb`.
* **A uniform +1 site per constructed object OVERSHOOTS.** It gives {base 2, RectOnly 1} /
  {RectOnly 4, base 3} / {RectOnly 13, base 12} / {base 6, RectOnly 6} - only BuildGameMenu
  lands near retail. Probing at the "remember the pointer" assignments (44 sites) gets
  LoadTabSprites' base count exactly right (15) and Dialog to 88.20, but again cuts a dock
  button in BuildStatusBarTabs. **So the missing sites are NOT one per item**, and the
  heterogeneous `m_tabSpriteN` / `m_notifyN` / `m_groupNotify[i]` stores are not setters.
* **Hoisting BuildGameMenu's mission-status branch to the top is refuted a second time.** It
  is tempting because retail's EH state indices number the two mission-status `new` sites 0
  and 1 - i.e. the FRONT END saw them first - while laying that branch out last, and those two
  sites are exactly the ones our build never cuts (they are last in our tuple order, so they
  get the largest slice). Writing `if (m_itemKind == GAME_TAB_MISSION_STATUS) { ...; return 1; }
  ...menu...` flips the emitted layout with the numbering: 86.06 -> **65.23**. Hoisting only the
  `CSBI_ImageSet* status;` declaration is byte-identical. How C1 can order those sites first
  while C2 emits them last is still unexplained, and it is the whole of BuildGameMenu's residue.

Two real reconstruction bugs were found on the way and are worth checking first, because both
LOOK like budget residue: an indirect call through the wrong vtable slot
(indirect-call-slot-names-the-receiver-type.md), and `p->m_frame = f ? f->GetAt(i) : NULL;`
where retail writes `if (f != NULL) { p->m_frame = f->GetAt(i); }` - the null path jumps PAST
the store, which the masked diff hides (masked-diff-hides-branch-target.md). Recovering
`CDDrawWorker::GetAt` at that one site (it was spelled out as
`f != NULL && f->m_minIndex <= i && f->m_maxIndex >= i` + `f->m_items.GetAt(i)`) plus the `if`
form took BuildStatusBarTabs 94.42 -> **97.35**.
