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
| createstreambuffer2.json | SoundStream::CreateStreamBuffer 0x137780 | 6 | 66.37 -> **66.82** from the STORE ORDER alone (dwFlags, lpwfxFormat, dwSize, dwBufferBytes). NEGATIVE on the `wf.cbSize = 0` half: all four placements cost points here (64.82 / 65.26 / 65.26 / 66.80) though its twin gains +3.63 from the same line - an unfound placement, not an absent store |
| createbuffer-cbsize.json | SoundDevice::CreateBuffer 0x1366f0 | 6 | 66.73 -> **70.36**: `wf.cbSize = 0` AFTER `&wf` escapes into desc.lpwfxFormat. Adjacent to the copy cl DSEs the pair. The SAME six cells give a different winner on its near-twin CreateStreamBuffer - see same-sites-different-per-function-optimum.md |
| updatediagonals.json | CMapMgr::UpdateDiagonals 0x82030 | 24 | 90.02 -> **90.17** with `up_last` (declare down/right/left/up, then the diagonals). The counter_decl half is fully INERT - all five `r`/`c` declaration spellings tie |
| updatediagonals2.json | CMapMgr::UpdateDiagonals 0x82030 | 6 | NEGATIVE - six more orthogonal-neighbour declaration orders, `up_last` (the committed one) is the product optimum. The residual 8 bytes are register COLOURING (`cell` in ebp vs retail's ebx), not declaration order |
| pathhazard-ctor.json | CPathHazard::CPathHazard 0xb35a0 | 12 | 96.23 -> **96.86**: both m_screenX/Y stores must precede both float converts. NEGATIVE on the sortkey_flags half - cl canonicalises `|=`, an explicit temp and a self-assign to the same `or [mem],imm` RMW, so retail's 3-instruction load/or/store form is not spellable |
| readplaneobjects-scatter.json | CDDrawWorkerHost::ReadPlaneObjects 0x162af0 | 8 | 85.61 -> **86.01**: hoisting the dynamic-flags dword into its own temp before the `|=` re-phases the eax/ecx/edx rotation across the whole 60-store field scatter. NEGATIVE on the rect-fixup operand order |
| sbi-imagesetani-init.json | CSBI_ImageSetAni::Init 0xe7980 | 15 | 69.34 -> **81.70**: EXPLICIT `if/else` arms beat the ternary for the -1 frame-window assignments. The ternary makes cl batch b2/b3/b4 into three live registers at once, which needs a fourth and forces a `push edi` retail does not have. The record-guard axis is NEGATIVE (all 3 spellings tie) |
| sbi-imageset-setupimage.json | CSBI_ImageSet::SetupImage 0xe72f0 | 36 | NEGATIVE - all 36 cells tie at 68.23 (guard form x Lookup receiver x frame-resolve spelling). The residual is that cl hoists `mov eax,[esp+0x8]` above `push esi`, so the load happens while ecx still holds `this` and host is forced into EAX; retail vacates ecx first (`mov esi,ecx`) and takes ecx for host. Every remaining diff row, including the key guard's `xor`-less epilogue, follows from that one swap |
| sbi-imagesetani-render.json | CSBI_ImageSetAni::Render 0xe7b00 | 27 | NEGATIVE - all 27 tie at 91.36 (cel-fetch spelling x anchor-add operand order x explicit register decrement). Base is 210 B against retail's 225: cl merges two wrap exits retail keeps apart, one of which uses a register decrement and the other `dec [esi+0x28]`. Branch sequences already AGREE on mnemonics AND symbolic targets |
| scrollto.json | CWwdSpatialMgr::ScrollTo 0x168340 | 15 | NEGATIVE - all 15 tie at 93.85 (size 225 EXACT, relocs 3/3). Residual is purely WHERE cl schedules the two `m_scroll{X,Y}` member stores: retail emits them at the jne target, ours sinks them 8 insns into the first Query arg block. Guard polarity, rect-decl position, store order and the three sum associations are all identical |
| refresh-menubar.json | CActionOptionsMenuBar::Refresh 0x9330 | 24 | NEGATIVE - all 24 tie at 93.24 (size 310 EXACT, relocs 2/2). Residual is which register holds g_gameReg and the resulting index/base swap in `[eax+ecx*4+0x1c]`; hoisting the board, the cell index, the registry pointer, swapping the index terms, and two store spellings all score identically |
