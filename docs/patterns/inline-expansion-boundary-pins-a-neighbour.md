# The inline-expansion boundary is a general scheduling lever, not a ctor-only one

tags: cpp:inline cpp:member cpp:ctor cpp:dtor | asm:mov asm:pop | topic:codegen-idiom topic:scheduling
symptoms: a function at 97-99.6% whose whole residue is ONE adjacent transposition -
a load hoisted over a store, a `mov ecx,<this>` receiver setup hoisted over the
preceding statement's stores, or an epilogue `pop` landing one slot early; instruction
multisets, CFG and relocation sets all identical
confidence: 9/10 (4 functions to EXACT, 2 negative controls, one attribution A/B)
variants: ctor-body-first-statement-is-an-inline-member.md, declaration-count-window-steers-regalloc.md

## The generalization

[`ctor-body-first-statement-is-an-inline-member.md`](ctor-body-first-statement-is-an-inline-member.md)
established the mechanism for one shape: cl 5.0's list scheduler hoists a statement's
memory access over a *successor-less* neighbouring instruction, and an inline-expansion
boundary takes that access out of the candidate set. Nothing in the mechanism is about
constructors or about vptr stores. Measured 2026-08-16, four functions, all `walls
diagnose` REGALLOC/SCHEDULING with identical multisets:

