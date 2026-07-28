//! **RLE16** — the 16bpp shade-blit sprite stream (`CDDrawShadeBlit`).
//!
//! This is *not* the PID grammar. It is the runtime form the engine builds when
//! it promotes an 8bpp sprite to a 16bpp display: same token stream, literals
//! widened from one palette index to one little-endian 16bpp pixel.
//!
//! Transcribed from `CDDrawShadeBlit::EncodeRle16` @0x1495d0 (both passes) and
//! its reader `CDDrawShadeBlit::DecodeFrame` @0x149250.
//!
//! # Token grammar (identical on both sides)
//!
//! ```text
//! t & 0x80  -> transparent skip of (t - 0x80) pixels.        1 byte in, 1 byte out
//! else      -> literal run of t pixels.        1 + t bytes in, 1 + 2*t bytes out
//! ```
//!
//! # The `width - 1` row terminator — RESOLVED: unobservable
//!
//! `EncodeRle16` closes a scanline at `x >= width - 1` (0x149694 and 0x14973d:
//! `mov edx,[ebp+4] / dec edx / cmp ...,edx / jl`), whereas the PID skip-run
//! decoder `CRezImage::DecodePidData` closes at `x >= width` (0x176597,
//! `cmp edx,[eax+0x438]`). Both are retail, and the disassembly cannot say
//! which is intended.
//!
//! The archives can, once you work out which streams reach the function.
//! `CDDrawShadeBlit::Build` @0x1490d0 `memcpy`s the PID skip/fill stream into
//! `m_rleData` **verbatim** (with `m_width`/`m_height` from the same header),
//! then calls `EncodeRle16` only when `m_srcBpp == 2` — which needs NEITHER
//! `PID_SRC_8BPP_SHADE` (0x40) NOR `PID_SRC_8BPP` (0x200). So the input corpus
//! is exactly the skip/fill sprites carrying neither bit, and
//! `gruntz-oracle rle16` counts them:
//!
//! * `GRUNTDEM.REZ`: **0 of 6 940**. The `width - 1` terminator is dead code.
//! * retail `Gruntz.REZ`: **5 of 13 037** — `AREA8\IMAGEZ\UFO\FRAME001..005`,
//!   64x64, flag word `0x01a5` — and on all five the two rules walk the stream
//!   **identically**: same rows, same byte count, neither runs short.
//!
//! They can only diverge when a token boundary lands exactly on `x ==
//! width - 1`, i.e. when a row ends with a one-pixel token. No shipped stream
//! does. So neither rule is "the bug"; they simply never disagree on data that
//! exists, and [`RowEnd`] keeps both spellings addressable rather than picking
//! one and calling it truth.

use core::fmt;

use gruntz_cast::AsUsize;

use crate::Sink;

/// Which retail spelling of the scanline terminator to use.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RowEnd {
    /// `EncodeRle16` @0x149694: a scanline ends at `x >= width - 1`.
    WidthMinusOne,
    /// `CRezImage::DecodePidData` @0x176597: a scanline ends at `x >= width`.
    Width,
}

impl RowEnd {
    fn limit(self, width: u32) -> usize {
        match self {
            RowEnd::WidthMinusOne => width.as_usize().saturating_sub(1),
            RowEnd::Width => width.as_usize(),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Rle16Error {
    StreamExhausted { row: usize, at: usize },
    OutputFull { need: usize, have: usize },
}

impl fmt::Display for Rle16Error {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Rle16Error::StreamExhausted { row, at } => {
                write!(f, "stream exhausted at byte {at} on row {row}")
            }
            Rle16Error::OutputFull { need, have } => {
                write!(f, "output buffer holds {have}, need {need}")
            }
        }
    }
}

impl core::error::Error for Rle16Error {}

/// One decoded token, borrowing its payload from the source stream.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Token<'a> {
    /// `t - 0x80` transparent pixels.
    Skip(u8),
    /// `t` literal palette indices.
    Literal(&'a [u8]),
}

/// A borrowing, allocation-free walk of a shade stream.
///
/// Yields `(row, token)`, stopping after `height` rows. A truncated stream
/// yields `Err` as its final item.
pub struct Tokens<'a> {
    src: &'a [u8],
    p: usize,
    x: usize,
    row: usize,
    height: usize,
    limit: usize,
    done: bool,
}

pub fn tokenize(src: &[u8], width: u32, height: u32, row_end: RowEnd) -> Tokens<'_> {
    Tokens {
        src,
        p: 0,
        x: 0,
        row: 0,
        height: height.as_usize(),
        limit: row_end.limit(width),
        done: false,
    }
}

