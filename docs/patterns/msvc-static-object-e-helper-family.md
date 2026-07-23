# File-scope object emits a compiler-private `$E<n>` constructor/atexit/destructor family
tags: cpp:ctor cpp:dtor cpp:static | asm:push asm:call asm:jmp | topic:identity topic:tooling
symptoms: an isolated 14-byte body does `push <adjacent helper>; call atexit; add esp,4; ret`; nearby helpers tail-jump to a constructor/destructor on one global object; FID calls the 14-byte body `__inc`; source hand-writes only the constructor helper while also defining the real file-scope object
confidence: 10/10

A real file-scope object with a non-trivial constructor or destructor makes MSVC
5 emit a family of compiler-private `_$E<n>` functions:

- a constructor helper that loads the global as `this` and tail-jumps its ctor;
- a 14-byte helper that passes the destructor helper to `atexit`;
- a destructor helper that loads the same global and tail-jumps its dtor;
- a combined initializer referenced by `.CRT$XCU`.

The numeric suffix is a per-object counter and changes when the TU's emitted
symbols change. Do not turn these into stable developer-written names, and do
not keep a hand-written constructor twin beside the compiler-emitted one. Define
the real global object and pin the retail helpers with distinct placeholder
names:

```cpp
DATA(0x00......)
CString g_name;
RVA_COMPGEN(0x000bd7f0, 0xa, _$E776176)
RVA_COMPGEN(0x000bd810, 0xe, _$E776208)
RVA_COMPGEN(0x000bd830, 0xa, _$E776240)
```

`canonicalize_data_symbols.py` recognizes both `$E28` and the x86 COFF spelling
`_$E28`, then pairs base and target by normalized bytes plus ordered relocation
targets. The placeholder number therefore has no identity meaning; the content
and relocations select the emitted helper.

## Explicit template-static specialization

A four-helper variant is stronger type evidence. For a non-trivial static data
member explicitly specialized from a class template, VC5 emits:

1. a 10-byte wrapper that calls the constructor helper and tail-jumps the
   `atexit` registrar;
2. the constructor helper;
3. the 14-byte `atexit` registrar;
4. a 31-byte destructor helper guarded by one bit in a compiler-private byte.

Multiple specializations in one TU share the byte and use successive bits. An
ordinary file-scope `CPtrList g(10)` probe instead emits the unguarded 10-byte
destructor helper; it cannot produce the 31-byte bit-test form. The guarded form
therefore justifies a real template static data member:

```cpp
template <class T>
struct CPool {
    static CPtrList s_freeList;
};

template <>
DATA(0x00......)
CPtrList CPool<CItem>::s_freeList(0xa);
```

Do not replace the member at `+0xc` with an overlapping count global:
`CPtrList::m_nCount` is part of the object and consumers should use the real
list interface. If the stripped image does not preserve the source template or
member name, use `@identity-TODO`; the helper family proves the structure, not
those spellings.

Do not test only the name recognizer. The first implementation classified
`_$E28` as an `$E` family but excluded executable sections from the definition
walker, so every helper remained unnormalized while the test stayed green.
The gate now proves that both VC5's local TEXT definition and the delinker's
external TEXT definition enter the content-addressing pipeline. The normalized
sidecar must show `family=e`, `storage=text`, a content-derived canonical name,
and a non-zero meaningful size. A same-name undefined duplicate resolved to the
unique definition must show `alias-of-definition`; an unresolved
`skipped-undefined` edge used by another helper means the contract is still
broken.

TEXT identity also excludes trailing NOPs and uses the meaningful body length,
not the containing allocation span. VC5 puts a 26-byte helper in its own
32-byte COMDAT while the delinker may pack the same body into a 28-byte span;
including either padding run or physical span in the digest makes identical
code hash differently. Data identities still retain their physical extent.

This is an acknowledged annotation-contract weakness. Names such as `_$E776208`
are globally unique RVA-derived placeholders, not semantic identities, and the
compiler's real private symbols can be renumbered or reordered by the
compiler. A future dedicated annotation should express the helper role
(constructor, atexit registrar, or destructor) and its owning global, then resolve
the emitted private symbol mechanically. Until that exists, never treat a
placeholder suffix as recovered source identity.

Audit the whole family, not only the visible 14-byte function. The address pushed
into `atexit` proves the destructor-helper boundary and global owner. The
constructor and destructor helpers prove the global's actual type. If the base
object does not emit a matching `$E` family, the source still has a storage or
class-model defect—commonly an `extern` plus `DATA_SYMBOL` standing in for the
real global—and a pin alone would hide that defect.

Evidence: `NetLobby::g_str649618` emits all three helpers automatically, and
replacing the hand-written constructor twin with `RVA_COMPGEN` pins recovered
the family at `0x000bd7f0`, `0x000bd810`, and `0x000bd830`. The 14-byte middle
helper had previously been mislabeled as LOW `__inc`.

## Inline destructor body and array variants

The constructor and destructor helpers do not have to tail-jump to separately
emitted methods. When the original class defines a small constructor or
destructor inline, VC5 can put that logic directly in the `$E<n>` helper. In
GameText, the `FreeNodePool` constructor helper clears four fields and its
destructor helper conditionally calls global `operator delete`, then clears the
same fields. Those bodies were previously modeled as developer-written
`ResetCoordPool` and `ClearCoordPool` functions. The complete adjacent
wrapper/registrar pair and their shared `g_coordPool` relocations prove that the
delete call belongs to an inline destructor, not to a manually named cleanup
routine. Put the inline methods on the real class in its shared header and
define the file-scope object normally.