| function | residue before | spelling that closed it | after |
|---|---|---|---|
| `CRezMgr::Close` 0x13b850 | `mov esi,[edi+0x14]` (loop-head first-storage read) hoisted over `mov [edi+0x20],ebp` (`m_pPrimaryRezFile = NULL`) | loop head reads the authentic inline `m_lstRezFiles.GetFirst()` | 99.5493 -> **100.000** |
| `CRezTyp::~CRezTyp` 0x139cf0 | `mov ecx,ebx` (the member dtor's receiver) hoisted over BOTH trailing body stores; retail puts it BETWEEN them | second store written `SetArchiveType(m_typeNode, NULL)` | 98.3099 -> **100.000** |
| `CRezDir::CRezDir` 0x139de0 | `pop esi` one slot early, before the `m_parent` store instead of after | LAST body statement written `SetArchiveDirectory(m_nameNode, this)` | 97.3333 -> **100.000** |
| `DispatchDoNothingNormalLogic` 0xa9e00 | inlined `new CDoNothingNormal(owner)`: `mov eax,[esi+0x38]` hoisted over the leaf vptr stamp | ctor body `SetObjectFlags(1)` instead of `m_wwdObject->m_flags \|= 1` | 99.6129 -> **100.000** |
| `CDDSurface::DecodeBmp` 0x143fc0 | two entry values spill and restore through the opposite registers after a palette guard | guard reads `HasPalette(pal)` instead of `pal->m_hasPalette` | 99.79 -> **100.000** |

So the rule is: **the boundary pins whichever neighbour was free to float** - a receiver
setup, a spilled-register restore, an epilogue pop, a vptr stamp. Wrap the statement that
should come SECOND and the free instruction lands just before it instead of earlier.

## Source ownership outranks minimizing the TU-state cone

The first reconstruction of `CRezMgr::Close` used an inferred list type. Adding a
header member closed the function but rotated eight unrelated current scores; replacing
it with a file-local inline helper kept `Close` exact without those rotations. That A/B
correctly demonstrated the declaration-state cone, but its old conclusion that the
boundary therefore belonged in the `.cpp` was false.

The surviving LithTech family later proved the real owner: `CVirtBaseList` plus the typed
`CBaseRezFileList::GetFirst()` header inline. Restoring that complete hierarchy, deleting
the file-local substitute, and using the authentic member kept `CRezMgr::Close` exact;
every non-EH function in the `rezfile` and `rezlist` units was exact as well. Current-score
rotations elsewhere are expected C1 movement and are not a reason to misplace a real
abstraction.

The corrected rule is: put a proven boundary in its authentic class/header owner. With no
source evidence, a narrow file-local helper remains useful as a disposable attribution
A/B, but it is not a source-model conclusion and must not displace an attested member.

`DecodeBmp` supplied the direct attribution control. A bounded exact-span run gave
99.614040% for both the direct member read and an identical helper declared but unused;
only calling the helper reached the retail instruction island at 99.780700% raw strict
score. Thus the expansion boundary, not inert declaration state, caused the schedule.
The remaining raw residue was the independently proven `s_palBmp + 0x400` one-past
referent; its exact-site retail oracle made the normalized function 100.000. The parallel
`DecodePcx` was flat across all eight eligible member-read expansions, so this remains a
per-site lever.

## A first-step dip can be the right base

* `CRezTyp::CRezTyp(i32,CRezDir*,i32,i32)` 0x139bf0: routing its FIRST body statement
  through `SetArchiveType` first took it 99.3548 -> **95.6452**. That was not a
  falsification. The surviving LithTech RezMgr source later supplied the missing authored
  statement order — `m_nType`, the typed member setter, then `m_pParentDir` — and the
  composition reached **100.000**. The old higher form was a local maximum. This is a
  direct controlled example of EXPLORATORY DESCENT: retain an authentic abstraction as a
  disposable base and compose the next independently evidenced source fact before ranking
  the path.
* `CSpotLight::Tick` 0xb1af0: replacing the two rotation-result stores with the
  surviving vector `Init` boundary first moved 84.2097 -> **81.6411**. Keeping
  that boundary and naming the two rotated-coordinate results restored
  **84.2097**. This did not close the function—the remaining wall is a separately
  diagnosed CFG mismatch—but it proves that the first helper-shaped island hid
  the caller-local lifetime that had to be composed next. Reverting at the first
  dip would have missed the source-backed combined form.
* `CAniPlayer::TickToggle` 0xe5b90: byte-identical either way (92.0000), so the boundary
  is not a universal improvement even inside a file where it closed two neighbours.

Read a drop after conversion as a new compiler state, not a verdict. Compose only
independently evidenced facts; if a bounded composition does not converge, restore the
higher humane form. A byte-flat control still says the boundary is not the lever at that
site.

## Detection signature

`gruntz walls diagnose <rva>` says REGALLOC/SCHEDULING with equal byte length, instruction,
call, branch, `ret` and relocation counts, and an address-aligned base/target dump shows
exactly one adjacent pair swapped. If the pair is (statement access, free instruction) the
boundary is worth one A/B. If the pair is two *independent* loads/stores of the same kind -
two spill reloads at a join, the two halves of a u32->double conversion - it is the C1
handle-state coin of `tu-state-probe-family-decides-reachability.md` instead, and no source
spelling reaches it.

## The expansion must CONTAIN a memory access - a store through the enclosing `this` is not one

Every closing example above moves a *read through a passed-by-reference object* inside the
expansion (`m_lstRezFiles.GetFirst()` puts the `[edi+0x14]` load there; the list's address is
already live in the caller). A helper whose whole body is a store to the object the caller
is ALREADY holding in a register adds no access to pin: cl folds it before scheduling and
emits the same bytes.

`SoundVolumeRamp::SoundVolumeRamp` 0x136fe0 is the control. Its residue is one free store - retail
emits `m_live = 1` first, cl sinks it four slots to pair with the leaf vptr stamp - and the
rest of the store sequence already matches retail exactly. BOTH boundary spellings came out
**byte-identical at 88.00**: wrapping the statement that should come second
(`SetVoiceBuffer(*this, owner)`) and wrapping the sinking statement itself
(`MarkVoiceLive(*this)`). So the lever is unavailable when the free instruction and its
neighbour are both `mov [<this-reg>+disp], <imm-or-reg>`; that shape is scheduler tie-break,
not a hoist out of a candidate set.
