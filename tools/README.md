# `tools/` — clean-room codecs and the resource-format oracle

A Rust workspace, independent of the C++ decompilation under `src/`. It exists
to answer one question the decompilation cannot answer about itself: **is our
model of the sprite format right?**

`src/` and a Rust rewrite of `src/` would fail together. So the codecs here were
derived from two sources only — retail `GRUNTZ.EXE` disassembly and the archived
bytes — and are then checked against a *third* implementation that is neither:
retail's own machine code, mapped out of the EXE and executed.

    decomp   src/Image, src/DDrawMgr        the byte-matching C++ reconstruction
    reimpl   tools/gruntz-codec             clean-room, from the disassembly
    recomp   ../recomp/harness/pidrun.exe   retail's actual bytes, mapped and executed

## Layout

| crate | what it is |
|---|---|
| `gruntz-cast` | lossless integer conversions, so nothing else writes `as` |
| `gruntz-rez`  | Monolith REZ v1 archive reader (lib + `rezls`) |
| `gruntz-codec`| ANI / PAL / RID / XMI / PID / PCX / BMP / RLE16 parsing and codecs |
| `gruntz-oracle` | the differential runner over real assets |
| `recomp/`     | the MSVC/wine harness that calls into retail |

The three libraries are `#![no_std]` with **no `alloc`** and no third-party
dependency. That is load-bearing, not decoration: the APIs are zero-copy —
headers borrow the input, decoders fill a caller-supplied `&mut [u8]`, archive
traversal is an iterator over borrowed slices — so there is nothing for an
allocator to do. `std`, file IO and `clap` live in the binaries.

## Running it

The libraries need nothing but the pinned `rustc`. The two **binaries** use
`clap`, so the first build needs the crates.io registry (or a warm
`~/.cargo` cache); `Cargo.lock` is committed so the version is fixed. If a
fully offline build is ever required, `cargo vendor` into `tools/vendor/` is
the intended escape hatch - it was not done pre-emptively because it would add
a few thousand files nobody has asked to read.

```sh
nix develop
cd tools && cargo build --release && cargo test

REZ=/path/to/GRUNTDEM.REZ
./target/release/rezls "$REZ" census
./target/release/gruntz-oracle --rez "$REZ" census
./target/release/gruntz-oracle --rez "$REZ" roundtrip --literals any
./target/release/gruntz-oracle --rez "$REZ" decoders
./target/release/gruntz-oracle --rez "$REZ" rle16

# inspect animation control records and sound cues
./target/release/gruntz-oracle --rez "$REZ" ani 'AREA2\ANIZ\FORTSPLASH'

# render an optional preview; ANI does not store its image-set binding
./target/release/gruntz-oracle --rez "$REZ" ani 'AREA2\ANIZ\FORTSPLASH' \
  --gif fortsplash.gif --frames 'AREA2\IMAGEZ\FORTSPLASH'

# batch every ANI with a resolvable image-set binding, preserving REZ paths
./target/release/gruntz-oracle --rez "$REZ" ani-all ../build/animation-gifs

# preserve the green source palette, or select any of the 17 runtime colours
./target/release/gruntz-oracle --rez "$REZ" ani-all ../build/green-gifs \
  --tint source

# inspect one Miles XMI or convert the full music bank to standard MIDI
./target/release/gruntz-oracle --rez "$REZ" xmi 'AREA1\MIDIZ\INTRO0' \
  --midi intro0.mid
./target/release/gruntz-oracle --rez "$REZ" xmi-all ../build/music-midi

# MIDI is note/event data; play it through a synthesizer, not mpv/FFmpeg
timidity ../build/music-midi/AREA1/MIDIZ/AMBIENT0.mid
fluidsynth -i /path/to/Gruntz.SF2 \
  ../build/music-midi/AREA1/MIDIZ/AMBIENT0.mid

# the third implementation (needs wine + $GRUNTZ_EXE)
../recomp/harness/build.sh
./target/release/gruntz-oracle --rez "$REZ" recomp
```

`tokens <path>` prints retail's token stream beside our re-encoding.
`dump <path> out.bmp` writes PID and RID resources as indexed BMPs; RID has no
embedded palette, so `--palette <REZ-path>` supplies one and omission produces a
grey ramp. `ani <path>` exposes timing, frame-step, loop, movement and cue data.
Its optional GIF is explicitly a preview: ANI stores no pixels or image-set
name, so `--frames <REZ-prefix>` supplies the PID/RID set which the game object
normally binds separately. `ani-all` resolves the archive's conventional
ANI/IMAGEZ layouts, records every selected image set in `BINDINGS.tsv`, and
puts generic controllers or missing external bindings in `UNRESOLVED.tsv`
instead of inventing pixels. Grunt sprites embed the retail green TOOL/TOY
palette and are recoloured by palette substitution at draw time. GIF output
does the same exact-byte recognition and defaults to orange; `--tint` selects
any runtime colour, while `--tint source` preserves the embedded green table.

