# Repeated field xrefs collapse interior globals into one object
tags: cpp:class cpp:inline | data:bss data:layout | asm:mov asm:call | topic:identity topic:mis-model
symptoms: several declared-only globals occupy consecutive word offsets; callers access them in the same order as the fields of a known class and reproduce one of that class's inline methods
confidence: 10/10

An address label at every referenced word can shred one retail object into fake
globals. Contiguous addresses alone do not prove the correction, but a complete
field-offset map plus a repeated method body does.

The former GruntVoice activation declarations covered:

```text
0x006514d8  collection/base object
0x006514dc  error sink
0x006514e0  low bound
0x006514e4  high bound
0x006514e8  allocation base
0x006514ec  spare/current entry
0x006514f0  element stride
0x006514f8  scratch/grown count
```

Those offsets are exactly the inherited `_zvec`/`_zdvec`/`zDArray` layout of
the real `CActReg g_actReg_6514d8`. Retail
`CGruntVoice::FireActivation` reads and writes the same fields, calls
`_zvec::GrowTo` with `ECX = 0x006514d8`, and invokes
`CVariantSlot::Set` through the field at `+4`. Replacing the eight field names
with `g_actReg_6514d8.ResolveEntry(coord)` makes the function instruction
identical.

The safe recovery procedure is:

1. Map every apparent global address to a candidate object's field offset.
2. Check the class hierarchy and full inherited layout, not only the leaf
   declaration.
3. Verify receiver setup at calls and field access widths in retail
   disassembly.
4. Compare the whole access sequence with a known or reconstructed method.
5. Replace consumers with the real object/member operation and remove the
   interior declarations.

Do not satisfy the declared-only ratchet by defining storage at each interior
address. That would create overlapping objects and conceal the actual class
model. If the outer object's construction or storage class is still unclear,
keep that one boundary symbol unresolved and record the remaining evidence;
the proven interior aliases can still be removed.

Use this pattern in reverse when auditing other dense data bands. A cluster of
word-spaced declared-only names is a strong candidate when multiple consumers
repeat the same bounds check, stride calculation, grow call, error path, and
spare-entry return.

Applying that reverse search to `CGrunt::ArrivalRecycle` found a second form.
The labels at `0x006bf658..0x006bf668` were not a separate "cell" collection:
they are `g_typeColl`'s inherited `m_lo`, `m_hi`, `m_base`, `m_spare`, and
`m_stride` fields at `+0x08..+0x18`. Four placeholder `zDArray` methods in the
same sequence also hid existing operations:

```text
MapCellIndex   -> _zvec::GrowTo
MapCellRecord  -> zErrHandling::Report
PinCellIndex   -> GetRetAddr
MapCellRecord2 -> CVariantSlot::Set through m_errSink
```

Two remaining "cell record" globals were aliases of already modeled storage,
not fields: `0x006bf464` is `g_projActCache`, and `0x006bf428` is
`g_retAddrBreadcrumb`. The latter two addresses are confirmed by the slow
path's relocation order: load the cache, obtain the return address, store the
breadcrumb, then call `m_errSink->Set(&g_typeColl, cache, 0xc)`. Modeling the
real fields, methods, and globals removed four false function declarations and
four declared-only data names. It also removed the overlapping
`g_zvecErrSentinel` definition at the same address as `g_projActCache`.

The same callee audit removed the last two one-site aliases from this vector
API. In `CBattlezMapConfig::CanPlaySpecialAnim`, placeholder
`zDArray::Probe` calls retail `_zvec::GrowTo` at `0x0016da80`, while
placeholder `zDArray::Reserve` calls the already reconstructed
`zErrHandling::Report` at `0x00034960`. Naming those existing inherited
operations preserved the function's 80.7091 fuzzy score and removed two more
declared-only functions.

A related one-site alias began at the correct object boundary rather than at
an interior offset. `g_gruntDefEntranceCell[3]` claimed `0x006448e8`, and the
`CGrunt` constructor read the base, `+4`, and `+8` words. That exact-base
collision was enough to remove the overlap, but the first replacement type was
still provisional.

The complete nine-address xref band and its nine compiler-generated
initializers later proved the real model: `0x006448e8` is the 12-byte
`GruntDirectionCell g_gruntMoveDirSouth`, one of nine separately constructed
and separately aligned direction objects. The same evidence disproved a second
overlay, the fake `DirDesc g_dirDescTable[9]`, whose 0x10 indexing happened to
land on those independently aligned bases. Expressing the constructor loads as
`g_gruntMoveDirSouth.row`, `.column`, and `.direction` preserves the retail
accesses without inventing overlapping storage. Check exact-base collisions as
well as interior offsets, then expand from the collision to every related xref
and initializer before treating the first recovered type as final.
