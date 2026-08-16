# A compiler-generated datum the two sides NAME differently caps a 99.8-99.9% row

tags: cpp:static cpp:dtor cpp:const | asm:push asm:call | topic:scoring-artifact topic:wall topic:tooling
symptoms: `gruntz walls diagnose` says `class: REFERENT - masked bytes identical; the relocation TARGETS differ`, the two names are obviously the SAME object (`$anon_data_<sha>` vs `__ehreg$<fn>`, or one name with two different `$Sdata_rdata_<sha>` suffixes), and the function sits a few hundredths below 100 with nothing in the body to fix
confidence: 9/10
variants: folded-base-address-names-the-neighbour.md

`REFERENT` is normally an identity question you fix in the source. Two sub-classes
are NOT: the referent is a datum **cl generated**, and the base side and the
delinked target side name it by different rules, so no source spelling can make
them agree. Recognize and park; the code bytes already match.

## A. cl's `atexit` static-destructor thunk in its own COMDAT

The reloc site is the `push <thunk>` of the `atexit(&thunk)` that a function-local
`static T` with a destructor emits inside its guard block.

```
base:    push $0x0   ->  $anon_data_e08e34e1..._0
target:  push $0x0   ->  __ehreg$?GetRect@CButeMgr@@QAEPAUButeIntRect@@PBD0@Z
```

The thunk itself is real and correct - for a POD-with-empty-dtor it is a bare
`c3` (`sema disasm 0x173840` => 1 byte `ret`); for a class it is
`mov ecx,<obj>; jmp <dtor>`. It diverges only in NAMING: `gruntz.delink.eh_band`
carves it into the EH band as `__ehreg$<owner>`, while
`gruntz.compare.canonicalize` classifies the base's separate COMDAT as anonymous
DATA and content-hashes it. When cl instead emits the thunk as a `$L` LABEL
inside the owning function's own COMDAT, canonicalize renames it and the row
matches - which is why sibling functions of identical shape score differently
(`CButeMgr::SetRect`/`SetPoint`/`SetVector` match; `GetRect`/`GetPoint`/
`GetVector`/`GetRange`/`GetString` and `CImage::RenderFrame`/`RenderFrameClipped`
do not).

`RVA_DYNINIT` does NOT fix it. `sema rva <thunk>` reports the row `unclaimed -
structure only` with the `src_dyninit` claim LOSING, and where no census row
exists at all `verify unique-names` rejects the pin outright
(`model violation: func claim <owner> (src_dyninit) at <rva> is not an admitted
census row`).

## B. a NAMED static's canonical suffix embeds its physical EXTENT

`canonicalize.py` spells a named TU-local datum
`<name>$S<kind>_<storage>_<digest>_<n>`, and the digest covers
`{kind, storage, span, meaningful_size, payload, relocations}` where
`span = definition.end - definition.start`, i.e. **the distance to the next
DEFINED symbol**. So the same datum with the same value gets two names whenever
the two sides disagree about what follows it.

```
base:    _kMsToSeconds$Sdata_rdata_c1264432..._0
target:  _kMsToSeconds$Sdata_rdata_bdd42671..._0
```

`CFader::RunFade` / `RunFadeStepped`. The VALUE is identical - retail holds
`6f 12 83 3a` (0.001f) at RVA 0x1f07bc, and so does the base. The extents differ:
cl puts `kMsToSeconds` at the head of the TU's 0xc-byte FP pool
(`0.001f, 1.0f, 0.0f` - the last two come from `CFaderMesh::ApplyInit`, still at
82%), while the delinker carves it at the head of a 0xb0 blob that also holds
four unclaimed EH `FuncInfo` records. Same for `CButeMgr::GetFloat`'s
`s_floatErr`.

Consequence worth knowing: **every FP-pool user in a TU is coupled to every
other**, because one wrong or extra pooled constant anywhere in the TU changes
the extent of the first one. Such a row is gated on the whole TU's pool matching
retail AND on the adjacent retail bytes being claimed - not on the victim
function's body.

## Detection

`walls diagnose <rva>` prints the pair. Confirm sub-class A by the `push` site
sitting between a guard-byte test and `call atexit`; confirm B by the two names
being equal up to the `$S…` suffix. Read the retail bytes directly before
believing a value differs - a same-name/different-digest pair is an extent
question, never a value question.
