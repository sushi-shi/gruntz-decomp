//! **PCX** — the full-screen images (`SCREENZ`, splash art).
//!
//! Transcribed from `CRezImage::DecodePcxData` @0x176000. Retail accepts
//! `bits_per_pixel == 8` only (0x17602a `cmp cl,8 / jne <fail>`) with
//! `n_planes` of 1 (paletted) or 3 (RGB planes).
//!
//! ```text
//! +0x00 u8  manufacturer   (0x0a)
//! +0x01 u8  version
//! +0x02 u8  encoding
//! +0x03 u8  bits_per_pixel  must be 8   (0x17602a)
//! +0x04 i16 xmin
//! +0x06 i16 ymin
//! +0x08 i16 xmax
//! +0x0a i16 ymax           width = xmax-xmin+1, height = ymax-ymin+1  (0x176019)
//! +0x41 u8  n_planes       (0x176043)
//! +0x42 u16 bytes_per_line
//! +0x80 ... RLE stream     (0x176070 `lea eax,[esi+0x80]`)
//! ```
//!
//! The RLE is the same `0xC0`-flagged grammar as [`crate::pid::Grammar::Rle`],
//! but decoded **per scanline of `width * n_planes` bytes** rather than per
//! `width` pixels, and retail *tolerates* a zero-count run here (0x1760fc
//! `jle` skips the store without decrementing the counter) where the PID
//! decoder would spin forever.
//!
//! Planes are de-interleaved on output: `dst[3i+0] = plane2`, `dst[3i+1] =
//! plane1`, `dst[3i+2] = plane0` (0x176144) — PCX R/G/B planes land as
//! Windows-DIB **BGR**.

use core::fmt;

use gruntz_cast::{AsI64, AsUsize};

use crate::pid::{Dims, LiteralRule, MAX_PIXELS, MAX_RLE_RUN};
use crate::Sink;

pub const HEADER_SIZE: usize = 0x80;
pub const PALETTE_SIZE: usize = 768;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PcxHeader {
    pub manufacturer: u8,
    pub version: u8,
    pub encoding: u8,
    pub bits_per_pixel: u8,
    pub xmin: i16,
    pub ymin: i16,
    pub xmax: i16,
    pub ymax: i16,
    pub n_planes: u8,
    pub bytes_per_line: u16,
}

impl PcxHeader {
    pub fn width(self) -> i32 {
        i32::from(self.xmax) - i32::from(self.xmin) + 1
    }
    pub fn height(self) -> i32 {
        i32::from(self.ymax) - i32::from(self.ymin) + 1
    }
    /// `width * n_planes` — one decoded scanline, all planes.
    pub fn row_bytes(self) -> i32 {
        self.width() * i32::from(self.n_planes)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum PcxError {
    Truncated {
        need: usize,
        have: usize,
    },
    /// Retail hard-requires 8 (0x17602a).
    UnsupportedDepth(u8),
    UnsupportedPlanes(u8),
    BadDimensions {
        width: i32,
        height: i32,
    },
    BadDestination {
        need: usize,
        have: usize,
    },
    /// The caller's de-interleave scratch is not `width * n_planes`.
    BadScratch {
        need: usize,
        have: usize,
    },
    StreamExhausted {
        row: usize,
        at: usize,
    },
    OutputFull {
        need: usize,
        have: usize,
    },
}

impl fmt::Display for PcxError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            PcxError::Truncated { need, have } => write!(f, "truncated: need {need}, have {have}"),
            PcxError::UnsupportedDepth(d) => write!(f, "bits_per_pixel {d} (retail requires 8)"),
            PcxError::UnsupportedPlanes(p) => write!(f, "n_planes {p} (retail handles 1 and 3)"),
            PcxError::BadDimensions { width, height } => {
                write!(f, "bad dimensions {width}x{height}")
            }
            PcxError::BadDestination { need, have } => {
                write!(f, "destination is {have} bytes, need exactly {need}")
            }
            PcxError::BadScratch { need, have } => {
                write!(f, "scratch is {have} bytes, need exactly {need}")
            }
            PcxError::StreamExhausted { row, at } => {
                write!(f, "stream exhausted at byte {at} on row {row}")
            }
            PcxError::OutputFull { need, have } => {
                write!(f, "output buffer holds {have}, need {need}")
            }
        }
    }
}

impl core::error::Error for PcxError {}

/// A PCX resource split into borrowed parts, with its geometry validated.
#[derive(Debug, Clone, Copy)]
pub struct Pcx<'a> {
    pub header: PcxHeader,
    /// `width * n_planes` by `height` — the shape of the *decoded scanlines*,
    /// so `dims.width()` is a byte count, not a pixel count.
    pub dims: Dims,
    pub planes: usize,
    pub stream: &'a [u8],
    /// The trailing VGA palette, if the `0x0c` marker is present at EOF-769.
    pub palette: Option<&'a [u8]>,
}

pub fn parse_header(b: &[u8]) -> Result<PcxHeader, PcxError> {
    let h: &[u8; HEADER_SIZE] =
        b.get(..HEADER_SIZE)
            .and_then(|s| s.try_into().ok())
            .ok_or(PcxError::Truncated {
                need: HEADER_SIZE,
                have: b.len(),
            })?;
    let i16at = |i: usize| i16::from_le_bytes([h[i], h[i + 1]]);
    Ok(PcxHeader {
        manufacturer: h[0],
        version: h[1],
        encoding: h[2],
        bits_per_pixel: h[3],
        xmin: i16at(4),
        ymin: i16at(6),
        xmax: i16at(8),
        ymax: i16at(10),
        n_planes: h[0x41],
        bytes_per_line: u16::from_le_bytes([h[0x42], h[0x43]]),
    })
}

