# A blit family's source-rect offset is chosen PER AXIS by that axis's mirror flag

tags: cpp:struct | asm:sub | topic:mis-model
symptoms: seven near-identical Blit* variants where the two with both mirror bits set score
~99% and every other variant sits 69-84%; the diff shows only `sub` operand swaps and a
downstream regalloc cascade, so it reads as a scheduling wall
confidence: 10/10

When one function is transcribed and then copy-pasted across a family of flip variants, the
per-variant asymmetry is the first thing to lose. In `CImage::Blit*` the source sub-rect offset
is picked independently for x and y from that axis's mirror bit:

| axis mirrored | source offset |
|---|---|
| yes | `s.left = right - d.right` / `s.top = bottom - d.bottom` |
| no  | `s.left = d.left - x`     / `s.top = d.top - y`          |

`BlitNorm` mirrors both (`dwDDFX = 6`), so "both mirrored" is right there and WRONG in the five
flip variants — which is exactly why the transcription survived: the reference function it was
copied from is the one case where the shortcut holds.

The tell is the operand pair of the two `sub`s just after the `w<=0 || h<=0` guard. Identify the
registers by which `if` guarded each: the register in `test ecx,ecx; jge` before a `d.top = 0`
store is `y`, the one in `cmp ebp,eax` against `dst->m_height` is `bottom`. `d.top - y` and
`bottom - d.bottom` are both a single `sub` and the masked diff shows only a register swap.

The last two arguments of the shaded variants' `m_owned->Blit(&d, surf, &s, a, b)` are the same
per-axis flags and are an independent cross-check (`push 1; push 1` for the both-mirrored case).

STEERABLE, and worth a lot: BlitFlipV 81.8 -> 98.6, BlitFlipH 82.8 -> 98.0, BlitShadeFlipHV
69.3 -> 97.2, BlitShadeFlipV 71.7 -> 99.8, BlitShadeFlipH 84.2 -> 99.9. Before believing a
family-wide "regalloc wall", diff the ONE member that scores highest against the others and look
for what the copy dropped.
