# A cross-game retail binary can prove a shared source family without revealing its spelling

tags: asm:binary asm:pe cpp:member | topic:source-oracle topic:evidence-discipline topic:regalloc
symptoms: a complete function in a sibling game has the target's exact instruction bytes except for independently explained call displacements and class-member offsets, while the reconstruction has the same calls, CFG, and semantic multisets but a whole-function register rotation
confidence: 10/10

## Exact witness

The original 1997 Claw demo embeds the source-path anchor
`C:\Proj\DDrawMgr\DIRSURF.CPP`. Its `CDDSurface::Blit824` implementation is at
VA `0x4ae0d0`; Gruntz retail's implementation is at VA `0x540110`. Both are
`0x30b` bytes and 265 instructions with the same two calls, 22 branches, three
returns, and complete control-flow topology.

A byte comparison over the whole `0x30b`-byte extent finds only 11 differing
bytes:

- two bytes inside the relative displacement of the direct `Lock(NULL)` call;
- nine one-byte member displacements, all explained by Claw placing height,
  width, and pitch four bytes earlier in its `CDDSurface` layout.

The other 768 bytes are identical, including the prologue that loads `pal` into
EBX before saving EBP, binds `this` to ESI, the two independently allocated
row-order arms, the complete 256-entry squared-RGB search, and the shared
virtual `Unlock(NULL)` tail. The Claw PE also reports Microsoft linker 5.0.

This is stronger than a similar-looking decompilation: it proves that the
Gruntz target belongs to the shared DDrawMgr compiled-source family and that
the target register allocation was attainable from that family. It also rules
out different pixel logic as the explanation for Gruntz's current
`pal`/`this` rotation.

## What it does not prove

Binary identity cannot choose among source spellings that VC5 compiles to the
same bytes. On the reconstructed body, all of these are one identical compiler
island at 71.061775%:

- split destination store/increment versus `*dst++`;
- direct duplicated row loops versus a complete-row macro;
- the row macro composed with a search macro;
- direct and row-macro callers of a `static inline FindNearestColor` helper.

The inline helper also leaves `Blit816` byte-identical at 92.525925%. The source
lineage therefore supports the common implementation family, while the normal
inline/macro and source-quality priors decide between byte-flat spellings. It
does not turn a binary into missing source text.

Likewise, a sibling binary's absence of a call does not disprove an inline
helper. It proves only that no call boundary survived. Search the sibling image
for an out-of-line copy or COMDAT before making a stronger claim.

## Controlled structural and state result

After restoring the shared `SQR` expression family, Gruntz's current body is
`0x309` bytes and 265 instructions against retail's `0x30b` and 265. Calls,
branches, returns, ordered relocations, mnemonics, immediates, displacements,
stores, and referents agree; the first difference is the opening allocation
tuple (`pal=ESI`, `this=EDI` versus retail/Claw `pal=EBX`, `this=ESI`).

A source-hash-scoped 128-trial target-adjacent C1 campaign executed 117 valid
forest, typedef, enum, struct, class, prototype, function, mixed, and
typedef-count probes. Every valid trial emitted the baseline object. The row
and search ownership compositions above were independently byte-flat. This
bounds the known local abstraction and parser-state families without weakening
the positive shared-lineage conclusion.

## Reverse-use rule

Use a sibling retail executable as a positive source-family oracle only when:

1. a source-path, library name, or independently mapped neighbouring family
   identifies the same owner;
2. the complete function extent and decoded CFG agree;
3. every differing byte is accounted for by a site-specific relocation,
   layout, or other independently proved revision fact;
4. the remaining instruction bytes agree over the whole function, not merely a
   short signature.

Then use the sibling to select the implementation family, parameter/local
widths, control flow, and compiler texture. Do not infer source spelling from a
byte-flat choice, import negative absence, or replace the Gruntz retail image as
the authority. A cross-game exact function can prove that a wall is downstream
compiler state; it cannot by itself supply the missing lever.

The Claw demo was extracted as data and never executed. The installer is
available from the [Claw demo archive](http://www.classicdosgames.com/game/Claw.html);
the InstallShield v3 payload was decoded with
[unshieldv3](https://github.com/wfr/unshieldv3).

## Related

- [`surviving-source-lineage-restores-typed-layers-and-order.md`](surviving-source-lineage-restores-typed-layers-and-order.md)
- [`debug-codeview-objects-recover-authored-source-shape.md`](debug-codeview-objects-recover-authored-source-shape.md)
- [`integer-square-macro-preserves-expression-origin.md`](integer-square-macro-preserves-expression-origin.md)
