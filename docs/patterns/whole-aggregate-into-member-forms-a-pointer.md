# A whole-aggregate assignment into a member forms a pointer; field-wise writing does not

## Symptom

A function's residue is a run of four stores into one member aggregate, and the
two sides disagree only about how the destination is addressed:

```
retail   add  edi,0x14                     ours   mov  DWORD PTR [eax+0x18],ecx
         mov  DWORD PTR [edi],eax                 mov  DWORD PTR [eax+0x14],ebp
         mov  DWORD PTR [edi+0x4],ebp             mov  DWORD PTR [eax+0x20],edi
         mov  DWORD PTR [edi+0x8],ecx             mov  DWORD PTR [eax+0x1c],ecx
         mov  DWORD PTR [edi+0xc],ebx
```

Same instruction count, same values, same member. Retail forms a pointer to the
member ONCE with `add reg,<member offset>` and then stores at zero-based
displacements `+0/+4/+8/+0xc`; ours stores at `[base + off]`, `[base + off+4]`,
... and never forms the pointer.

## Mechanism

`p->m_rect = rc;` is an operation on the OBJECT: cl 5.0 takes the destination's
address, then copies field by field through it. `p->m_rect.left = ...;` four
times are four independent member stores, each addressed from `p`.

The pointer formation is therefore a direct readout of the source spelling, and
unlike a frame-size or spill argument it does not depend on register pressure.
Two secondary consequences follow and are visible in the same window:

* **Store order becomes the STRUCT's field order**, not the source's. A whole
  assignment emits `left, top, right, bottom` regardless of the order the
  fields were computed in; field-wise assignment lets the scheduler reorder the
  stores freely (ours above emits `top, left, bottom, right`).
* **The struct temp can cost a spill** if the values feeding it are not computed
  in an order that lets cl keep them all in registers. That is a SEPARATE
  problem to fix on top, not evidence against the spelling - see below.

## Evidence

`CStatusBarMgr::UpdateChipGrinderStatusBar` 0x1076a0, **87.81 -> 100.00 EXACT**.
Retail's `add edi,0x14` / `[edi+0/4/8/0xc]` against ours at `[eax+0x14..0x20]`
was the whole lead. Applying the spelling alone **DIPPED to 83.15** - the new
local made cl spill the top edge (`mov [esp+0x18],ecx` and a reload) - but the
addressing shape had arrived, so it was the right BASE. Two further levers on
top of the dip closed it: computing the two y-terms adjacently so cl consumes
the shared `m_rect10.top` twice with `lea` (83.15 -> 93.64), and `rc.bottom`
before `rc.top` to match retail's EBX/EBP binding (93.64 -> 97.36). A fourth,
unrelated lever (`m_fallRect.top += speed`) took it to 100.00.

Positive control carrying the property: the sibling
`CStatusBarMgr::UpdateFallingItemStatusBar` 0x107590, whose source ALREADY
writes `n->m_rect14 = rc;`, shows the identical `add ebp,0x14` /
`[ebp+0/4/8/0xc]` in retail and `add ecx,0x14` in ours - the same offset,
different register, which is why the sieve below keys on the OFFSET and never
on the register name.

## Detection

Find `add <reg>,<imm>` followed, within a short window and **before `<reg>` is
redefined**, by at least three of the four stores `[reg+0]`, `[reg+4]`,
`[reg+8]`, `[reg+0xc]`. Compare the multiset of immediates between the two
sides; retail-only immediates are candidates.

The redefinition guard is the whole false-positive population. Without it,
`CGrunt::LoadGruntCombatAnimations` 0x597a0 fires: an arithmetic `add eax,0x20`
on a coordinate is followed three instructions later by
`lea eax,[esi+0x43c]`, and the three stores that then go through EAX get
credited to the arithmetic `add`.

## Census

Run over all 595 rows of the sub-100 queue, with the guard: **0 rows** where
retail has more of these sites than the base. The one candidate without the
guard is the false positive above. So this shape is not a live lead anywhere
else in the current queue - `UpdateChipGrinderStatusBar` was its only instance
and it is now exact. The value of the entry is the READING, for the next
function whose residue is four member stores.

## Related

* [[whole-object-assignment-pins-an-aggregate-to-the-frame]] - the same
  spelling seen from the LOCAL side: one whole-object assignment stops cl
  scalar-replacing a small aggregate. That entry's lever is about where the
  aggregate lives; this one is about how its destination is addressed, which is
  readable even when the aggregate never reaches the frame at all.
* [[whole-struct-copy-vs-scalars]] - warns that the frame is not the arbiter.
  Consistent: the pointer formation here is an addressing fact, not a frame
  fact, and it survived a spelling that changed the frame.
* [[plain-rect-vs-crect-assignment]] - `CRect` assignment goes through imported
  `CopyRect`; everything above is plain `RECT`.
