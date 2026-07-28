//! **PID** — the Gruntz sprite format.
//!
//! Everything here was derived from retail `GRUNTZ.EXE` disassembly and from
//! the archive bytes; nothing from the C++ under `src/`.
//!
//! # Container
//!
//! ```text
//! +0x00 u32 file_desc     small integer, purpose unproven
//! +0x04 u32 flags         see `flags`, below
//! +0x08 i32 width
//! +0x0c i32 height
//! +0x10 i32 offset_x      draw anchor, signed
//! +0x14 i32 offset_y
//! +0x18 u32 fill          fill/transparent colour; masked to the low word when
//!                         flags & 0x100  (retail 0x1764b2 `test ch,1`)
//! +0x1c u32 unk1
//! +0x20 ...  pixel stream
//! ... if flags & 0x80: the LAST 768 bytes of the resource are a VGA palette
//!     (256 x RGB, 0..255 - retail 0x145b8c reads `hdr + size - 0x300`).
//! ```
//!
//! `CDDSurface::DecodePid` @0x145b3d additionally requires `width % 4 == 0` and
//! that the destination surface already measures exactly `width x height`
//! (`cmp [eax+0x1c],ebx` / `cmp [eax+0x18],edx`). Those are constraints of that
//! *call path*, not of the format, so they are exposed as
//! [`PidHeader::decodepid_would_accept`] rather than enforced here.
//!
//! # The two pixel grammars
//!
//! `flags & 0x20` selects between two *different* stream grammars — proven at
//! `CRezImage::DecodePidData` 0x1764ce (`test cl,0x20 / je <0xC0-grammar>`):
//!
//! * **bit clear → [`Grammar::Rle`]** — PCX-style. Token `t`:
//!   `t & 0xC0 == 0xC0` ⇒ run of `t & 0x3F` copies of the *next* byte;
//!   otherwise ⇒ one literal byte `t`. A byte ≥ 0xC0 therefore cannot be a
//!   literal and must be spelled `C1 <byte>`.
//! * **bit set → [`Grammar::SkipRun`]** — token `t`:
//!   `t & 0x80` ⇒ `t - 0x80` pixels of the header's `fill` colour;
//!   otherwise ⇒ `t` literal bytes follow inline.
//!
//! # The two *decoders* — and they do not agree
//!
//! Retail ships two independent decoders for [`Grammar::Rle`], and they differ
//! on the one case the format leaves open: a run that would overrun the current
//! scanline.
//!
//! * [`RowOverrun::Carry`] — `CDDSurface::RunDecode1` @0x145270 (and its
//!   surface-locking twin `DecodeRun8` @0x140aa0). Clamps the run to the row
//!   and *carries* the remainder into the head of the next row.
//! * [`RowOverrun::Spill`] — `CRezImage::DecodePidData` @0x176440. Writes the
//!   full run unclamped (`rep stos` of `n` bytes at `dst`), so it spills past
//!   the row end into the next scanline's pixels.
//!
//! Whether that matters is an empirical question about the shipped data, which
//! is what `gruntz-oracle` answers.

use core::fmt;

use gruntz_cast::{AsI64, AsUsize, LowByte};

use crate::Sink;

pub const HEADER_SIZE: usize = 0x20;
pub const PALETTE_SIZE: usize = 768;

/// Sanity ceiling on `width * height`. Nothing in the archives comes close;
/// it exists so a corrupt header cannot ask for a terabyte.
pub const MAX_PIXELS: i64 = 1 << 28;

/// Longest run the `0xC0` token can express.
pub const MAX_RLE_RUN: u8 = 0x3f;
/// Longest run either `SkipRun` token can express.
pub const MAX_SKIPRUN: u8 = 0x7f;

/// Longest *fill* run the encoder will emit.
///
/// The token can express 127, but the shipping exporter stops at 126: a
/// 132-pixel transparent scanline is spelled `FE 86` (126 + 6), not `FF 85`
/// (127 + 5). So it never emits the byte `0xFF`. Reproducer:
/// `GRUNTZ\IMAGEZ\NERFGUNGRUNT\PROJECTILE\SHADOW\FRAME001`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum FillRunCap {
    /// 126 — never emit the `0xFF` token. What the archives contain.
    #[default]
    AvoidFf,
    /// 127 — everything the token can express.
    Full,
}

