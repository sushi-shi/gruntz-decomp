# `src/Stub/types/` — comprehension type headers

This directory holds the **comprehension-layer** type headers (structs/enums) that
feed the struct generator. `scripts/gruntz/build/ghidra_metadata_generate.py` reads
`src/**/*.h` (matched, authoritative) PLUS each `.h` here (wrapped in a one-line
`.cpp` and laid out with clang `-fdump-record-layouts`); **`src/` wins on any name
overlap**. As a class is byte-matched and authoritatively defined in `src/`, its
header here becomes redundant and is deleted — the layer shrinks toward empty.

The headers are compilable placeholder C++ (same style as `src/**`): real bases +
`virtual`, `int`/`void*`/`char[]` placeholder members, explicit `char _padNN[K]`
for unknown gaps. There are no `@offset`/`@size` annotations — a class's compilable
declaration *is* its layout. Each parses standalone under the MSVC-5.0 clang target
(`--target=i686-pc-windows-msvc -fms-extensions -fms-compatibility-version=1100`).

The full 231-RTTI-class accounting (per-class scaffold + knowledge tier) lives in
`docs/rtti-class-index.md`.

## Deletability test (per header)

A header is deletable once **all** its structs/enums are authoritatively in `src/`:
run `gruntz structs` before/after — the header is deletable iff
`build/gen/structs.json` + `build/gen/enums.json` drop **nothing** (namespaced
`WAP32::X` duplicates of a `src/` `X` are acceptable drops). Then `git rm` the
header and re-run `gruntz structs` to confirm.
