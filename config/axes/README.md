# Hand-authored source-shape axes manifests

Each file is the exact Cartesian matrix that was run for one function, so the result is
reproducible and the *negative* results stay auditable (the manifest records which
spellings were ruled out, which a commit message cannot).

Run one with:

    python -m gruntz.permute.batch_source_variants config/axes/<fn>.json --top 10 --output /tmp/<fn>-out

Use `batch_source_variants` directly, not `match_variants --axes-from`, when the axes are
hand-authored: the latter also emits libclang AST mutations, which can overlap a
hand-authored axis and abort the run.

`find` spans must be globally unique in the file. Where a file holds near-identical
sibling functions there may be no unique line in the region of interest - anchor the axis
on the function signature and fold that region's sub-site product into that axis's
options (see measurewrapped.json / layoutwrapped.json).

See docs/patterns/same-sites-different-per-function-optimum.md.

| manifest | fn | cells | outcome |
|---|---|---|---|
| allocatememory.json | Font::AllocateMemory 0x179720 | 12 | 92.1 -> **100 EXACT** (whole-Glyph struct copy) |
| measuretext.json | FontRenderer::MeasureText 0x17ac50 | 48 | 71.8 -> 72.7 (two-site interaction) |
| measurewrapped.json | FontRenderer::MeasureWrapped 0x17ad10 | 192 | confirmed the committed spelling is the product optimum |
| drawwrapped.json | FontRenderer::DrawWrapped 0x17a460 | 96 | 74.2 -> 75.2, optimum OPPOSITE to MeasureWrapped's |
| layoutwrapped.json | FontRenderer::LayoutWrapped 0x17b120 | 48 | 86.5 -> 86.7, a third distinct optimum |
| parsewave.json | SoundStream::ParseWave 0x137b70 | 72 | NEGATIVE - every cell identical; frame packer, not declaration sites |
| buildleveltitlestring.json | BuildLevelTitleString 0xe44e0 | 24 | NEGATIVE - see the doc; the EH-object slot order is not declaration-steerable |
| getxmidivolume.json | CGruntzSoundZ::GetXMidiVolume 0x1389c0 | 8 | NEGATIVE - all 7 divide-expression spellings tie at 91.30; the 8th (nested clamps) craters to 64.44. The residual is which register cl's reciprocal-divide expander keeps the quotient in |
| removematching.json | DSoundList::RemoveMatching 0x136f60 | 6 | 99.30 -> **99.90**: `Unlink(e ? &e->m_link : 0)` beats `Unlink(e ? node : 0)`; re-forming the address AT the call lands the neg/sbb/and null-mask in eax (retail) instead of edx |
| dsoundvoicetick.json | DSoundVoice::Tick 0x137060 | 6 | NEGATIVE - all 6 tie at 95.70. The residual is which of `this`/`now` gets esi; hoisting m_rampDurationMs / m_rampStartTime to locals, negating the LHS and splitting `done`'s declaration all score identically |
| measuretext-glyph.json | FontRenderer::MeasureText 0x17ac50 | 16 | 72.52 -> **74.06**: `Glyph g;` at FUNCTION scope (above the null-font branch) with ONLY `g.height` initialised. Needs both halves; zeroing both fields is worse. The inherited measuretext.json had only tested a hoist to just above the loop and an in-loop init |
| drawglyphrun-glyph.json | FontRenderer::DrawGlyphRun 0x179e70 | 32 | NEGATIVE - Glyph scope/init/copy across all three sites, spread 65.70-65.92 on a 1516-byte function. Frame/allocation, not the sites |
| freesamples.json | SoundDevice::FreeSamples 0x136ed0 | 4 | NEGATIVE - baseline early-return is BEST (77.31); the positive form drops to 72.13 and a result variable to 71.39, REFUTING positive-gate-enables-shrink-wrap.md for this function |
| getitem.json | DSoundCloneInst::GetItem 0x135d70 | 4 | NEGATIVE - all 4 guard spellings tie at 90.31 |
| createstreambuffer.json | SoundStream::CreateStreamBuffer 0x137780 | 7 | 65.27 -> **66.37** with `memset(&desc,0,sizeof)`; retail zeroes all five DSBUFFERDESC dwords then fills four. `wf.cbSize = 0` placed ADJACENT to the copy regresses (cl DSEs the pair) - retail's zero lands after `&wf` escapes into the desc, still UNTESTED |
| createstreambuffer2.json | SoundStream::CreateStreamBuffer 0x137780 | 6 | NOT RUN - the cbSize-after-escape follow-up; authored, ready |
