//! **FNT** — the four bitmap fonts shipped beside `GRUNTZ.EXE`.
//!
//! Retail `Font::LoadFont` @0x179830 reads a little-endian signed glyph count,
//! followed by that many records. Each record is an `i32 width`, `i32 height`,
//! then exactly `width * height` bytes of intensity data. The shipped fonts
//! contain 256 glyphs and consume the file exactly under this layout.

use core::fmt;

pub const HEADER_SIZE: usize = 4;
pub const GLYPH_HEADER_SIZE: usize = 8;

#[derive(Debug, Clone, Copy)]
pub struct Font<'a> {
    bytes: &'a [u8],
    count: usize,
}

impl<'a> Font<'a> {
    pub fn count(self) -> usize {
        self.count
    }

    pub fn glyphs(self) -> Glyphs<'a> {
        Glyphs {
            bytes: self.bytes,
            at: HEADER_SIZE,
            index: 0,
            remaining: self.count,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Glyph<'a> {
    pub index: usize,
    pub width: usize,
    pub height: usize,
    pub pixels: &'a [u8],
}

#[derive(Debug, Clone, Copy)]
pub struct Glyphs<'a> {
    bytes: &'a [u8],
    at: usize,
    index: usize,
    remaining: usize,
}

impl<'a> Iterator for Glyphs<'a> {
    type Item = Glyph<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.remaining == 0 {
            return None;
        }
        let width = read_i32(self.bytes, self.at)?;
        let height = read_i32(self.bytes, self.at + 4)?;
        let width = usize::try_from(width).ok()?;
        let height = usize::try_from(height).ok()?;
        let len = width.checked_mul(height)?;
        let pixels_at = self.at.checked_add(GLYPH_HEADER_SIZE)?;
        let pixels = self.bytes.get(pixels_at..pixels_at.checked_add(len)?)?;
        let glyph = Glyph {
            index: self.index,
            width,
            height,
            pixels,
        };
        self.at = pixels_at + len;
        self.index += 1;
        self.remaining -= 1;
        Some(glyph)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FntError {
    Truncated {
        glyph: usize,
        need: usize,
        have: usize,
    },
    NegativeCount(i32),
    NegativeDimensions {
        glyph: usize,
        width: i32,
        height: i32,
    },
    DimensionsOverflow {
        glyph: usize,
        width: i32,
        height: i32,
    },
    TrailingBytes(usize),
}

impl fmt::Display for FntError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            FntError::Truncated { glyph, need, have } => {
                write!(f, "glyph {glyph}: need {need} bytes, have {have}")
            }
            FntError::NegativeCount(count) => write!(f, "negative glyph count {count}"),
            FntError::NegativeDimensions {
                glyph,
                width,
                height,
            } => write!(f, "glyph {glyph}: negative dimensions {width} x {height}"),
            FntError::DimensionsOverflow {
                glyph,
                width,
                height,
            } => write!(f, "glyph {glyph}: dimensions {width} x {height} overflow"),
            FntError::TrailingBytes(count) => write!(f, "{count} trailing byte(s) after glyphs"),
        }
    }
}

impl core::error::Error for FntError {}

pub fn split(bytes: &[u8]) -> Result<Font<'_>, FntError> {
    let count = read_i32(bytes, 0).ok_or(FntError::Truncated {
        glyph: 0,
        need: HEADER_SIZE,
        have: bytes.len(),
    })?;
    let count = usize::try_from(count).map_err(|_| FntError::NegativeCount(count))?;
    let mut at = HEADER_SIZE;
    for glyph in 0..count {
        let header_end = at
            .checked_add(GLYPH_HEADER_SIZE)
            .ok_or(FntError::DimensionsOverflow {
                glyph,
                width: 0,
                height: 0,
            })?;
        if header_end > bytes.len() {
            return Err(FntError::Truncated {
                glyph,
                need: header_end,
                have: bytes.len(),
            });
        }
        let width = read_i32(bytes, at).unwrap_or_default();
        let height = read_i32(bytes, at + 4).unwrap_or_default();
        if width < 0 || height < 0 {
            return Err(FntError::NegativeDimensions {
                glyph,
                width,
                height,
            });
        }
        let len = usize::try_from(width)
            .ok()
            .and_then(|width| {
                usize::try_from(height)
                    .ok()
                    .and_then(|height| width.checked_mul(height))
            })
            .ok_or(FntError::DimensionsOverflow {
                glyph,
                width,
                height,
            })?;
        at = header_end
            .checked_add(len)
            .ok_or(FntError::DimensionsOverflow {
                glyph,
                width,
                height,
            })?;
        if at > bytes.len() {
            return Err(FntError::Truncated {
                glyph,
                need: at,
                have: bytes.len(),
            });
        }
    }
    if at != bytes.len() {
        return Err(FntError::TrailingBytes(bytes.len() - at));
    }
    Ok(Font { bytes, count })
}

fn read_i32(bytes: &[u8], at: usize) -> Option<i32> {
    Some(i32::from_le_bytes(
        bytes.get(at..at.checked_add(4)?)?.try_into().ok()?,
    ))
}
