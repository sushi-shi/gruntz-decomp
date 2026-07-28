//! **BMP** — the Windows DIB the engine reads (`CDDSurface::DecodeBmp`
//! @0x143fc0) and writes (`CDDSurface::SaveRle16` @0x144640).
//!
//! The reader covers what the differential runner needs: DIB geometry, so a
//! `.BMP` on disk can be compared against a decoded sprite. The writer exists
//! so a *disagreeing* sprite can be dumped and looked at, which is the point of
//! an oracle.
//!
//! `SaveRle16` writes a 24bpp bottom-up DIB, no compression, rows padded to 4
//! bytes: `BITMAPFILEHEADER { "BM", size, 0, 0, 0x36 }` then
//! `BITMAPINFOHEADER { 40, w, h, 1, 24, 0, ... }`, pixels stored **BGR**.

use core::fmt;

use gruntz_cast::AsUsize;

pub const FILE_HEADER_SIZE: usize = 14;
pub const INFO_HEADER_SIZE: usize = 40;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Dib {
    pub width: i32,
    /// Negative means top-down storage.
    pub height: i32,
    pub bit_count: u16,
    pub compression: u32,
    pub data_offset: usize,
    pub palette_entries: usize,
}

impl Dib {
    /// `width * bit_count / 8` rounded up to a 4-byte boundary — the same rule
    /// `CRezImage::DecodeBmpHeader` @0x1757f9 applies to its DIB section.
    pub fn stride(self) -> usize {
        (self.width.unsigned_abs().as_usize() * self.bit_count.as_usize()).div_ceil(32) * 4
    }
    pub fn rows(self) -> usize {
        self.height.unsigned_abs().as_usize()
    }
    pub fn top_down(self) -> bool {
        self.height < 0
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BmpError {
    Truncated { need: usize, have: usize },
    NotBmp,
    UnsupportedHeader(u32),
    Compressed(u32),
    OutputFull { need: usize, have: usize },
}

impl fmt::Display for BmpError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            BmpError::Truncated { need, have } => write!(f, "truncated: need {need}, have {have}"),
            BmpError::NotBmp => write!(f, "no 'BM' signature"),
            BmpError::UnsupportedHeader(n) => write!(f, "info header size {n} (expected 40)"),
            BmpError::Compressed(c) => write!(f, "BI_ compression {c} is not supported"),
            BmpError::OutputFull { need, have } => {
                write!(f, "output buffer holds {have}, need {need}")
            }
        }
    }
}

impl core::error::Error for BmpError {}

pub fn parse(b: &[u8]) -> Result<Dib, BmpError> {
    let need = FILE_HEADER_SIZE + INFO_HEADER_SIZE;
    if b.len() < need {
        return Err(BmpError::Truncated {
            need,
            have: b.len(),
        });
    }
    if &b[0..2] != b"BM" {
        return Err(BmpError::NotBmp);
    }
    let u32at = |i: usize| u32::from_le_bytes([b[i], b[i + 1], b[i + 2], b[i + 3]]);
    let u16at = |i: usize| u16::from_le_bytes([b[i], b[i + 1]]);
    let hdr = u32at(14);
    if usize::try_from(hdr) != Ok(INFO_HEADER_SIZE) {
        return Err(BmpError::UnsupportedHeader(hdr));
    }
    let compression = u32at(30);
    if compression != 0 {
        return Err(BmpError::Compressed(compression));
    }
    let bit_count = u16at(28);
    let mut used = u32at(46).as_usize();
    if used == 0 && bit_count <= 8 {
        used = 1usize << bit_count;
    }
    Ok(Dib {
        width: i32::from_le_bytes([b[18], b[19], b[20], b[21]]),
        height: i32::from_le_bytes([b[22], b[23], b[24], b[25]]),
        bit_count,
        compression,
        data_offset: u32at(10).as_usize(),
        palette_entries: used,
    })
}

/// Bytes [`write_indexed_into`] needs.
pub fn indexed_len(width: usize, height: usize) -> usize {
    FILE_HEADER_SIZE + INFO_HEADER_SIZE + 256 * 4 + ((width + 3) & !3) * height
}

/// Write an 8bpp indexed BMP (bottom-up, 4-byte padded rows) from **top-down**
/// pixels plus an optional 256 x RGB palette. A missing palette becomes a grey
/// ramp so the dump is still readable.
pub fn write_indexed_into(
    pixels: &[u8],
    width: usize,
    height: usize,
    palette: Option<&[u8]>,
    dst: &mut [u8],
) -> Result<usize, BmpError> {
    let total = indexed_len(width, height);
    if dst.len() < total {
        return Err(BmpError::OutputFull {
            need: total,
            have: dst.len(),
        });
    }
    let stride = (width + 3) & !3;
    let data_off = FILE_HEADER_SIZE + INFO_HEADER_SIZE + 256 * 4;
    let out = &mut dst[..total];
    out.fill(0);
    out[0..2].copy_from_slice(b"BM");
    let put32 = |o: &mut [u8], at: usize, v: u32| o[at..at + 4].copy_from_slice(&v.to_le_bytes());
    let u32of = |v: usize| u32::try_from(v).map_err(|_| BmpError::OutputFull { need: v, have: 0 });
    put32(out, 2, u32of(total)?);
    put32(out, 10, u32of(data_off)?);
    put32(out, 14, u32of(INFO_HEADER_SIZE)?);
    put32(out, 18, u32of(width)?);
    put32(out, 22, u32of(height)?);
    put32(out, 34, u32of(stride * height)?);
    put32(out, 46, 256);
    out[26..28].copy_from_slice(&1u16.to_le_bytes()); // planes
    out[28..30].copy_from_slice(&8u16.to_le_bytes()); // bit count
    for i in 0..256 {
        let (r, g, b) = match palette {
            Some(p) if p.len() >= (i + 1) * 3 => (p[i * 3], p[i * 3 + 1], p[i * 3 + 2]),
            // No embedded palette: a grey ramp keeps the dump readable.
            // `i` is bounded by the 0..256 loop, so the byte is exact.
            _ => {
                let g = u8::try_from(i).unwrap_or(u8::MAX);
                (g, g, g)
            }
        };
        let at = FILE_HEADER_SIZE + INFO_HEADER_SIZE + i * 4;
        out[at] = b;
        out[at + 1] = g;
        out[at + 2] = r;
    }
    for y in 0..height {
        let src = &pixels[y * width..(y + 1) * width];
        let at = data_off + (height - 1 - y) * stride;
        out[at..at + width].copy_from_slice(src);
    }
    Ok(total)
}