impl<'a> Iterator for Tokens<'a> {
    type Item = Result<(usize, Token<'a>), Rle16Error>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.done || self.row >= self.height {
            return None;
        }
        let Some(&t) = self.src.get(self.p) else {
            self.done = true;
            return Some(Err(Rle16Error::StreamExhausted {
                row: self.row,
                at: self.p,
            }));
        };
        let row = self.row;
        let tok = if t & 0x80 != 0 {
            let n = t - 0x80;
            self.x += n.as_usize();
            self.p += 1;
            Token::Skip(n)
        } else {
            let n = t.as_usize();
            let Some(lit) = self.src.get(self.p + 1..self.p + 1 + n) else {
                self.done = true;
                return Some(Err(Rle16Error::StreamExhausted {
                    row: self.row,
                    at: self.p,
                }));
            };
            self.x += n;
            self.p += 1 + n;
            Token::Literal(lit)
        };
        if self.x >= self.limit {
            self.row += 1;
            self.x = 0;
        }
        Some(Ok((row, tok)))
    }
}

impl Tokens<'_> {
    /// Bytes of the source stream consumed so far.
    pub fn consumed(&self) -> usize {
        self.p
    }
}

fn widen_tokens(
    src: &[u8],
    width: u32,
    height: u32,
    table: &[u16; 256],
    row_end: RowEnd,
    sink: &mut Sink<'_>,
) -> Result<(), Rle16Error> {
    for item in tokenize(src, width, height, row_end) {
        let (_, tok) = item?;
        let ok = match tok {
            Token::Skip(n) => sink.push(0x80 | n),
            Token::Literal(lit) => {
                // The literal length came from a single token byte, so it fits.
                let mut ok = sink.push(u8::try_from(lit.len()).unwrap_or(u8::MAX));
                for &idx in lit {
                    ok = ok && sink.extend(&table[idx.as_usize()].to_le_bytes());
                }
                ok
            }
        };
        if !ok {
            return Err(Rle16Error::OutputFull {
                need: 0,
                have: sink.len(),
            });
        }
    }
    Ok(())
}

/// The byte length `EncodeRle16`'s sizing pass computes (0x149655..0x1496a0):
/// one byte per skip token, `1 + 2n` per literal run.
pub fn widened_len(
    src: &[u8],
    width: u32,
    height: u32,
    row_end: RowEnd,
) -> Result<usize, Rle16Error> {
    let mut s = Sink::Count(0);
    widen_tokens(src, width, height, &[0u16; 256], row_end, &mut s)?;
    Ok(s.len())
}

/// Widen an 8bpp shade stream to 16bpp using `table`.
///
/// Exactly `EncodeRle16`'s second pass (0x1496c5..0x149763): the token bytes
/// are copied verbatim and only the literal payload widens, so the output token
/// stream is structurally identical to the input.
pub fn widen_into(
    src: &[u8],
    width: u32,
    height: u32,
    table: &[u16; 256],
    row_end: RowEnd,
    dst: &mut [u8],
) -> Result<usize, Rle16Error> {
    let have = dst.len();
    let mut s = Sink::Write { buf: dst, at: 0 };
    match widen_tokens(src, width, height, table, row_end, &mut s) {
        Ok(()) => Ok(s.len()),
        Err(Rle16Error::OutputFull { .. }) => Err(Rle16Error::OutputFull {
            need: widened_len(src, width, height, row_end)?,
            have,
        }),
        Err(e) => Err(e),
    }
}

/// Rebuild the 256-entry 16bpp lookup table from a `PALETTEENTRY[256]`
/// (`peRed, peGreen, peBlue, peFlags`) and the display's channel shifts.
///
/// Transcribed from `EncodeRle16`'s first pass (0x1495e8..0x149634):
/// `(g >> g_down) << g_up | (r >> r_down) << r_up | (b >> b_down)`. Note there
/// is no `<< b_up`: blue is the low channel and its up-shift is zero by
/// construction, which is why retail omits the shift entirely.
pub fn build_table(
    palette: &[u8; 1024],
    r_up: u32,
    g_up: u32,
    r_down: u32,
    g_down: u32,
    b_down: u32,
) -> [u16; 256] {
    let mut t = [0u16; 256];
    for (i, slot) in t.iter_mut().enumerate() {
        let (r, g, b) = (palette[i * 4], palette[i * 4 + 1], palette[i * 4 + 2]);
        let packed = (u32::from(g >> g_down) << g_up)
            | (u32::from(r >> r_down) << r_up)
            | u32::from(b >> b_down);
        // Retail stores the result with `mov WORD PTR [..],dx` - the low word.
        *slot = u16::from_le_bytes([packed.to_le_bytes()[0], packed.to_le_bytes()[1]]);
    }
    t
}