pub fn split(resource: &[u8]) -> Result<Pcx<'_>, PcxError> {
    let header = parse_header(resource)?;
    if header.bits_per_pixel != 8 {
        return Err(PcxError::UnsupportedDepth(header.bits_per_pixel));
    }
    if header.n_planes != 1 && header.n_planes != 3 {
        return Err(PcxError::UnsupportedPlanes(header.n_planes));
    }
    let (w, h) = (header.width(), header.height());
    let bad = PcxError::BadDimensions {
        width: w,
        height: h,
    };
    if w <= 0 || h <= 0 || w.as_i64() * h.as_i64() * i64::from(header.n_planes) > MAX_PIXELS {
        return Err(bad);
    }
    let dims = Dims::new(header.row_bytes(), h).map_err(|_| bad)?;

    // The trailing palette is the standard PCX convention (0x0c marker then 768
    // bytes at EOF). Retail's `CRezImage` path never reads it - it builds a
    // DIB_PAL_COLORS index table instead - so treat it as advisory.
    //
    // It applies to PALETTED images only. Testing the marker byte on a 3-plane
    // (24bpp) image is not merely useless, it is actively wrong: in
    // `STATEZ\ATTRACT\SCREENZ\TITLE3` the byte 769 from the end happens to be
    // 0x0c as part of the pixel stream, and stripping "the palette" left the
    // decode seven scanlines short. Gating on `n_planes == 1` is the fix, and
    // that sprite is the regression case.
    let (stream, palette) = if header.n_planes == 1
        && resource.len() > HEADER_SIZE + PALETTE_SIZE
        && resource[resource.len() - PALETTE_SIZE - 1] == 0x0c
    {
        let cut = resource.len() - PALETTE_SIZE - 1;
        (&resource[HEADER_SIZE..cut], Some(&resource[cut + 1..]))
    } else {
        (&resource[HEADER_SIZE..], None)
    };
    Ok(Pcx {
        header,
        dims,
        planes: header.n_planes.as_usize(),
        stream,
        palette,
    })
}

impl Pcx<'_> {
    /// Bytes the caller must hand to [`Pcx::decode_into`] as `dst`.
    pub fn pixel_len(&self) -> usize {
        self.dims.pixel_len()
    }

    /// Bytes the caller must hand to [`Pcx::decode_into`] as `scratch`.
    pub fn scratch_len(&self) -> usize {
        self.dims.width()
    }

    /// Decode into `dst` (`width * n_planes * height`) using `scratch`
    /// (`width * n_planes`). Returns the stream bytes consumed.
    pub fn decode_into(&self, dst: &mut [u8], scratch: &mut [u8]) -> Result<usize, PcxError> {
        let row_bytes = self.dims.width();
        if dst.len() != self.dims.pixel_len() {
            return Err(PcxError::BadDestination {
                need: self.dims.pixel_len(),
                have: dst.len(),
            });
        }
        if scratch.len() != row_bytes {
            return Err(PcxError::BadScratch {
                need: row_bytes,
                have: scratch.len(),
            });
        }
        let w = row_bytes / self.planes;
        let mut p = 0usize;
        for y in 0..self.dims.height() {
            // retail decodes one whole `width * n_planes` scanline per iteration
            let mut filled = 0usize;
            while filled < row_bytes {
                let tok = *self
                    .stream
                    .get(p)
                    .ok_or(PcxError::StreamExhausted { row: y, at: p })?;
                p += 1;
                if tok & 0xc0 == 0xc0 {
                    let val = *self
                        .stream
                        .get(p)
                        .ok_or(PcxError::StreamExhausted { row: y, at: p })?;
                    p += 1;
                    // 0x1760fc: a zero count stores nothing and does NOT loop.
                    let n = (tok & MAX_RLE_RUN).as_usize().min(row_bytes - filled);
                    scratch[filled..filled + n].fill(val);
                    filled += n;
                } else {
                    scratch[filled] = tok;
                    filled += 1;
                }
            }
            let row = &mut dst[y * row_bytes..(y + 1) * row_bytes];
            if self.planes == 1 {
                row.copy_from_slice(scratch);
            } else {
                // 0x176144: plane 0 -> byte 2, plane 1 -> byte 1, plane 2 -> byte 0.
                for i in 0..w {
                    row[3 * i] = scratch[2 * w + i];
                    row[3 * i + 1] = scratch[w + i];
                    row[3 * i + 2] = scratch[i];
                }
            }
        }
        Ok(p)
    }
}

/// Encode scanline-oriented bytes with the PCX RLE. Canonical greedy form, the
/// same choice the PID encoder makes (see [`crate::pid::encode_rle_into`]).
fn pcx_tokens(rows: &[u8], dims: Dims, rule: LiteralRule, sink: &mut Sink<'_>) -> bool {
    let row_bytes = dims.width();
    for y in 0..dims.height() {
        let row = &rows[y * row_bytes..(y + 1) * row_bytes];
        let mut i = 0usize;
        while i < row.len() {
            let v = row[i];
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

pub fn encoded_len(rows: &[u8], dims: Dims, rule: LiteralRule) -> usize {
    let mut s = Sink::Count(0);
    pcx_tokens(rows, dims, rule, &mut s);
    s.len()
}

pub fn encode_into(
    rows: &[u8],
    dims: Dims,
    rule: LiteralRule,
    dst: &mut [u8],
) -> Result<usize, PcxError> {
    let have = dst.len();
    let mut s = Sink::Write { buf: dst, at: 0 };
    if !pcx_tokens(rows, dims, rule, &mut s) {
        return Err(PcxError::OutputFull {
            need: encoded_len(rows, dims, rule),
            have,
        });
    }
    Ok(s.len())
}
