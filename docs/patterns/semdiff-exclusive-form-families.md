# Five instruction-form families explain most semdiff store/imm exclusives

tags: cpp:member cpp:struct cpp:pointer | asm:mov asm:and asm:lea | topic:method topic:tooling topic:scoring-artifact
symptoms: `walls semdiff` prints a store or imm key on ONE side of a paired
function; the operand looks like a missing member write, a wrong mask, or an
extra field read; the referent sequence is clean
confidence: 9/10

The 2026-08-22 full-queue semsweep (599 todo rows) flagged ~45 rows with
exclusive keys. Adjudicating each against the retail bytes reduced them to a
handful of real defects (the fort-recolor player index, the MidiManager
ctor, the Toolhelp32 clears, Run's two error arms) plus FIVE recurring
instruction-form families that are semantically equal. Check these families
BEFORE reading an exclusive as a missing statement:

1. **RMW fusion.** One side writes `and/or DWORD [mem],imm` (one memory-op,
   no store key); the other loads, masks, stores (`store +0x0` + the same
   imm). `CInGameIcon::PlaceAt`'s "missing" flags store is retail's split
   form of our fused `and [ecx+eax*4],0xfffbffff`. The BYTE subset is real
   source signal though: retail's `or BYTE [cell+3],0x20` against our dword
   RMW was the m_rowBytes byte-view spelling (ClaimSwitchTile, fixed).

2. **Struct-copy first-store.** cl copies a 16-byte member as
   `mov [base+OFF],r; lea p,[base+OFF]; mov [p+4],r2; ...` on one side and
   entirely through the lea pointer on the other. Shows as `store +OFF`
   exclusive with the other side holding `imm OFF`. Measured NOT spellable:
   field-wise assignment makes it worse (CSBI_WellGoo::Setup 6 exclusives),
   the whole-object assignment is already the right source (CGrunt ctor's
   four rects, PathScan's IntersectRect fail arms). Selection nuance; park.

3. **`&member` materialization.** Retail computes the member's address once
   (`add reg,OFF`) and reads small displacements through it; ours folds into
   one big displacement (or vice versa). Shows as paired
   `disp +BIG base-only` / `imm OFF target-only`. A POINTER LOCAL closes it
   where the address has multiple uses (`RECT* view = &cam->m_viewRect` in
   OnLButtonDown/OnKeyDown, the GRID_RECT_INLINE_PTR tail) and is
   copy-propagated into nothing at single-use sites (CStaticHazard's
   `m_animCursor.m_consumeDraw` store) - measure, don't assume.

4. **AL-accumulator peephole.** `imm 0xe0 target` paired with
   `imm 0xffffffe0 base` (and kin: 0xef/0xffffefff, 0xfa/0xfffffffa) is
   `and al,imm8` against `and reg,simm8`: cl emits the 2-byte AL form only
   when the value lands in EAX. Same mask, same source; which register the
   rotation hands the value is the coin. Retail itself mixes both forms
   (SetArrivalTarget is EXACT with the dword form in edx). Park.

5. **add-negative vs lea/sub.** `imm 0xfffffff9`/`0xffffffe0`-class keys on
   one side against `-0x7`/`-0x20` displacements on the other are
   `add reg,-N` vs `lea/sub` spellings of the same subtraction - the I4a
   lifetime reading (wall-reasons-allocation.md). Park unless the whole
   function's census says otherwise.

What survived as REAL after the families: a register-dataflow index bug
(AdvanceAnim recoloring by the LOSER's m_players entry - invisible to every
multiset, visible only by reading the [esp+N] slot discipline across the
push), a missing ctor (MidiManager - store exclusives at a `new` site fed
by zero registers), sized-clear idioms (Toolhelp32 memset past dwSize), and
a branchless-vs-two-arm merge (Run's 0x41c/0x41d). The screen is the sweep;
the adjudication is the retail dataflow read.
