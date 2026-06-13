# zlib 1.0.4 — vendored matching source

## What / where
`vendor/zlib-1.0.4/` — the upstream zlib **1.0.4** source, vendored into the repo
(zlib license, so safe to commit).

- Source: `https://zlib.net/fossils/zlib-1.0.4.tar.gz`
- tarball sha256: `e5c260cd3db1370fb3e0c193e9cbd9f127a9bd055d622b3fb55b82747f6e5b24`
- Confirmed `#define ZLIB_VERSION "1.0.4"` in `zlib.h` — exactly the build statically
  linked into `GRUNTZ.EXE` (the binary carries `deflate 1.0.4 Copyright 1995-1996
  Jean-loup Gailly` / `inflate 1.0.4 Copyright 1995-1996 Mark Adler`).

## Why zlib is the first matching target
zlib is statically linked into `GRUNTZ.EXE` (it's the WWD/REZ decompressor — `.WWD`
levels are deflate-compressed). It's the **cleanest calibration target**: well-known
public source, small, and exactly versioned. Compiling it with our VC++ 5.0 SP3
toolchain and iterating compiler flags until the emitted objects **byte-match** the
zlib functions in `GRUNTZ.EXE` pins down the **exact flag set** (e.g. `/MT`, opt level
`/O1` vs `/O2`, `/GF`, struct packing, etc.) the whole binary was built with — the
foundation every later function match iterates from.

## In-scope translation units (likely linked into GRUNTZ.EXE)
`deflate.c inflate.c infblock.c infcodes.c inffast.c inftrees.c infutil.c trees.c
adler32.c zutil.c` (the deflate/inflate core + the default Adler-32 checksum + shared
utils). **Probably NOT linked** (confirm during matching): `crc32.c` (only used by the
gzip path), `gzio.c`/`compress.c`/`uncompr.c` (file/one-shot wrappers the engine
doesn't use — it reads from REZ/memory). `example.c`/`minigzip.c` are sample programs,
never linked.

## Rule: keep the upstream source pristine
For a clean match our source must EQUAL upstream zlib 1.0.4 — do not edit
`vendor/zlib-1.0.4/`. Anything the match needs (defines, packing, calling convention)
belongs in the **build configuration**, not the source. (Cross-check function bodies
against the FID/cross-game results in `docs/libraries-and-funcid.md`.)

## Status
Source vendored. Compilation + flag-iteration is **step 5** of the build-up plan and is
gated on the VC++ 5.0 toolchain (`nix develop .#build`).
