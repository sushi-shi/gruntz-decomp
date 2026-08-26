# Matching lessons from other decompilation projects

This audit covers project documentation, source comments, and commit history in
FUELDecompilation, th06, isledecomp/isle, and OpenBarnyard. The conclusions are
filtered through Gruntz's stricter rule: byte identity, relocations, and correct
source structure remain authoritative. A trick that only raises a metric is a
probe, not source to retain.

## Highest-value additions for Gruntz

### Candidate statement locations from `/Z7`

The local HoMM3 implementation already provides the right first layer. It
compiles a cached parallel object with `/Z7`, parses classic COFF
`IMAGE_LINENUMBER` records, and refuses the map unless the selected function's
debug-build bytes equal the normal matching object's bytes (allowing only up to
15 trailing alignment NOPs). `homm3 sema diff <rva> --source` then annotates the
block-aligned candidate listing with the first divergent candidate statement.

This is candidate-source metadata, never recovered retail debug information.
It does not affect match verdicts. Also, `homm3 sema disasm --source` is
intentionally incompatible with its public `--blocks` option; source-aware
*diff* performs block alignment internally. The Gruntz port should reproduce
that contract rather than exposing an unsafe option combination.

ISLE's [stack-layout comparator](https://github.com/isledecomp/isle/commit/8446a7ffa1983e053507876a9c61706a1abbe190)
is the complementary second layer: read locals and types from the candidate
PDB, align original and recompiled memory operands, and require a bijective
stack-offset mapping. Together they answer two different questions:

1. Which candidate statement emitted the first divergent instructions?
2. Which candidate local/type owns each stack slot, and is the stack model
   structurally consistent?

Port the HoMM3 statement layer first, unchanged in principle, then add ISLE's
local/type view. Never treat either as evidence of original names.

### Make claims self-describing in COFF

FUEL's [COFF-comment control records](https://github.com/widberg/FUELDecompilation/commit/96b344df1131ad2064177c8c85e42dbfdf2eb8c3)
carry delink annotations plus source file/line into the compiled object and
check collisions while reading them. Gruntz already scrapes `RVA`/`DATA`
annotations from source; emitting a compact secondary record would prove that
the compiled object and scraped claim came from the same spelling and prevent
stale source-to-object association. It should be an integrity oracle, not a new
identity authority.

