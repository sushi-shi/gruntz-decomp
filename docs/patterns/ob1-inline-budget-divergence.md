# /Ob1 inline-budget divergence — the SAME ctor is inlined at one call site and called at another

**Tags:** `topic:wall` `cpp:ctor` `cpp:inline` `asm:call` `topic:eh`

## Symptom

A constructor (or any small member) appears in retail **both ways**:

* it has a real out-of-line body at some RVA (a COMDAT the linker kept), **and**
* some callers `call` that RVA while other callers have its stores expanded inline.

Your reconstruction can only produce one of the two, and which one you get is decided
by where you put the definition:

| definition placement | every call site |
|---|---|
| **in-class** (header) | inline candidate → cl expands it wherever its budget allows |
| **out-of-line** (`.cpp`) | never a candidate → always a `call` |

MSVC 5.0's `/O2` is `/Ogityb1`, i.e. **`/Ob1`**: only `inline`/`__inline` functions and
member functions **defined inside the class declaration** are inline candidates. An
out-of-line definition in the *same* TU is *not* one (unlike `/Ob2`, which VC5 `/O2`
does **not** imply). So placement is a hard, binary switch — and retail's mixed
behaviour is the *inliner's own budget*, not two different definitions.

## Worked example (the wide-object factories, 0x159250 / 0x159440 / 0x159600 / 0x1598d0)

`CGameObject::CGameObject(CDDrawSurfaceMgr*, i32, i32)` has a COMDAT at `0x15b390`.

* `CreateObject_159250` (C kind), `_159440` (F), `_159600` (A) **expand** it whole and
  `call` the sub-object ctors inside it (`0x15b2c0` CResolveNode, `0x15b2a0` grid node,
  `0x15b270` dirty-rect, `??0CString`, `0x15b300` worker).
* `CreateObject_1598d0` (B kind), `CWwdGameObject::CreateObject` (`0x166640`) and
  `CDDrawWorkerHost::ReadPlaneObjects` (`0x162af0`) **call** `0x15b390`.
* Inside `0x15b390` itself all five of those sub-ctors are **expanded** — the same
  functions the factories call.

Consistent reading: every one of these ctors is in-class; MSVC's inliner works with a
per-function accumulated budget, so the *first* expansion at a site consumes it and the
nested ones stay as calls, while in the small standalone COMDAT the budget is fresh.

**Our cl has a more generous budget than retail's build had.** With every ctor in-class
it expands the whole chain and the factories land at 62/59/61/42 %. The pragmatic
optimum is to place each definition where it buys the most sites:

| function | placement | why |
|---|---|---|
| `CResolveNode::CResolveNode(mgr,i32,i32)` @0x15b2c0 | **out-of-line** | 3 factories `call` it |
| `AnimWorkerObj::AnimWorkerObj(mgr,i32,i32)` @0x15b300 | out-of-line | 3 factories `call` it |
| `CAniAdvanceCursor::CAniAdvanceCursor(mgr,i32,i32)` @0x15b730 | out-of-line | `_159600` calls it |
| `WwdDirtyRect::WwdDirtyRect()` @0x15b270 | out-of-line | 3 factories `call` it |
| `WwdGridNode::WwdGridNode()` @0x15b2a0 | out-of-line | `_159250`/`_159440` `call` it |
| `WwdRegion::WwdRegion()` @0x15b2b0 | **in-class** | `_159250`/`_159440` expand it |
| `CGameObject::CGameObject(...)` @0x15b390 | **in-class** | 3 factories expand it |

That took `_159250` and `_159440` to **100 % EXACT** and `_159600` to 95.7 %.

## The cost, and why it is not fixable

Two demands genuinely conflict — `_159250`/`_159440` expand `WwdRegion::WwdRegion()`
while `_159600` calls it; three factories expand `CGameObject::CGameObject` while three
other sites call it. **MSVC 5.0 has no `noinline`** (`__declspec(noinline)` is VC7+), and
`#pragma inline_depth(n)` is depth- not size-based, so it flips the wrong set (it would
also block the `+0x1a0` cursor expansion that `0x1598d0` *does* have).

A second, quieter consequence: **when nothing calls an in-class function out-of-line,
cl emits no COMDAT for it, so its RVA cannot be labelled at all.** `0x15b390` (296 B)
and `0x15b2b0` (14 B) are unclaimed for exactly that reason — the retail bytes exist,
but no base obj has the symbol to pair them with. Budget the trade before you make a
definition in-class.

## Related

* `docs/patterns/base-trio-in-ctor-body-misplaces-vptr.md` — where the vptr stamp lands.
* `docs/patterns/rezalloc-placement-new-no-eh-frame.md` — the wall this work retired:
  `RezAlloc` + placement-new emits no `/GX` ctor-in-flight frame; a real `new T(...)`
  does. Superseded for the wide-object factories.