impl FillRunCap {
    fn max(self) -> u8 {
        match self {
            FillRunCap::AvoidFf => MAX_SKIPRUN - 1,
            FillRunCap::Full => MAX_SKIPRUN,
        }
    }
}

/// The `+0x04` flag word.
///
/// Only four of these bits are *read* by any code path this crate transcribes.
/// The rest are listed because the archives contain them, not because their
/// meaning is proven — see [`UNVERIFIED`] and [`UNEXPLAINED`].
///
/// Every flag word present in the shipped archives (demo / retail counts):
///
/// ```text
/// 0x0080   762 /  2580   EMBEDDED_PALETTE                                   Rle
/// 0x0081  2143 /  4336   EMBEDDED_PALETTE|TRANSPARENCY                      Rle
/// 0x0165   109 /   160   TRANSPARENCY|0x04|COMPRESSION|0x40|FILL_IS_WORD    SkipRun
/// 0x01a5     0 /     5   0x0165 - 0x40 + EMBEDDED_PALETTE                   SkipRun
/// 0x03a4     5 /     5   0x03a5 - TRANSPARENCY                              SkipRun
/// 0x03a5  6826 / 12867   ...|EMBEDDED_PALETTE|FILL_IS_WORD|0x200            SkipRun
/// ```
pub mod flags {
    /// Install the transparent colour key after decoding. Read by
    /// `CDDSurface::DecodePid` @0x145c97 (`test BYTE PTR [esp+0x18],1` then
    /// `FillPalette(colorKey)`).
    pub const TRANSPARENCY: u32 = 0x01;
    /// Selects the skip/fill grammar instead of the 0xC0 run grammar. Read by
    /// `CRezImage::DecodePidData` @0x1764ce (`test cl,0x20`). Note the sense:
    /// the bit being **clear** selects the run-length grammar.
    pub const COMPRESSION: u32 = 0x20;
    /// A 768-byte VGA palette is appended at end-of-resource. Read by
    /// `CDDSurface::DecodePid` @0x145b85 (`test BYTE PTR [esp+0x18],0x80`).
    pub const EMBEDDED_PALETTE: u32 = 0x80;
    /// `fill` is a 16bpp value: mask it to the low word. Read by
    /// `CRezImage::DecodePidData` @0x1764b2 (`test ch,1`).
    pub const FILL_IS_WORD: u32 = 0x100;

    /// Named in the C++ reconstruction's `PidFlags` but **not verified here**:
    /// no code path this crate transcribes reads either bit, and no shipped
    /// sprite sets `0x02` at all. Kept only so the numbering is not silently
    /// re-used.
    pub const VIDEO_MEMORY: u32 = 0x02;
    /// See [`VIDEO_MEMORY`]. `0x04` *is* set on every `SkipRun` sprite, but
    /// nothing reads it, so "system memory" is inherited folklore.
    pub const SYSTEM_MEMORY: u32 = 0x04;

    /// Bits present in the archives that no transcribed code path reads.
    ///
    /// `0x40` appears on exactly the `SkipRun` sprites that have **no**
    /// embedded palette (flag word `0x0165`), and `0x200` on exactly those that
    /// **do** (`0x03a4`/`0x03a5`) - which is suggestive, and is as far as the
    /// evidence goes.
    pub const UNEXPLAINED: u32 = 0x40 | 0x200;

    /// The bits above that are declared but unproven.
    pub const UNVERIFIED: u32 = VIDEO_MEMORY | SYSTEM_MEMORY;
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Grammar {
    /// `flags & 0x20 == 0`: PCX-style `0xC0 | count` runs.
    Rle,
    /// `flags & 0x20 != 0`: `0x80 | n` fill-skip plus inline literal runs.
    SkipRun,
}

/// Which retail decoder's row-overrun behaviour to reproduce.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RowOverrun {
    /// `CDDSurface::RunDecode1` @0x145270 — clamp and carry into the next row.
    Carry,
    /// `CRezImage::DecodePidData` @0x176440 — write the whole run, spilling.
    Spill,
}