`xmi` exposes the sequence, timbre, event, and timing counts in a Miles XMIDI
resource. Its optional MIDI output expands XMIDI note durations into note-off
events and writes broadly compatible 60 PPQN / 120 BPM timing, preserving
Miles' 120 ticks/second clock exactly. `xmi-all` converts every music resource
while preserving its REZ path. Together these commands turn a disagreement
into something that can be read, viewed, or played rather than a percentage.

## Results

Corpus: `GRUNTDEM.REZ` (10 553 resources, 9 845 PID) and retail `Gruntz.REZ`
(21 303 resources, 19 953 PID) — **29 798 sprites**.

| check | result |
|---|---|
| archive walks with exact-size validation | 100 % (both archives) |
| sprites decode cleanly | **100 %** |
| `decode -> encode` byte-exact | **100 %** |
| our decoder vs **retail's own machine code** (9 821 `Rle` sprites) | **100 % identical pixels** |
| retail's two decoders agree with each other | 100 % — no shipped sprite crosses a scanline |
| PAL tables parse at exact size | **36 / 36** retail palettes |
| XMI parses and exports to MIDI | **37 / 37** retail music resources |

## What the format turned out to be

See the module docs for the per-field disassembly citations; the short version:

* **Two grammars, selected by `flags & 0x20`** (`CRezImage::DecodePidData`
  @0x1764ce) — a PCX-style `0xC0|count` run grammar, and a `0x80|n` fill-skip
  grammar with inline literal runs. Our `PidFlags` comment called `0x20`
  "COMPRESSION"; it is a *grammar selector*, and the bit being **clear** is what
  selects the RLE.
* **Two exporters**, distinguished by one bit of the literal test. Tiles and
  menus emit a bare literal whenever the decoder would accept one
  (`(v & 0xC0) != 0xC0`); sprites and booty only when `(v & 0xC0) == 0`. Recovering
  that is what took `roundtrip` from 85 % to 100 %.
* **Two decoders that disagree**, on a run that would cross a scanline:
  `CDDSurface::RunDecode1` clamps and carries, `CRezImage::DecodePidData` writes
  the whole run and spills. They consume a *different number of tokens*, so one
  such run desynchronises them permanently. No shipped sprite contains one.
* **The RLE16 row-end split is unobservable.** `EncodeRle16` ends a scanline at
  `x >= width - 1`, `DecodePidData` at `x >= width`. Only sprites with neither
  `0x40` nor `0x200` reach `EncodeRle16` at all: **0 of 6 940** in the demo,
  **5 of 13 037** in retail (`AREA8\IMAGEZ\UFO\FRAME001..005`), and on those
  five both rules walk the stream identically. Neither is "the bug".
* **Every flag bit has a reader.** An earlier pass called four of them
  unverified; widening the search to `DecodePcxData`, `CDDrawShadeBlit::Build`
  and `CImage::LoadDispatch` found one for each. `0x02`/`0x04` edit the
  `DDSCAPS_VIDEOMEMORY`/`DDSCAPS_SYSTEMMEMORY` surface caps; `0x40`/`0x200` say
  the payload is 8bpp indices (`0x40` also selects shade draw-type 2).
* **The `0x0C` trailing byte** on 11 % of sprites is the PCX end-of-image palette
  marker, left behind by the PCX->PID conversion. Harmless: retail addresses the
  palette from EOF and stops the token loop when the last row fills.
* **The fill-run cap is 126, not 127** — the exporter never emits the byte `0xFF`.

## Conventions

* No `as` casts. `gruntz-cast` provides `as_usize` / `as_u64` / `as_i64` for the
  conversions that are provably lossless on x86-64 (asserted at compile time),
  and `low_byte()` for the places the *format* deliberately truncates. Anything
  that could silently lose information uses `TryFrom` and reports the failure.
* Every public item cites the retail RVA that proves it.
* A hypothesis that the corpus refuted stays in the code as a named variant
  (`LiteralRule::HighBitClear`) rather than being deleted, so the next reader
  does not re-derive it.

This directory is not part of the `gruntz build` graph and does not affect the
C++ match.
