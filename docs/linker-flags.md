# Candidate linking

The active command and defaults live in
[scripts/gruntz/graph/link.py](../scripts/gruntz/graph/link.py).
Run `gruntz link --help` inside `nix develop` for overrides.

The candidate uses the Windows subsystem, `WinMainCRTStartup`, a map file,
an explicit image base, and `/FIXED:NO`. Incremental linking is the default;
`--no-incremental` selects a flat link. The keep-all mode adds
`/OPT:NOREF /OPT:NOICF`. Use the generated response file to inspect the exact
object order, libraries, and options for a particular build.

Compile matching and final-image matching are different checks. A function's
normalized COFF match does not establish final RVA placement, import binding,
or startup correctness. See [image comparison](image-diff.md).

Do not infer original source ownership from proximity alone, fabricate padding
to force addresses, or equate identical COMDAT selection with arbitrary
identical-code folding. Retail instructions, relocations, and independently
identified contributions constrain those decisions.

The [old linker investigation](https://github.com/sushi-shi/gruntz-decomp/blob/b27b05deb249e4cacbb29f55f17b469ecfe56f26/docs/linker-flags.md)
is historical evidence, not the live command specification.
