//! Monolith **FEC 1.1** movie archive reader — zero-copy and allocation-free.
//!
//! Retail `CFecFile::ReadArchive` @0x17b5f0 walks an interleaved stream of
//! 0x10c-byte entry headers, random padding, and payload bytes. Entry names are
//! obscured by alternating `+0x4f`, `+0x53` bytes (`FecDecode` @0x17bfe0).

use core::fmt;

pub const MAGIC: &[u8; 3] = b"FEC";
pub const FILE_HEADER_SIZE: usize = 15;
pub const ENTRY_HEADER_SIZE: usize = 0x10c;
pub const NAME_CAPACITY: usize = 0x100;
pub const SCRAMBLE_BASE: u16 = 0x2b8;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct FecHeader {
    pub version_major: i32,
    pub version_minor: i32,
    pub file_count: usize,
}

#[derive(Debug, Clone, Copy)]
pub struct Fec<'a> {
    bytes: &'a [u8],
    pub header: FecHeader,
}

impl<'a> Fec<'a> {
    pub fn new(bytes: &'a [u8]) -> Result<Self, FecError> {
        if bytes.len() < FILE_HEADER_SIZE {
            return Err(FecError::Truncated {
                at: 0,
                need: FILE_HEADER_SIZE,
                have: bytes.len(),
            });
        }
        if bytes.get(..MAGIC.len()) != Some(MAGIC) {
            return Err(FecError::BadMagic);
        }
        let count = read_i32(bytes, 11).ok_or(FecError::Truncated {
            at: 11,
            need: 4,
            have: bytes.len().saturating_sub(11),
        })?;
        let file_count = usize::try_from(count).map_err(|_| FecError::NegativeFileCount(count))?;
        Ok(Self {
            bytes,
            header: FecHeader {
                version_major: read_i32(bytes, 3).unwrap_or_default(),
                version_minor: read_i32(bytes, 7).unwrap_or_default(),
                file_count,
            },
        })
    }

    pub fn bytes(self) -> &'a [u8] {
        self.bytes
    }

    pub fn entries(self) -> Entries<'a> {
        Entries {
            bytes: self.bytes,
            at: FILE_HEADER_SIZE,
            remaining: self.header.file_count,
            failed: false,
            checked_end: false,
        }
    }

    pub fn validate(self) -> Result<usize, FecError> {
        let mut count = 0usize;
        for entry in self.entries() {
            entry?;
            count += 1;
        }
        Ok(count)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct FecEntry<'a> {
    pub index: i32,
    pub name_encoded: &'a [u8],
    pub scramble: u16,
    pub payload_offset: usize,
    pub payload: &'a [u8],
}

impl FecEntry<'_> {
    pub fn decoded_name(self, out: &mut [u8; NAME_CAPACITY]) -> Result<&str, FecError> {
        for (index, (&encoded, decoded)) in self.name_encoded.iter().zip(out.iter_mut()).enumerate()
        {
            let delta = if index % 2 == 0 { 0x4f } else { 0x53 };
            *decoded = encoded.wrapping_sub(delta);
        }
        core::str::from_utf8(&out[..self.name_encoded.len()]).map_err(|_| FecError::NonUtf8Name)
    }
}

#[derive(Debug, Clone, Copy)]
pub struct Entries<'a> {
    bytes: &'a [u8],
    at: usize,
    remaining: usize,
    failed: bool,
    checked_end: bool,
}

impl<'a> Entries<'a> {
    fn fail(&mut self, error: FecError) -> Option<Result<FecEntry<'a>, FecError>> {
        self.failed = true;
        Some(Err(error))
    }
}