/// Validated sprite geometry. Constructing one proves `width * height` fits and
/// is positive, which is what lets the decoders index without further checks.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Dims {
    width: u32,
    height: u32,
}

impl Dims {
    pub fn new(width: i32, height: i32) -> Result<Dims, PidError> {
        let bad = PidError::BadDimensions { width, height };
        if width <= 0 || height <= 0 || width.as_i64() * height.as_i64() > MAX_PIXELS {
            return Err(bad);
        }
        Ok(Dims {
            width: u32::try_from(width).map_err(|_| bad)?,
            height: u32::try_from(height).map_err(|_| bad)?,
        })
    }

    pub fn width(self) -> usize {
        self.width.as_usize()
    }
    pub fn height(self) -> usize {
        self.height.as_usize()
    }
    pub fn pixel_len(self) -> usize {
        self.width.as_usize() * self.height.as_usize()
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PidHeader {
    pub file_desc: u32,
    pub flags: u32,
    pub width: i32,
    pub height: i32,
    pub offset_x: i32,
    pub offset_y: i32,
    pub fill: u32,
    pub unk1: u32,
}

impl PidHeader {
    pub fn parse(b: &[u8]) -> Result<PidHeader, PidError> {
        let h: &[u8; HEADER_SIZE] =
            b.get(..HEADER_SIZE)
                .and_then(|s| s.try_into().ok())
                .ok_or(PidError::Truncated {
                    need: HEADER_SIZE,
                    have: b.len(),
                })?;
        let word = |i: usize| [h[i], h[i + 1], h[i + 2], h[i + 3]];
        Ok(PidHeader {
            file_desc: u32::from_le_bytes(word(0x00)),
            flags: u32::from_le_bytes(word(0x04)),
            width: i32::from_le_bytes(word(0x08)),
            height: i32::from_le_bytes(word(0x0c)),
            offset_x: i32::from_le_bytes(word(0x10)),
            offset_y: i32::from_le_bytes(word(0x14)),
            fill: u32::from_le_bytes(word(0x18)),
            unk1: u32::from_le_bytes(word(0x1c)),
        })
    }

    pub fn grammar(self) -> Grammar {
        if self.flags & flags::COMPRESSION != 0 {
            Grammar::SkipRun
        } else {
            Grammar::Rle
        }
    }

    pub fn has_palette(self) -> bool {
        self.flags & flags::EMBEDDED_PALETTE != 0
    }

    pub fn dims(self) -> Result<Dims, PidError> {
        Dims::new(self.width, self.height)
    }

    /// The fill byte the `SkipRun` grammar stamps, reproducing retail's
    /// `test ch,1 / and eax,0xffff` (0x1764b2) followed by a byte-wide
    /// `rep stos` (0x176546): only the low byte ever reaches the 8bpp surface,
    /// and when `flags & 0x100` is clear retail stamps a hard zero.
    pub fn fill_byte(self) -> u8 {
        if self.flags & flags::FILL_IS_WORD != 0 {
            self.fill.low_byte()
        } else {
            0
        }
    }

    /// Would `CDDSurface::DecodePid` @0x145b10 accept this sprite for a surface
    /// of the same size? It rejects `width % 4 != 0` outright (0x145b3d
    /// `test bl,3`).
    pub fn decodepid_would_accept(self) -> bool {
        self.width % 4 == 0
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PidError {
    Truncated {
        need: usize,
        have: usize,
    },
    /// `width <= 0 || height <= 0`, or the pixel count is absurd.
    BadDimensions {
        width: i32,
        height: i32,
    },
    /// The caller's output buffer is not exactly `width * height`.
    BadDestination {
        need: usize,
        have: usize,
    },
    /// The token stream ran out before every row was filled.
    StreamExhausted {
        row: usize,
        remaining: usize,
        at: usize,
    },
    /// A run token with count 0. Retail's `RunDecode1` subtracts 0 from
    /// `remaining` and loops forever on this; we refuse instead.
    ZeroRun {
        at: usize,
    },
    /// A run wrote past the end of the pixel buffer (`Spill`), or a carry
    /// longer than a whole scanline walked off its row (`Carry`).
    Overrun {
        at: usize,
        row: usize,
    },
    /// `flags & 0x80` but the resource cannot hold a 768-byte palette.
    MissingPalette {
        size: usize,
    },
    /// An encoder ran out of destination space.
    OutputFull {
        need: usize,
        have: usize,
    },
}

impl fmt::Display for PidError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            PidError::Truncated { need, have } => write!(f, "truncated: need {need}, have {have}"),
            PidError::BadDimensions { width, height } => {
                write!(f, "bad dimensions {width}x{height}")
            }
            PidError::BadDestination { need, have } => {
                write!(f, "destination is {have} bytes, need exactly {need}")
            }
            PidError::StreamExhausted { row, remaining, at } => write!(
                f,
                "stream exhausted at byte {at}: row {row} still wants {remaining} pixel(s)"
            ),
            PidError::ZeroRun { at } => {
                write!(f, "zero-length run token at byte {at} (retail hangs here)")
            }
            PidError::Overrun { at, row } => {
                write!(f, "run at byte {at} overran the buffer on row {row}")
            }
            PidError::MissingPalette { size } => {
                write!(f, "flags say palette but the resource is only {size} bytes")
            }
            PidError::OutputFull { need, have } => {
                write!(f, "output buffer holds {have}, need {need}")
            }
        }
    }
}

impl core::error::Error for PidError {}

/// A PID resource, split into its three borrowed parts. Nothing is copied.
#[derive(Debug, Clone, Copy)]
pub struct Pid<'a> {
    pub header: PidHeader,
    /// The token stream: resource `[0x20 .. len - palette]`.
    pub stream: &'a [u8],
    /// The trailing 768-byte VGA palette, present iff `flags & 0x80`.
    pub palette: Option<&'a [u8]>,
}