FUEL also [separates its objdiff object from the production link object](https://github.com/widberg/FUELDecompilation/commit/6588dda65428f0a200c602c5eed3f26d859ee075).
That is a useful general rule when symbol surgery needed for linking would
otherwise pollute comparison evidence.

### Add diagnostic-only effective-match classes

ISLE recognizes [compatible compare/branch inversions, register renames, and
limited instruction relocation](https://github.com/isledecomp/isle/commit/c8840117be3c127d293217c7560949ea35a32667).
These should become wall classifiers or diff annotations in Gruntz, not a
weaker success gate. They can name the remaining compiler decision while exact
bytes and strict referents continue to define 100%.

### Cross-title evidence matrices

OpenBarnyard is not a strict historical byte-match project, but its strongest
method is cross-game triangulation. For example, it [recovered
`TSkeletonSequence` from de Blob](https://github.com/InfiniteC0re/OpenBarnyard/commit/1a43621c28c3497941f0f91de1d79283582f90d5),
changing array ownership, names, layout, and comparison interfaces together.
Other fixes cover [virtual order](https://github.com/InfiniteC0re/OpenBarnyard/commit/8e74f88127d709299ee08ff188428490f3186d88),
[object layout](https://github.com/InfiniteC0re/OpenBarnyard/commit/496a13d15d892a6d73c2f9ac99453bd62a7b1adf),
[boolean polarity](https://github.com/InfiniteC0re/OpenBarnyard/commit/3b62d29392a00b1abc6154d44890af549908f7c2),
and [vendor-wrapper ownership](https://github.com/InfiniteC0re/OpenBarnyard/commit/6c1599f6fa772799fadaa614fdc4e586baa8c78f).

HoMM2, HoMM3, Gruntz, Vostok, Claw, and later shared-engine games should keep a
machine-readable matrix of class size, field offsets, vtable slots, mangled
signatures, constants, and literal names by title/version. A sibling proves a
source prior or semantic name; it does not prove that an older branch retained
the same layout.

## Source-shape evidence from th06

The th06 history is especially useful because it records small source changes
that alter old-MSVC output:

- local declaration/stack order is observable; see
  [`FileSystem::OpenPath`](https://github.com/GensokyoClub/th06/commit/6421bfa3fc1bfbff68214689db4fc28e8a968b97)
  and [`CStreamingSound`](https://github.com/GensokyoClub/th06/commit/1fe6d368c1579ff3fe792b239bfb8a3e363bbab0);
- blank `if`/`else if` branches followed by a shared return can reproduce a CFG
  that gotos or short-circuit rewrites do not; see the
  [Supervisor change](https://github.com/GensokyoClub/th06/commit/bfe60b709b04938e8e1face236b54247b6ca4d6d);
- floating constants' source order can matter independently of their values;
  see [`ResultScreen`](https://github.com/GensokyoClub/th06/commit/28d0ad8b39b3939dcd1822bc7cd6fbac9d925567);
- restoring a real in-header inline constructor eliminated fake stack-space
  calls, a strong example of correct ownership solving apparent stack residue;
  see [`BombData`](https://github.com/GensokyoClub/th06/commit/2db3fde2102f2aff2b53f5b38a6cfdbf0746890e).

There are equally important anti-lessons. th06 temporarily used `/ORDER` when
inlining disturbed placement ([commit](https://github.com/GensokyoClub/th06/commit/7a5b1e3d766f521302fe292364fa623b57adb4f9));
Gruntz must keep link-layout diagnosis separate from source proof. It also
removed an intrinsic pragma that existed for matching
([commit](https://github.com/GensokyoClub/th06/commit/87c265f20be5ee15278da70fab8e5922ba02f828))
and contains accuracy-preserving codegen transcriptions such as split copies
([commit](https://github.com/GensokyoClub/th06/commit/566edf8f836fd25f55ef440330624de76bff49ed)).
Those are useful A/B spellings but fail Gruntz's no-sane-developer test unless
other evidence proves them.

## ABI and class-model evidence

FUEL's patched MSVC8 accepts custom `__usercall`/`__userpurge`
([commit](https://github.com/widberg/FUELDecompilation/commit/0924bff3ddd9d0c514c782f2398c674fcbbc406f)).
The lesson is to preserve explicit ABI uncertainty and instrument ambiguous
call sites—FUEL also added [runtime usercall instrumentation](https://github.com/widberg/FUELDecompilation/commit/70c5aed08f2cdf81f7ffa86963adce5e9fa9f9e1)—but
not to patch VC5 until Gruntz has evidence of a convention the compiler cannot
express. FUEL's source comments such as “verify usercall varargs” are valuable
because they avoid promoting an unresolved ABI guess into a fact.

ISLE's virtual-inheritance work explicitly handles [vtable thunks](https://github.com/isledecomp/isle/commit/3f03940fcb23602353180ca5d7ec8cbece537c7d)
and [vtordisp](https://github.com/isledecomp/isle/commit/9383076e04703aa5a8ed9c76d722afc3a904b98f).
If those signatures appear in Gruntz, its vtable census should classify them
rather than manufacture dummy virtuals. ISLE also has dedicated handling for
[static locals](https://github.com/isledecomp/isle/commit/264b9e815be0e3c78aac9c9f23d9f737363dc2d9)
and [enforced vtable correspondence](https://github.com/isledecomp/isle/commit/b5a3c5feea0a7ab2c7d42c32dc985e9485851975).

Several ISLE source comments are reusable reverse-engineering clues:

- returning a UDT by value rather than reference can force MSVC's hidden
  [`$ReturnUdt$` stack slot](https://github.com/isledecomp/isle/blob/31bd20de79df0a2d2d26b63f734e155ddd17e8ae/LEGO1/omni/src/common/mxstring.cpp#L146);
- a [union containing a struct](https://github.com/isledecomp/isle/blob/31bd20de79df0a2d2d26b63f734e155ddd17e8ae/LEGO1/omni/include/mxtypes.h#L62)
  can be required even when a flattened layout has the same size;
- an [outer call may inline while recursion targets the real body](https://github.com/isledecomp/isle/blob/31bd20de79df0a2d2d26b63f734e155ddd17e8ae/LEGO1/lego/legoomni/src/paths/legopathactor.cpp#L599);
- a [Beta implementation can omit a local that matches retail](https://github.com/isledecomp/isle/blob/31bd20de79df0a2d2d26b63f734e155ddd17e8ae/LEGO1/omni/src/stream/mxstreamer.cpp#L232),
  and an [apparent Beta inline may not inline in retail](https://github.com/isledecomp/isle/blob/31bd20de79df0a2d2d26b63f734e155ddd17e8ae/LEGO1/lego/legoomni/src/main/legomain.cpp#L639),
  so a second-version source oracle never overrules the target binary.

The comments also expose anti-patterns such as an
[`unused` local “required for match”](https://github.com/isledecomp/isle/blob/31bd20de79df0a2d2d26b63f734e155ddd17e8ae/LEGO1/lego/legoomni/src/video/legoanimpresenter.cpp#L1076)
or a [cast retained for unknown matching reasons](https://github.com/isledecomp/isle/blob/31bd20de79df0a2d2d26b63f734e155ddd17e8ae/LEGO1/lego/legoomni/src/actors/helicopter.cpp#L459).
They are search clues under Gruntz's rules, never final justification.

## Recommended implementation order

1. Keep the completed 1.01 migration as the address authority; never feed a
   source-line map claims from the retired 1.00 census.
2. Port HoMM3's byte-verified `/Z7` statement map to Gruntz's aligned diff.
3. Add the PDB local/type stack map and bijection checks from ISLE.
4. Add effective-match diagnostics without changing the 100% gate.
5. Prototype self-describing COFF claim records and prove stale-object negative
   controls before adopting them.
6. Build the cross-title class/ABI matrix, starting with WAP32/Monolith names
   and any New World Computing infrastructure shared by HoMM2/HoMM3.
