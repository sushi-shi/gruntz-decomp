# Compiler-generated referents need owner and content proof, not an ordinal pin

tags: cpp:static cpp:dtor cpp:const | asm:push asm:call | topic:scoring-artifact topic:wall topic:tooling
symptoms: `gruntz walls diagnose` says `REFERENT` with masked bytes identical, and a `push <callback>` before `call atexit` names a base `$anon_data_<sha>` but a target `FUN_...` or `__ehreg$...`
confidence: 9/10
variants: folded-base-address-names-the-neighbour.md

`REFERENT` remains an identity question even for compiler-generated code. A
base/target name difference alone does not prove a wall. First recover the
emitting owner and the callback's real body, then let normal relocation/content
comparison decide whether they match. The earlier advice to park these rows was
falsified by the controlled callback attribution below.

## A. cl's `atexit` static-destructor thunk in its own COMDAT

The reloc site is the `push <thunk>` of the `atexit(&thunk)` that a function-local
`static T` with a destructor emits inside its guard block.

```
base:    push $0x0   ->  $anon_data_e08e34e1..._0
target:  push $0x0   ->  __ehreg$?GetRect@CButeMgr@@QAEPAUButeIntRect@@PBD0@Z
```

The thunk is real code: either `c3` or `mov ecx,<object>; jmp <destructor>`.
`RVA_DYNINIT` at the owning local static gives a semantic source/retail owner
without pinning cl's volatile `_$E<n>` ordinal. The delinker now derives that
ordinal from the current base COFF only when its named owner has exactly one
`_atexit` call whose pushed argument relocates to a defined executable `_$E<n>`
callback. The corresponding named retail owner must have one call to `atexit`
(possibly through an ILT thunk); the pushed pointer must have a real HIGHLOW
relocation to a same-unit `src_dyninit` pin and a valid callback body. Only then
is the transient name provisioned in the synthetic PDB, *before* EH-band and
static-library labels. Ordinary strict normalization still compares callback
payload and ordered relocations; no byte equivalence is asserted by the pin.

Controlled A/B: without attribution, `CButeMgr::GetString/GetRect/GetPoint/
GetVector/GetRange` and `CImage::RenderFrame/RenderFrameClipped` each had
byte-identical caller instructions but one mismatched callback referent.
Provisioning the ten proved callback names made all seven callers **100% exact**;
each now has identical raw bytes and identical ordered relocations. A full-path
control checks the derived names reach `function_records`; removing the retail
relocation sites suppresses every attribution. Keep the `RVA_DYNINIT` owner pin,
not a numeric ordinal or an invented source function.

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

`walls diagnose <rva>` prints the pair. Confirm A by the guard, callback push,
`atexit` call, COFF callback definition and retail HIGHLOW site; a missing pin,
different owner, second `atexit` call or unrecognized callback must remain
unattributed. Confirm B by the two names being equal up to the `$S…` suffix.
Read the retail bytes directly before believing a value differs: a
same-name/different-digest pair is an extent question, not necessarily a value
question.
