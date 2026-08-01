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