/// Split a resource into (header, token stream, palette) without copying.
pub fn split(resource: &[u8]) -> Result<Pid<'_>, PidError> {
    let header = PidHeader::parse(resource)?;
    if header.has_palette() {
        // Retail: `if (size <= 0x300) return 0;` then reads `hdr + size - 0x300`.
        let cut = resource
            .len()
            .checked_sub(PALETTE_SIZE)
            .filter(|c| *c >= HEADER_SIZE)
            .ok_or(PidError::MissingPalette {
                size: resource.len(),
            })?;
        Ok(Pid {
            header,
            stream: &resource[HEADER_SIZE..cut],
            palette: Some(&resource[cut..]),
        })
    } else {
        Ok(Pid {
            header,
            stream: &resource[HEADER_SIZE..],
            palette: None,
        })
    }
}

impl Pid<'_> {
    /// Decode into `dst`, which must be exactly `width * height` bytes.
    /// Returns the number of stream bytes consumed — compare it against
    /// `self.stream.len()` to detect a short or over-long stream.
    ///
    /// Pixels are written **top-down**. Retail's `CRezImage` path stores them
    /// bottom-up (its row table is `(height-1-y) * stride`), but that is a
    /// Windows-DIB storage convention, not the format: `CDDSurface::RunDecode1`
    /// writes the very same token stream top-down into `dst + width*y`.
    pub fn decode_into(&self, dst: &mut [u8], overrun: RowOverrun) -> Result<usize, PidError> {
        let dims = self.header.dims()?;
        match self.header.grammar() {
            Grammar::Rle => decode_rle_into(self.stream, dst, dims, overrun),
            Grammar::SkipRun => {
                decode_skiprun_into(self.stream, dst, dims, self.header.fill_byte())
            }
        }
    }

    /// Re-encode `pixels` with this sprite's own grammar. Pair with
    /// [`Pid::encoded_len`].
    pub fn encode_into(
        &self,
        pixels: &[u8],
        dims: Dims,
        rule: LiteralRule,
        dst: &mut [u8],
    ) -> Result<usize, PidError> {
        match self.header.grammar() {
            Grammar::Rle => encode_rle_into(pixels, dims, rule, dst),
            Grammar::SkipRun => encode_skiprun_into(
                pixels,
                dims,
                self.header.fill_byte(),
                FillRunCap::default(),
                dst,
            ),
        }
    }

    pub fn encoded_len(&self, pixels: &[u8], dims: Dims, rule: LiteralRule) -> usize {
        match self.header.grammar() {
            Grammar::Rle => encoded_rle_len(pixels, dims, rule),
            Grammar::SkipRun => {
                encoded_skiprun_len(pixels, dims, self.header.fill_byte(), FillRunCap::default())
            }
        }
    }
}

