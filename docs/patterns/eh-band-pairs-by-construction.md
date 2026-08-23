# The EH band pairs BY CONSTRUCTION, and most of it is its parent's frame

tags: cpp:eh cpp:dtor cpp:temp msvc5:mfc | asm:jmp asm:lea | topic:tooling
topic:scoring-artifact topic:wall
symptoms: 222 `__ehunwind$`/`__ehreg$` rows in the sub-100 queue that no sieve
reads; a tool reporting them "unpairable"; funclets at 0.00 or 17.50 next to a
parent that looks fine
confidence: 9/10 (whole sub-100 EH band, 42 parents, 2026-08-23)
variants: eh-unwind-map-is-a-c1-fingerprint.md,
identical-derived-dtor-comdat-is-named-by-link-order.md,
repeated-container-call-is-an-inline-member.md

## They are not unpairable

`walls thisscan` used to skip 222 rows with the note "cl 5.0 emits them inside
the parent's COMDAT with no symbol of their own, so there is nothing to pair
them against." **Both halves are wrong.**

* cl 5.0 gives the unwind funclets **their own `.text` COMDAT**, separate from
  the parent's.
* They **do** carry symbols there - cl's local labels, `$L42015`, `$L42016`, …
  at an 11-byte stride.
* The normalizer's `build/objdiff/compare-new/<side>/<unit>.symbols.tsv`
  records, for each of them, the canonical `__ehunwind$<parent>$<n>` name plus
  its `(section_ordinal, section_offset)`.

So the base side pairs against the target side by NAME, through the canonical
map. One trap: `Obj.section_members` does not list `$L<n>` labels, so cutting a
funclet at "the next COFF member" runs the slice to the end of the whole unwind
COMDAT. Union the canonical offsets into the boundary set.

    gruntz walls ehactions --census        the whole band, grouped by PARENT

## Group by PARENT, never by funclet index

`$<n>` is per-side ADDRESS order. The moment one side has a funclet the other
lacks, every later index shifts and index-wise comparison reports garbage. The
parent is the unit of work, and the thing to compare is the ordered list of
`(object slot -> destructor)` ACTIONS.

## The census: 222 rows are 42 questions, of which 14 are work

| verdict | parents | rows | what it is |
|---|---:|---:|---|
| `slot-shift` | 28 | 155 | the SAME destructors in the SAME order, at a different frame displacement |
| `count` | 9 | 56 | the funclet count differs |
| `dtor-identity` | 5 | 11 | the ordered destructor list differs |

**`slot-shift` is not work of its own.** `CPlay::ValidateLevelTiles` has 22
funclets on each side, all `-> ??1CString@@QAE@XZ`, ours at `[ebp-0x3c]` and
retail's at `[ebp-0x30]`. That is the parent's frame layout showing up a second
time; the 22 rows close exactly when the parent closes and never before. Do not
open them as separate tickets - 70% of the band is this.

**`count` is the ctor/dtor inline boundary**, already documented in
[repeated-container-call-is-an-inline-member](repeated-container-call-is-an-inline-member.md):
a funclet exists per live cleanup state, so inlining one more constructor can
remove a temporary's state. This is the actionable bucket, and the funclets at
0.00 count the missing sites for free.

**`dtor-identity` is the only structural bucket, and four of its five parents
are a DESTRUCTOR-NAMING question, not a cleanup one.** Ours says
`??1CGruntCoordList@@UAE@XZ` where retail says `??1CPtrList@@UAE@XZ` at
`CGrunt+0x31c` (three funclets: the CGrunt ctor, the CGrunt dtor and
`SerialObjectFactory`), and ours says `??1CTileImageSet@@UAE@XZ` where retail
says `??1CObject@@UAE@XZ` in `~CImageSet3`. In both, the object SLOT and the
funclet's position in the sequence are identical - only the referent's name
differs, which is the signature of
[identical-derived-dtor-comdat-is-named-by-link-order](identical-derived-dtor-comdat-is-named-by-link-order.md):
an EMPTY derived destructor is byte-identical to its base's, because cl deletes
the most-derived vptr store when the inlined base dtor overwrites `[this]`
before any call observes it, so the retail link legitimately holds one copy
under whichever name came first.

Do not "fix" the class to match the funclet's referent - the unwind action is
the wrong instrument for that question, since it can only report the name the
LINK kept. Whether `CGruntCoordList : public CPtrList` (which adds one
non-virtual method, `NextData` at 0x029a30, and no members or virtuals) belongs
in the original source is an open modelling question, and it must be decided on
the vftable and COMDAT evidence, not here.

The fifth, `CGruntzMgr::ChangeState`, is the negative control documented in
`walls ehactions` itself: both sides have eleven funclets and retail's differ
because it CALLS the `CArray` ctor/dtor our side expands.

## What this costs, stated plainly

A third of the sub-100 queue was invisible. Made visible, it holds **zero new
independent defects**: 155 rows are their parent's frame, 56 are an inline
boundary the campaign already works, 11 are naming artifacts and one known
control. The value of pairing them is not new bugs - it is deleting a third of
the queue from the worklist and knowing why.

## Reverse use

A funclet score is a READOUT, never a target. If a parent's funclets are all
`slot-shift`, its unwind model is already right and the row is telling you
about the frame - `--shift` says WHICH frame question, and the answer is
usually not the frame's SIZE
(eh-slot-shift-measures-the-parents-local-homing.md). If they are `count`,
look for a missing out-of-line constructor call. Only `dtor-identity` with a
destructor neither side can explain is a cleanup question - and check the
byte-identical-dtor artifact first.
