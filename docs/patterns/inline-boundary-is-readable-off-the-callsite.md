# The inline BOUNDARY is readable off the call site - move the body to match it

- **tags**: `cpp:ctor` `cpp:dtor` `cpp:inline` `cpp:class` | `asm:call` `asm:jmp` | `topic:codegen-idiom`
- **confidence**: 9/10

## Symptom

A function in the 40-70 band whose logic is already right, where retail and the
recompile disagree about *where a callee's body lives*: one has a `call` the
other has the expanded body, the frame differs by one or two callee-saved
registers and one or two stack locals, and the block skeleton is off by a
handful of instructions per site.

This is not regalloc noise. It is a header/`.cpp` placement question, and the
disassembly answers it directly.

## Reading the answer

Whether a body is `inline`-in-header or out-of-line in a `.cpp` is decided by
one thing: **cl can only expand what the TU has already parsed.** So:

| what retail shows at the call site | where the body belongs |
|---|---|
| the callee's body expanded, in a TU that is not the callee's owner | a HEADER (inline) |
| a real `call` / ILT `jmp` thunk | a `.cpp`, out of line |

Cross-check the size: cl5 will not expand a body of a few hundred bytes, so a
large callee can sit inline in a header and still be CALLED everywhere.
Conversely, a body cl WILL expand must not be left out-of-line, or every caller
loses it.

## Four measured cases

- **`CWwdSpatialMgr` ctor + dtor** — `CDDrawWorkerHost::RebuildPlanes` 0x1628f0
  carries the ctor's NULL stores after `operator new` and inlines `delete`
  twice (which is where its /GX trylevel comes from).  Moving both into
  `WwdSpatialMgr.h` took it 50.47 -> 96.67.  The single out-of-line COMDAT copy
  is emitted by the TU that also calls it non-inline (`Unload`); pin it there
  with `RVA_COMPGEN`.
- **`CMenuItem::Reset`** — three of `CMenuPage`'s four new-sites carry its body
  between the six member `CString` ctors and the `Init` call, so it belongs in
  `MenuItem.h`. The two `AddItem` overloads and the simple `AddAnimatedItem` overload all went
  ~60 -> 100.00 EXACT.
  The fourth site (the largest caller) is where cl's inline budget ran out and
  retail emitted a real `call` - expect ONE sibling to read low, and check the
  budget story before believing a body is out-of-line.
- **`CUserLogic::CUserLogic`** — retail's `CGrunt` (0x47a10) and `CProjectile`
  (0xdec60) ctors open with `push <owner>; call <ILT thunk>`.  Chasing the thunk
  found a 405-byte function no unit claimed.  At 405 bytes cl cannot expand it,
  so it must be a `.cpp` body; leaving it inline in `UserLogic.h` let our
  (smaller) transcription be expanded into every derived ctor.
- **`CUserLogic::BuildLogicTypeTable`** — the converse: retail's out-of-line
  ctor expands it, so the tree keeps an `inline` copy in a header for the TUs
  that need the expansion.  That copy had DRIFTED from the out-of-line one
  (`Find()` vs `m_workers.Lookup(name, found)`).  One function, one shape - when
  a codebase keeps an inline twin of an out-of-line body, diff them.

## Corollary

`gruntz sema xref <rva>` on the out-of-line body tells you which call sites
retail left as calls; every OTHER site is one it inlined.  That is the whole
answer, and it costs one command.