fn check_dst(dst: &[u8], dims: Dims) -> Result<(), PidError> {
    if dst.len() != dims.pixel_len() {
        return Err(PidError::BadDestination {
            need: dims.pixel_len(),
            have: dst.len(),
        });
    }
    Ok(())
}

// ---------------------------------------------------------------------------
// decoders
// ---------------------------------------------------------------------------

/// The PCX-style grammar. `dst` must be `width * height`; returns the stream
/// bytes consumed.
///
/// Transcribed from `CDDSurface::RunDecode1` @0x145270 ([`RowOverrun::Carry`])
/// and `CRezImage::DecodePidData` @0x1765e8 ([`RowOverrun::Spill`]).
///
/// Retail keeps `remaining` in a signed register and lets `Spill` drive it
/// negative; a `saturating_sub` here is behaviourally identical, because the
/// only thing that ever reads `remaining` afterwards is the `> 0` loop test.
pub fn decode_rle_into(
    stream: &[u8],
    dst: &mut [u8],
    dims: Dims,
    overrun: RowOverrun,
) -> Result<usize, PidError> {
    check_dst(dst, dims)?;
    let width = dims.width();
    let mut p = 0usize; // token cursor
    let mut carry = 0usize; // pixels of `carry_val` owed to the next row
    let mut carry_val = 0u8;

    for y in 0..dims.height() {
        let row = y * width;
        let mut x = 0usize;
        let mut remaining = width;

        if overrun == RowOverrun::Carry && carry > 0 {
            // retail 0x1452e9: the carry loop is NOT clamped to the new row, so
            // a run longer than a whole scanline would walk off it. The shipped
            // encoder never does that; refuse rather than reproduce an
            // out-of-bounds write.
            if carry > width {
                return Err(PidError::Overrun { at: p, row: y });
            }
            dst[row..row + carry].fill(carry_val);
            x = carry;
            remaining -= carry;
            carry = 0;
        }

        while remaining > 0 {
            let exhausted = PidError::StreamExhausted {
                row: y,
                remaining,
                at: p,
            };
            let tok = *stream.get(p).ok_or(exhausted)?;
            p += 1;
            let val_at = p;
            if tok & 0xc0 == 0xc0 {
                let val = *stream.get(val_at).ok_or(PidError::StreamExhausted {
                    row: y,
                    remaining,
                    at: val_at,
                })?;
                p += 1;
                let mut n = (tok & MAX_RLE_RUN).as_usize();
                if n == 0 {
                    return Err(PidError::ZeroRun { at: p - 2 });
                }
                match overrun {
                    RowOverrun::Carry => {
                        if n > remaining {
                            carry = n - remaining;
                            carry_val = val;
                            n = remaining;
                        }
                    }
                    RowOverrun::Spill => {
                        // 0x176646: `rep stos` of the full n at dst, unclamped.
                        if row + x + n > dst.len() {
                            return Err(PidError::Overrun { at: p - 2, row: y });
                        }
                    }
                }
                dst[row + x..row + x + n].fill(val);
                x += n;
                remaining = remaining.saturating_sub(n);
            } else {
                dst[row + x] = tok;
                x += 1;
                remaining -= 1;
            }
        }
    }
    Ok(p)
}

