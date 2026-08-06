//! **RID** — raw indexed Gruntz images.
//!
//! Retail `CRezImage::DecodeRidData` @0x1762c0 reads the same 0x20-byte header
//! used by PID, then blits exactly `width * height` uncompressed 8-bit pixels.
//! RID carries no palette of its own.

use core::fmt;

use crate::pid::{Dims, PidError, PidHeader, HEADER_SIZE};

#[derive(Debug, Clone, Copy)]
pub struct Rid<'a> {
    pub header: PidHeader,
    pub dims: Dims,
    pub pixels: &'a [u8],
    pub trailing: &'a [u8],
}

impl Rid<'_> {
    pub fn decode_into(&self, dst: &mut [u8]) -> Result<usize, RidError> {
        if dst.len() != self.pixels.len() {
            return Err(RidError::BadDestination {
                need: self.pixels.len(),
                have: dst.len(),
            });
        }
        dst.copy_from_slice(self.pixels);
        Ok(self.pixels.len())
    }

    pub fn encoded_len(&self) -> usize {
        HEADER_SIZE + self.pixels.len() + self.trailing.len()
    }

    pub fn encode_into(&self, dst: &mut [u8]) -> Result<usize, RidError> {
        let need = self.encoded_len();
        if dst.len() < need {
            return Err(RidError::OutputFull {
                need,
                have: dst.len(),
            });
        }
        let words = [
            self.header.file_desc,
            self.header.flags,
            u32::from_le_bytes(self.header.width.to_le_bytes()),
            u32::from_le_bytes(self.header.height.to_le_bytes()),
            u32::from_le_bytes(self.header.offset_x.to_le_bytes()),
            u32::from_le_bytes(self.header.offset_y.to_le_bytes()),
            self.header.fill,
            self.header.unk1,
        ];
        for (i, word) in words.into_iter().enumerate() {
            let at = i * 4;
            dst[at..at + 4].copy_from_slice(&word.to_le_bytes());
        }
        let mut at = HEADER_SIZE;
        dst[at..at + self.pixels.len()].copy_from_slice(self.pixels);
        at += self.pixels.len();
        dst[at..at + self.trailing.len()].copy_from_slice(self.trailing);
        at += self.trailing.len();
        Ok(at)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RidError {
    Header(PidError),
    TruncatedPixels { need: usize, have: usize },
    BadDestination { need: usize, have: usize },
    OutputFull { need: usize, have: usize },
}

impl fmt::Display for RidError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            RidError::Header(e) => write!(f, "bad RID header: {e}"),
            RidError::TruncatedPixels { need, have } => {
                write!(f, "RID has {have} pixel byte(s), need {need}")
            }
            RidError::BadDestination { need, have } => {
                write!(f, "destination is {have} bytes, need exactly {need}")
            }
            RidError::OutputFull { need, have } => {
                write!(f, "output buffer holds {have}, need {need}")
            }
        }
    }
}

impl core::error::Error for RidError {}

pub fn split(resource: &[u8]) -> Result<Rid<'_>, RidError> {
    let header = PidHeader::parse(resource).map_err(RidError::Header)?;
    let dims = header.dims().map_err(RidError::Header)?;
    let need = dims.pixel_len();
    let body = resource.get(HEADER_SIZE..).unwrap_or_default();
    if body.len() < need {
        return Err(RidError::TruncatedPixels {
            need,
            have: body.len(),
        });
    }
    Ok(Rid {
        header,
        dims,
        pixels: &body[..need],
        trailing: &body[need..],
    })
}