An array of non-trivial objects has the same four roles, but its initializer is
the expanded element-construction run and its destructor helper calls the
vector-destruction helper. The GameText `CString[8]` name tables produce
10/121/14/20-byte wrapper, initializer, registrar, and destructor bodies.
Treat the four shared-base relocations as one family; do not name only the
semantically conspicuous initializer as if it were a developer function.

Recovering a real large file-scope object can also widen the owning TU's
ordinary-data band and expose smaller foreign definitions inside it. Audit that
warning with contribution evidence rather than reverting the object model.
For GameText, the initializer-table run proves the surrounding constructed
objects, while `g_panMinX` and `g_panMaxX` occupy the intervening ordinary BSS.
Their consumers establish scroll semantics but not storage ownership, so their
definitions belong to GameText and their declarations remain in the shared
autoscroll header. This is the reverse-use rule: xrefs name a datum; a proven
ordinary contribution homes it.

## Reverse audit of the apparent ten-function dip

The original documentation first called the NetLobby family exact without
proving that executable definitions entered normalization. Fixing that path
made ten older helpers appear to dip. That conclusion was also incomplete: raw
instructions and ordered semantic relocations agreed, while four COFF spelling
artifacts differed:

- VC5 used decorated ctor/dtor names while the delinker used `Class`/`~Class`;
- VC5 used `_atexit` while the delinker used `atexit`;
- the guarded destructor's private guard-byte symbol was independently named;
- retail COFF retained an undefined `$E<n>` duplicate beside its definition and
  parent helpers relocated through the undefined symbol.

The normalizer now maps ctor/dtor/atexit spellings to semantic roles, recognizes
only the exact 31-byte guard-site pattern, and resolves an undefined `$E` alias
only when its same-object name has exactly one candidate definition. A full
synthetic COFF test proves both the dependency hash and the rewritten relocation
name; recognizer-only coverage is insufficient.

This recovered all ten apparent losses in `AreaMgr`, `GameText`, `CImage`, and
`NetLobbyDialogs`. The template-static reconstruction then emitted twelve more
exact helpers: eight for the two Gruntz command recycle lists and four for the
network command pool. Thus the earlier “unresolved relocation/model audit
queue” was wrong: it recorded a normalization defect, not evidence against the
object models. Preserve this history because the same signature can be used in
reverse—when an `$E` family dips with identical raw instructions, audit
semantic relocation aliases and duplicate undefined helper symbols before
changing reconstructed C++.

The full-build overlap gate also falsified three LOW library labels:
`0x000238f0`, `0x00023980`, and `0x000bef30` had been called
`LIBCMT ___inittime`. Each is the 14-byte registrar inside one of the proven
four-helper families, so the labels were pruned. Use this in reverse on other
LOW `___inittime` rows, but require the adjacent wrapper, ctor, guarded dtor,
shared object relocation, and `atexit` edge; the short body alone is not enough
to reclassify an RVA.

## Activation-table reverse audit

The activation-table sweep supplied a much larger control. Fifty retail
families all have the same four boundaries relative to the constructor helper
at `X`:

- wrapper at `X - 0x20`, size `0xa`;
- constructor helper at `X`, size `0x15`;
- `atexit` registrar at `X + 0x30`, size `0xe`;
- guarded destructor helper at `X + 0x50`, size `0x1f`.

All fifty raw families were checked before changing source. Modeling each datum
as an explicit specialization of `CActRegPool<Tag>::s_table` made the 200
helpers compiler-emitted and removed the corresponding fake globals and
hand-written 21-byte initializer functions. This also invalidated 52 more LOW
`___inittime` rows. Three adjacent container-family library aliases and the
source-emitted `zDArray<CActHandler>` vtable carve-out were false for the same
reason. This is the safe reverse-use signature: do not bulk-reclassify a short
registrar without the other three boundaries and their common object
relocations.

The sweep exposed a second normalization dependency. Retail calls to a curated
body can go through a five-byte leading-band `E9 rel32` ILT thunk that still has
a stale Ghidra name. Content hashes then disagree even though the body name,
instructions, and target are correct. Propagate a curated exact-body name to
such forwarding thunks before hashing relocation targets; do not TU-attribute
the thunk itself. A recognizer test for the body name does not exercise this
failure mode.

One nearby object is an important negative control. `g_typeColl` is a real
`CTypeCollRuntime` global, not another activation-table specialization. Giving
it real storage and its `_zdvec`/`_zvec` hierarchy removes the undefined global
and the fake lookup declarations. VC5 emits its constructor helper, vtable, and
scalar-deleting destructor from the corrected owner TU, but the source shape
with a separately materialized virtual destructor does not emit the retail
`??__F` atexit wrapper at `0x0016e7a0`. Leave that compiler-generated body
unnamed until its emission trigger is recovered; manually writing a destructor
wrapper would repeat the original modeling error.

These structural corrections can lower the current aggregate fuzzy percentage:
the helper families enlarge the named-function denominator, and a previously
masked relocation alias can stop pairing even while many new functions become
exact. Preserve the MAX high-water mark, compare the exact affected helpers and
ordered relocations, and record the cause. Do not undo the object model merely
to restore the current percentage.