/// The `flags & 0x20` grammar. `dst` must be `width * height`; returns the
/// stream bytes consumed.
///
/// Transcribed from `CRezImage::DecodePidData` @0x176515. The row advance is
/// `x >= width` (retail `cmp edx,[eax+0x438]` @0x176597, where +0x438 is the
/// width stored by `DecodeBmpHeader` @0x1757d8) — **not** `width - 1`; see
/// [`crate::rle16`] for the other retail spelling of that test.
pub fn decode_skiprun_into(
    stream: &[u8],
    dst: &mut [u8],
    dims: Dims,
    fill: u8,
) -> Result<usize, PidError> {
    check_dst(dst, dims)?;
    let width = dims.width();
    dst.fill(fill);
    let mut p = 0usize;
    let mut y = 0usize;
    let mut x = 0usize;

    while y < dims.height() {
        let exhausted = PidError::StreamExhausted {
            row: y,
            remaining: width - x,
            at: p,
        };
        let tok = *stream.get(p).ok_or(exhausted)?;
        let base = y * width;
        let n = if tok & 0x80 != 0 {
            let n = (tok - 0x80).as_usize();
            if x + n > width {
                return Err(PidError::Overrun { at: p, row: y });
            }
            dst[base + x..base + x + n].fill(fill);
            p += 1;
            n
        } else {
            let n = tok.as_usize();
            let src = stream.get(p + 1..p + 1 + n).ok_or(exhausted)?;
            if x + n > width {
                return Err(PidError::Overrun { at: p, row: y });
            }
            dst[base + x..base + x + n].copy_from_slice(src);
            p += 1 + n;
            n
        };
        x += n;
        if x >= width {
            y += 1;
            x = 0;
        }
    }
    Ok(p)
}

// ---------------------------------------------------------------------------
// encoders
// ---------------------------------------------------------------------------

/// Which single bytes an encoder is willing to spell as a bare literal.
///
/// The decoder only requires that a literal not look like a run tag, i.e.
/// `(v & 0xC0) != 0xC0`. Which of the legal spellings the *shipping tool*
/// chose is an empirical question, and the answer is: **it depends on the
/// asset group**, which is itself the finding.
///
/// Measured over `GRUNTDEM.REZ` (9 845 sprites), byte-exact re-encode rate:
///
/// | rule            | TILEZ     | MENU      | IMAGEZ (Rle) | BOOTY   |
/// |-----------------|-----------|-----------|--------------|---------|
/// | `Decodable`     | **100 %** | **100 %** | 37.6 %       | 6.5 %   |
/// | `HighBitClear`  | 3.1 %     | 17.0 %    | 22.2 %       | 6.5 %   |
/// | `Never`         | 1.7 %     | 0 %       | 0.7 %        | 0 %     |
///
/// So `Decodable` is exactly the tile/menu exporter, and the `IMAGEZ`/`BOOTY`
/// sprite exporter is something else again — one that emits a *larger* stream
/// than the greedy rule on every sprite it misses. See
/// `gruntz-oracle tokens` for the per-token evidence.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum LiteralRule {
    /// `(v & 0xC0) != 0xC0` — every spelling the decoder accepts, chosen
    /// greedily. Reproduces the tile and menu exporters exactly; the default.
    #[default]
    Decodable,
    /// `(v & 0xC0) == 0` — a lone byte outside the low six bits is spelled
    /// `C1 v`. This is the `IMAGEZ`/`BOOTY` sprite exporter: it looks like
    /// `if (v & 0xC0) emit_run(); else emit_literal();`, i.e. the run test
    /// written as `!= 0` where the decoder's test is `== 0xC0`.
    LowSixBits,
    /// `v < 0x80` — a lone byte with the high bit set is spelled `C1 v`.
    /// Tested because the first `IMAGEZ` divergence looked like exactly this;
    /// the token-level diff refuted it (the residual divergences are all in
    /// `0x40..0x7f`), and it is kept so that stays refuted rather than being
    /// re-guessed.
    HighBitClear,
    /// No bare literals at all: every single pixel is spelled `C1 v`, so the
    /// stream is a flat sequence of run tokens. Some shipped sprites exceed one
    /// byte per pixel, which only a rule this conservative can produce — but it
    /// is not the rule either (0.8 % exact).
    Never,
}

impl LiteralRule {
    pub fn allows(self, v: u8) -> bool {
        match self {
            LiteralRule::Decodable => v & 0xc0 != 0xc0,
            LiteralRule::LowSixBits => v & 0xc0 == 0,
            LiteralRule::HighBitClear => v < 0x80,
            LiteralRule::Never => false,
        }
    }
}