impl<'a> Iterator for Entries<'a> {
    type Item = Result<FecEntry<'a>, FecError>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.failed {
            return None;
        }
        if self.remaining == 0 {
            if self.checked_end {
                return None;
            }
            self.checked_end = true;
            if self.at != self.bytes.len() {
                return self.fail(FecError::TrailingBytes(
                    self.bytes.len().saturating_sub(self.at),
                ));
            }
            return None;
        }

        let entry_at = self.at;
        let Some(header_end) = entry_at.checked_add(ENTRY_HEADER_SIZE) else {
            return self.fail(FecError::RangeOverflow { at: entry_at });
        };
        if header_end > self.bytes.len() {
            return self.fail(FecError::Truncated {
                at: entry_at,
                need: ENTRY_HEADER_SIZE,
                have: self.bytes.len().saturating_sub(entry_at),
            });
        }
        let index = read_i32(self.bytes, entry_at).unwrap_or_default();
        let name_len = usize::from(read_u16(self.bytes, entry_at + 4).unwrap_or_default());
        if name_len > NAME_CAPACITY {
            return self.fail(FecError::NameTooLong(name_len));
        }
        let name_at = entry_at + 6;
        let name_encoded = &self.bytes[name_at..name_at + name_len];
        let scramble = read_u16(self.bytes, entry_at + 0x106).unwrap_or_default();
        let Some(padding) = scramble.checked_sub(SCRAMBLE_BASE).map(usize::from) else {
            return self.fail(FecError::BadScramble(scramble));
        };
        let payload_len = read_i32(self.bytes, entry_at + 0x108).unwrap_or_default();
        let Ok(payload_len) = usize::try_from(payload_len) else {
            return self.fail(FecError::NegativePayloadLength(payload_len));
        };
        let Some(payload_at) = header_end.checked_add(padding) else {
            return self.fail(FecError::RangeOverflow { at: header_end });
        };
        let Some(payload_end) = payload_at.checked_add(payload_len) else {
            return self.fail(FecError::RangeOverflow { at: payload_at });
        };
        let Some(payload) = self.bytes.get(payload_at..payload_end) else {
            return self.fail(FecError::Truncated {
                at: payload_at,
                need: payload_len,
                have: self.bytes.len().saturating_sub(payload_at),
            });
        };
        self.at = payload_end;
        self.remaining -= 1;
        Some(Ok(FecEntry {
            index,
            name_encoded,
            scramble,
            payload_offset: payload_at,
            payload,
        }))
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FecError {
    BadMagic,
    Truncated { at: usize, need: usize, have: usize },
    NegativeFileCount(i32),
    NameTooLong(usize),
    NonUtf8Name,
    BadScramble(u16),
    NegativePayloadLength(i32),
    RangeOverflow { at: usize },
    TrailingBytes(usize),
}

impl fmt::Display for FecError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            FecError::BadMagic => write!(f, "no 'FEC' signature"),
            FecError::Truncated { at, need, have } => {
                write!(f, "truncated at {at:#x}: need {need} bytes, have {have}")
            }
            FecError::NegativeFileCount(count) => write!(f, "negative file count {count}"),
            FecError::NameTooLong(len) => write!(f, "entry name length {len} exceeds 256"),
            FecError::NonUtf8Name => write!(f, "decoded entry name is not UTF-8"),
            FecError::BadScramble(value) => {
                write!(f, "scramble value {value:#x} is below {SCRAMBLE_BASE:#x}")
            }
            FecError::NegativePayloadLength(len) => write!(f, "negative payload length {len}"),
            FecError::RangeOverflow { at } => write!(f, "entry range overflows at {at:#x}"),
            FecError::TrailingBytes(count) => write!(f, "{count} trailing byte(s) after entries"),
        }
    }
}

impl core::error::Error for FecError {}

fn read_i32(bytes: &[u8], at: usize) -> Option<i32> {
    Some(i32::from_le_bytes(
        bytes.get(at..at.checked_add(4)?)?.try_into().ok()?,
    ))
}

fn read_u16(bytes: &[u8], at: usize) -> Option<u16> {
    Some(u16::from_le_bytes(
        bytes.get(at..at.checked_add(2)?)?.try_into().ok()?,
    ))
}
