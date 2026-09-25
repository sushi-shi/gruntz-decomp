# `tools/` — clean-room codecs and the resource-format oracle

A Rust workspace, independent of the C++ decompilation under `src/`. It exists
to answer one question the decompilation cannot answer about itself: **is our
model of the resource formats right?**

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
| `gruntz-rez`  | Monolith REZ/VRZ v1 reader **and writer**, plus the FEC reader (`rezls`, `rezpack`, `fecls`) |
| `gruntz-codec`| ANI / FNT / PAL / RID / WWD / XMI / PID / PCX / BMP / RLE16 parsing and codecs |
| `gruntz-oracle` | the differential runner plus loose-asset renderers (`fntdump`) |
| `recomp/`     | the MSVC/wine harness that calls into retail |

The three libraries are `#![no_std]` and their READERS have **no `alloc`** and
no third-party dependency. That is load-bearing, not decoration: the APIs are
zero-copy — headers borrow the input, decoders fill a caller-supplied
`&mut [u8]`, archive traversal is an iterator over borrowed slices — so there is
nothing for an allocator to do. `std`, file IO, compression and PNG output live
in the binaries.

The one exception is `gruntz-rez`'s **writer**, which cannot be: a REZ directory
body's length is unknown until its children are placed. It sits behind the
`alloc` feature (on by default so `cargo build` produces `rezpack`), still
`no_std`, and it borrows payloads rather than copying them.
`cargo build -p gruntz-rez --lib --no-default-features` builds the
allocation-free reader alone; the reader never touches `alloc` in either
configuration.

## Running it

The libraries need nothing but the pinned `rustc`. The **binaries** use
`clap`, `flate2`, and `png`, so the first build needs the crates.io registry (or a warm
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

# VRZ is the same REZ v1 container; retail contains 1,517 voice WAVs
./target/release/rezls /path/to/GRUNTZ.VRZ census

# write REZ v1. `roundtrip` is the writer's real test: decode -> re-encode ->
# re-parse -> compare every resource's path, type, id, time, comment, keys and
# payload bytes, plus every directory and its time
./target/release/rezpack roundtrip "$REZ"
./target/release/rezpack check     "$REZ"     # validate + the is_sorted predicate
./target/release/rezpack unpack    "$REZ" ../build/rez-tree
./target/release/rezpack pack      ../build/rez-tree ../build/rebuilt.rez

# inspect all four loose bitmap fonts and render 16x16 glyph atlases
./target/release/fntdump /path/to/GAME/{LARGE,MEDIUM,SMALL,TINY}.FNT \
  --out ../build/font-atlases

# list or extract the three embedded Smacker movies from either FEC archive
./target/release/fecls /path/to/MOVIEZ/GRUNTZ.FEC
./target/release/fecls /path/to/MOVIEZ/GRUNTZ.FEC extract-all \
  ../build/movies-high

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
  --midi intro0.mid --wav intro0.wav
./target/release/gruntz-oracle --rez "$REZ" xmi-all ../build/music-midi --wav

# render one WWD or all maps; non-main planes are emitted separately
./target/release/gruntz-oracle --rez "$REZ" wwd \
  'AREA1\WORLDZ\LEVEL1' ../build/level1.png
./target/release/gruntz-oracle --rez "$REZ" wwd-all ../build/map-pngs

# MIDI is note/event data; synthesize it before handing it to mpv/FFmpeg
timidity ../build/music-midi/AREA1/MIDIZ/AMBIENT0.mid
timidity -Ow -o ambient0.wav ../build/music-midi/AREA1/MIDIZ/AMBIENT0.mid
mpv ambient0.wav
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
ANI/IMAGEZ layouts and the frame bindings proven by reconstructed call sites.
When one controller is applied to several sets, it emits named variants such as
`PYRAMIDUP--RED.gif` and records the variant, selected image set, and output in
`BINDINGS.tsv`. Generic controllers or missing external bindings remain in
`UNRESOLVED.tsv` instead of acquiring compatible-looking but unevidenced pixels.
Grunt sprites embed the retail green TOOL/TOY
palette and are recoloured by palette substitution at draw time. GIF output
does the same exact-byte recognition and defaults to orange; `--tint` selects
any runtime colour, while `--tint source` preserves the embedded green table.

`xmi` exposes the sequence, timbre, event, and timing counts in a Miles XMIDI
resource. Its optional MIDI output expands XMIDI note durations into note-off
events and writes broadly compatible 60 PPQN / 120 BPM timing, preserving
Miles' 120 ticks/second clock exactly. `xmi-all` converts every music resource
while preserving its REZ path; `--wav` renders every track. `xmi --wav` invokes the pinned TiMidity
synthesizer to make an ordinary PCM preview that mpv can play; its configured
instrument bank is useful for inspection but is not the original Gruntz
SoundFont. Together these commands turn a disagreement into something that can
be read, viewed, or played rather than a percentage.

`wwd` inflates and validates the world file, follows its declared `TILEZ` and
`IMAGEZ` registry bindings, and renders the plane carrying the retail main-plane
flag to the requested PNG. Other planes are written to a sibling
`<name>-planes/` directory because their parallax relationship depends on the
camera. `wwd-all` preserves REZ paths, renders the complete archive, and writes
`UNRESOLVED.tsv`; the retail corpus resolves every tile reference.

## Format layouts

The implementations own the format details:
[REZ reader](gruntz-rez/src/lib.rs) and [writer](gruntz-rez/src/write.rs),
[WWD](gruntz-codec/src/wwd.rs), and [ANI](gruntz-codec/src/ani.rs).
The REZ writer and WWD parser include ASCII layout diagrams.

Generate corpus results with the commands above instead of maintaining a second
field map or a fixed score table here. Retail bytes and their consumers remain
the comparison evidence; neither implementation is ground truth by itself.

## Conventions

* No `as` casts. `gruntz-cast` provides `as_usize` / `as_u64` / `as_i64` for the
  conversions that are provably lossless on x86-64 (asserted at compile time),
  and `low_byte()` for the places the *format* deliberately truncates. Anything
  that could silently lose information uses `TryFrom` and reports the failure.
* Every public item cites the evidence that proves it, which is the retail RVA
  wherever the proof is retail's own code. Two kinds of module legitimately
  cannot: `xmi.rs`, derived from Miles' shipped declarations
  (`vendor/miles-6.0c/mss.h`) rather than from `GRUNTZ.EXE`, and the oracle's
  own output writers `gif.rs` / `midi.rs`, which encode formats Gruntz never
  reads. Everything else is engine-read and cites retail — including `bmp.rs`
  (`CDDSurface::DecodeBmp` @0x143fc0, `SaveRle16` @0x144640). A citation-free
  claim outside those two kinds is a gap, not a convention.
* A hypothesis that the corpus refuted stays in the code as a named variant
  (`LiteralRule::HighBitClear`) rather than being deleted, so the next reader
  does not re-derive it.

This directory is not part of the `gruntz build` graph and does not affect the
C++ match.