/// The single definition of the PCX-style encoder; [`encoded_rle_len`] and
/// [`encode_rle_into`] are the two ways to run it.
///
/// The canonical spelling, inferred from the shipped streams and then checked
/// against every archived sprite:
///
/// * runs never cross a scanline (each row restarts);
/// * a run of `n >= 2` identical bytes is emitted as `C0|min(n,63), value` —
///   so `n == 2` prefers the run over two literals, a size tie and therefore a
///   real choice the original encoder made;
/// * a lone byte is a literal iff `rule` allows it, otherwise `C1, value`.
fn rle_tokens(pixels: &[u8], dims: Dims, rule: LiteralRule, sink: &mut Sink<'_>) -> bool {
    let width = dims.width();
    for y in 0..dims.height() {
        let row = &pixels[y * width..(y + 1) * width];
        let mut i = 0usize;
        while i < row.len() {
            let v = row[i];
            // `n` is a u8 by construction: the loop stops at MAX_RLE_RUN.
            let mut n = 1u8;
            while i + n.as_usize() < row.len() && row[i + n.as_usize()] == v && n < MAX_RLE_RUN {
                n += 1;
            }
            let ok = if n >= 2 || !rule.allows(v) {
                sink.push(0xc0 | n) && sink.push(v)
            } else {
                sink.push(v)
            };
            if !ok {
                return false;
            }
            i += n.as_usize();
        }
    }
    true
}

pub fn encoded_rle_len(pixels: &[u8], dims: Dims, rule: LiteralRule) -> usize {
    let mut s = Sink::Count(0);
    rle_tokens(pixels, dims, rule, &mut s);
    s.len()
}

pub fn encode_rle_into(
    pixels: &[u8],
    dims: Dims,
    rule: LiteralRule,
    dst: &mut [u8],
) -> Result<usize, PidError> {
    let have = dst.len();
    let mut s = Sink::Write { buf: dst, at: 0 };
    if !rle_tokens(pixels, dims, rule, &mut s) {
        return Err(PidError::OutputFull {
            need: encoded_rle_len(pixels, dims, rule),
            have,
        });
    }
    Ok(s.len())
}

/// The `flags & 0x20` encoder: a `fill`-coloured span becomes `0x80 | n`
/// (n <= 0x7f), anything else a literal run `n, bytes...` (n <= 0x7f). Rows
/// restart and close at `x == width`.
fn skiprun_tokens(
    pixels: &[u8],
    dims: Dims,
    fill: u8,
    cap: FillRunCap,
    sink: &mut Sink<'_>,
) -> bool {
    let width = dims.width();
    for y in 0..dims.height() {
        let row = &pixels[y * width..(y + 1) * width];
        let mut i = 0usize;
        while i < row.len() {
            let run_of_fill = row[i] == fill;
            let limit = if run_of_fill { cap.max() } else { MAX_SKIPRUN };
            let mut n = 0u8;
            while i + n.as_usize() < row.len()
                && (row[i + n.as_usize()] == fill) == run_of_fill
                && n < limit
            {
                n += 1;
            }
            let ok = if run_of_fill {
                sink.push(0x80 | n)
            } else {
                sink.push(n) && sink.extend(&row[i..i + n.as_usize()])
            };
            if !ok {
                return false;
            }
            i += n.as_usize();
        }
    }
    true
}

pub fn encoded_skiprun_len(pixels: &[u8], dims: Dims, fill: u8, cap: FillRunCap) -> usize {
    let mut s = Sink::Count(0);
    skiprun_tokens(pixels, dims, fill, cap, &mut s);
    s.len()
}

pub fn encode_skiprun_into(
    pixels: &[u8],
    dims: Dims,
    fill: u8,
    cap: FillRunCap,
    dst: &mut [u8],
) -> Result<usize, PidError> {
    let have = dst.len();
    let mut s = Sink::Write { buf: dst, at: 0 };
    if !skiprun_tokens(pixels, dims, fill, cap, &mut s) {
        return Err(PidError::OutputFull {
            need: encoded_skiprun_len(pixels, dims, fill, cap),
            have,
        });
    }
    Ok(s.len())
}
