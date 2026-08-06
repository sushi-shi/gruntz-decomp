//! **PAL** — the 256-entry RGB lookup tables used by Gruntz sprites.
//!
//! Retail `CShadeTableCache::AlphaTable` @0x149c86 reads 256 consecutive
//! red/green/blue triples and converts them to the active 16-bit display
//! format. `CSpriteRefTable::BuildToolToyColorTable` @0x082003 loads the
//! colour-specific TOOL and TOY tables used to tint Grunt sprites.

use core::fmt;

pub const ENTRY_COUNT: usize = 256;
pub const BYTE_LEN: usize = ENTRY_COUNT * 3;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Palette<'a> {
    bytes: &'a [u8],
}

impl<'a> Palette<'a> {
    pub fn as_bytes(self) -> &'a [u8] {
        self.bytes
    }

    pub fn entry(self, index: usize) -> Option<[u8; 3]> {
        let at = index.checked_mul(3)?;
        let rgb = self.bytes.get(at..at + 3)?;
        Some([rgb[0], rgb[1], rgb[2]])
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct PalError {
    pub have: usize,
}

impl fmt::Display for PalError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "PAL is {} bytes; expected exactly {BYTE_LEN}", self.have)
    }
}

impl core::error::Error for PalError {}

pub fn split(resource: &[u8]) -> Result<Palette<'_>, PalError> {
    if resource.len() != BYTE_LEN {
        return Err(PalError {
            have: resource.len(),
        });
    }
    Ok(Palette { bytes: resource })
}
