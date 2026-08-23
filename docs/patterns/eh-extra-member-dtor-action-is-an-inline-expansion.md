# An extra `??1CString` unwind action is an inlined MEMBER destructor, not a temporary we invented

tags: cpp:eh cpp:dtor cpp:inline cpp:member msvc5:mfc | asm:call asm:lea | topic:wall topic:negative-control
symptoms: `walls ehactions` shows our side running a destructor retail's action sequence lacks
  (`[ebp-0x14c] -> ??1CString@@QAE@XZ` between two actions both sides share); `walls diagnose`
  says `base calls, target expanded/lacks: ??1CString@@QAE@XZ`; a `count` verdict with our
  funclet count higher; the temptation to hunt for a `CString` returned by value
confidence: 9/10
variants: inline-vs-call-is-decided-per-site.md, last-member-ctor-gets-no-unwind-state.md

An unwind action naming a destructor the other side never runs looks like a destructible
temporary one side materialises. It usually is not. The other way to reach a member's
destructor is to INLINE the enclosing object's destructor: the member then needs its own
cleanup STATE in the caller, and the caller grows a funclet for it. There is no expression to
delete, because the member is not a temporary at all.

## The measurement that decides it

Count the retail DIR32 references to the base class's vtable. A destructor cl inlined restamps
the base vptr, so every inline expansion of `~CBase` leaves one reference; the out-of-line copy
leaves exactly one more.

    gruntz sema xref / refs_to_range on ??_7CFileMemBase@@6B@   ->  FOUR sites, image-wide:
      CFileMemBase::CFileMemBase+0x32     the ctor's own stamp
      CFileMemBase::~CFileMemBase+0x1f    the out-of-line dtor's own restamp
      CFileMem::~CFileMem+0x40            the derived dtor, inlining its base
      LoadRecordFile+0x37                 ONE inlined expansion in a caller

Retail inlines the same destructor in `LoadRecordFile` and calls it in `SnapshotChildren`. So
the in-class body IS the original source shape, and "move it out of line" is the WRONG fix -
it would break `LoadRecordFile`.

`CDDrawSurfaceMgr::SnapshotChildren` (0x156020) reads
`[ebp-0x158] CFileMemBase / [ebp-0x148] CFile / [ebp-0x14c] CString` where retail reads only
the first two. `-0x158` is the `CFileMem S;` local, `-0x14c` is `S+0xc` = `CFileMemBase::m_name`
and `-0x148` is `S+0x10` = `m_file`. The `CString` is the MEMBER. Our build spent one more
inline expansion of the in-class `~CFileMemBase()` than retail's did at that site.

The same function carries the sibling case in its prologue: retail emits
`call ?Reset@CFileMem@@UAEXXZ` where we expand the in-class body (four zero stores plus
`call ?Empty@CString@@QAEXXZ`). `?Reset@CFileMem` has exactly two direct retail callers -
`SnapshotChildren` and `RestoreChildren` - and `LoadRecordFile` expands it inline, at
0x156b43..0x156b57, four zero stores and the same `CString::Empty`. Again: in-class is right,
and the site decision is cl's.

## Verdict

Both rows are the per-site inline BUDGET class (`inline-vs-call-is-decided-per-site.md`,
`walls inline-model --gap`), not an authored cleanup defect. Do not go looking for a
`CString` returned by value, a `const char*` implicitly converted at a call, or a
concatenation - there is none. Before treating any extra unwind action as a temporary,
count the base vtable's references and check whether RETAIL inlines the same destructor
somewhere else in the image.

Already REFUTED experimentally, from the other direction, in
`last-member-ctor-gets-no-unwind-state.md`: moving `CFileMem::Reset` out of the header buys
`RestoreChildren` 93.84 -> 96.89 and `SnapshotChildren` 70.12 -> 71.61 and costs
`LoadRecordFile` 100.00 -> 75.96. The vtable-reference count predicts that outcome without
building anything.
