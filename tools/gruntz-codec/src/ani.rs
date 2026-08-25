//! **ANI** — Gruntz animation-control resources.
//!
//! ANI contains no pixels.  It is a small program that advances the frame of
//! an image set already attached to a game object, applies position deltas and
//! optionally fires sound cues.  The retail parser is `CAniElement::Build`
//! @0x165460 and `CAniRecordView::Parse` @0x168c60.
//!
//! ```text
//! +0x00 u32       always 0x20; NEVER READ by retail        (see below)
//! +0x04 u32       always 0;    NEVER READ by retail
//! +0x08 i32       animation flags                          `[ebp-0x18]` @0x165477
//! +0x0c i32       record count                             `[ebx+0x0c]` @0x1654c7
//! +0x10 u32       animation-name byte length               `[ebx+0x10]` @0x165483
//! +0x14 [u8; 12]  always zero; NEVER READ by retail
//! +0x20 [u8; n]   animation name (not NUL-terminated on disk)
//! then `count` records:
//!   10 x i16      flags, step, loop, position, param, duration, draw,
//!                 delta-x, delta-y, reserved
//!   if flags&2    NUL-terminated whitespace-separated sound-cue names
//! ```
//!
//! The name offset is **hard-coded**, not derived: `CAniElement::Build` opens
//! with `mov ebx,ebp / add ebp,0x20` and reads the name from `ebp` directly.
//! Enumerating every header access in that function gives exactly four —
//! `[ebp-0x18]`, `[ebx+0x0c]`, `[ebx+0x10]` and `[ebp+0x00]` — so **20 of the
//! 32 header bytes are never read at all**.
//!
//! Those 20 bytes are also constant across the whole corpus: `+0x00` is 0x20
//! and `+0x04`/`+0x14..0x1f` are zero in all 660 retail and 378 demo ANI
//! resources. It is tempting to read `+0x00 == 0x20` as a header size, and the
//! authoring tool may well have meant it that way — but the sibling PID/RID
//! header, which is also 0x20 bytes, carries **10** in the same slot
//! (constant across 29 798 sprites), so the two cannot both be a size. The
//! honest statement is the one that is proven: the game ignores the field.

use core::fmt;

use gruntz_cast::AsUsize;

pub const HEADER_SIZE: usize = 0x20;
pub const RECORD_SIZE: usize = 0x14;
pub const FLAG_FRAME_COUNT: u16 = 0x01;
pub const FLAG_HAS_CUES: u16 = 0x02;
pub const FLAG_POSITIONAL_CUE: u16 = 0x04;
pub const FLAG_CULL_CUE_WHEN_NOT_DRAWN: u16 = 0x08;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AniHeader {
    /// `+0x00..0x08`. Proven **never read** by `CAniElement::Build`; constant
    /// `20 00 00 00 00 00 00 00` across all 1038 shipped ANI resources. Kept
    /// verbatim so a re-encode cannot silently drop it.
    pub prefix: [u8; 8],
    pub flags: i32,
    pub count: i32,
    pub name_len: u32,
    /// `+0x14..0x20`. Proven never read; all-zero across the corpus.
    pub reserved: [u8; 12],
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct AniRecord<'a> {
    pub flags: u16,
    pub step_mode: i16,
    pub loop_mode: i16,
    pub position_mode: i16,
    pub param: i16,
    pub duration: i16,
    pub draw_value: i16,
    pub delta_x: i16,
    pub delta_y: i16,
    pub reserved: u16,
    /// The raw NUL-free cue string.  Use [`AniRecord::cues`] for tokens.
    pub cue_text: Option<&'a [u8]>,
}

impl AniRecord<'_> {
    pub fn cues(&self) -> Cues<'_> {
        Cues {
            bytes: self.cue_text.unwrap_or_default(),
            at: 0,
        }
    }

    /// Retail converts frame-count durations to 22 ms per update in
    /// `CAniRecordView::GetDurationMs` @0x168e50. Time durations are already ms.
    pub fn duration_ms(self) -> u32 {
        if self.duration <= 0 {
            return 22;
        }
        let n = u32::try_from(self.duration).unwrap_or(1);
        if self.flags & FLAG_FRAME_COUNT != 0 {
            n.saturating_mul(22)
        } else {
            n
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub struct Cues<'a> {
    bytes: &'a [u8],
    at: usize,
}

impl<'a> Iterator for Cues<'a> {
    type Item = &'a [u8];

    fn next(&mut self) -> Option<Self::Item> {
        while self.at < self.bytes.len() && self.bytes[self.at] <= 0x21 {
            self.at += 1;
        }
        let start = self.at;
        while self.at < self.bytes.len() && self.bytes[self.at] > 0x21 {
            self.at += 1;
        }
        (start < self.at).then_some(&self.bytes[start..self.at])
    }
}

#[derive(Debug, Clone, Copy)]
pub struct Ani<'a> {
    pub header: AniHeader,
    pub name: &'a [u8],
    records_data: &'a [u8],
    pub trailing: &'a [u8],
}

impl<'a> Ani<'a> {
    pub fn records(&self) -> AniRecords<'a> {
        AniRecords {
            bytes: self.records_data,
            at: 0,
            remaining: usize::try_from(self.header.count).unwrap_or_default(),
        }
    }

    pub fn encoded_len(&self) -> usize {
        HEADER_SIZE + self.name.len() + self.records_data.len() + self.trailing.len()
    }

    /// Rebuild the resource from parsed fields. Header-reserved and trailing
    /// bytes are preserved because their semantics are not yet proven.
    pub fn encode_into(&self, dst: &mut [u8]) -> Result<usize, AniError> {
        let need = self.encoded_len();
        if dst.len() < need {
            return Err(AniError::OutputFull {
                need,
                have: dst.len(),
            });
        }
        dst[..8].copy_from_slice(&self.header.prefix);
        dst[8..12].copy_from_slice(&self.header.flags.to_le_bytes());
        dst[12..16].copy_from_slice(&self.header.count.to_le_bytes());
        dst[16..20].copy_from_slice(&self.header.name_len.to_le_bytes());
        dst[20..32].copy_from_slice(&self.header.reserved);
        let mut at = HEADER_SIZE;
        dst[at..at + self.name.len()].copy_from_slice(self.name);
        at += self.name.len();
        for record in self.records() {
            let fields = [
                i16::from_le_bytes(record.flags.to_le_bytes()),
                record.step_mode,
                record.loop_mode,
                record.position_mode,
                record.param,
                record.duration,
                record.draw_value,
                record.delta_x,
                record.delta_y,
                i16::from_le_bytes(record.reserved.to_le_bytes()),
            ];
            for field in fields {
                dst[at..at + 2].copy_from_slice(&field.to_le_bytes());
                at += 2;
            }
            if let Some(cues) = record.cue_text {
                dst[at..at + cues.len()].copy_from_slice(cues);
                at += cues.len();
                dst[at] = 0;
                at += 1;
            }
        }
        dst[at..at + self.trailing.len()].copy_from_slice(self.trailing);
        at += self.trailing.len();
        Ok(at)
    }
}

#[derive(Debug, Clone, Copy)]
pub struct AniRecords<'a> {
    bytes: &'a [u8],
    at: usize,
    remaining: usize,
}

impl<'a> Iterator for AniRecords<'a> {
    type Item = AniRecord<'a>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.remaining == 0 {
            return None;
        }
        let (record, next) = parse_record(self.bytes, self.at).ok()?;
        self.at = next;
        self.remaining -= 1;
        Some(record)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AniError {
    Truncated { at: usize, need: usize, have: usize },
    NegativeCount(i32),
    NameTooLong { len: u32, available: usize },
    UnterminatedCues { record: usize, at: usize },
    OutputFull { need: usize, have: usize },
}

impl fmt::Display for AniError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            AniError::Truncated { at, need, have } => {
                write!(f, "truncated at {at:#x}: need {need} byte(s), have {have}")
            }
            AniError::NegativeCount(n) => write!(f, "negative record count {n}"),
            AniError::NameTooLong { len, available } => {
                write!(f, "name length {len} exceeds {available} available byte(s)")
            }
            AniError::UnterminatedCues { record, at } => {
                write!(
                    f,
                    "record {record} cue string at {at:#x} is not NUL-terminated"
                )
            }
            AniError::OutputFull { need, have } => {
                write!(f, "output buffer holds {have}, need {need}")
            }
        }
    }
}

impl core::error::Error for AniError {}

pub fn split(resource: &[u8]) -> Result<Ani<'_>, AniError> {
    let header_bytes = resource.get(..HEADER_SIZE).ok_or(AniError::Truncated {
        at: 0,
        need: HEADER_SIZE,
        have: resource.len(),
    })?;
    let mut prefix = [0u8; 8];
    prefix.copy_from_slice(&header_bytes[..8]);
    let mut reserved = [0u8; 12];
    reserved.copy_from_slice(&header_bytes[20..32]);
    let i32_at =
        |at: usize| i32::from_le_bytes(header_bytes[at..at + 4].try_into().unwrap_or_default());
    let header = AniHeader {
        prefix,
        flags: i32_at(8),
        count: i32_at(12),
        name_len: u32::from_le_bytes(header_bytes[16..20].try_into().unwrap_or_default()),
        reserved,
    };
    if header.count < 0 {
        return Err(AniError::NegativeCount(header.count));
    }
    let name_len = header.name_len.as_usize();
    let available = resource.len() - HEADER_SIZE;
    if name_len > available {
        return Err(AniError::NameTooLong {
            len: header.name_len,
            available,
        });
    }
    let name_end = HEADER_SIZE + name_len;
    let mut at = name_end;
    for record in 0..usize::try_from(header.count).unwrap_or_default() {
        match parse_record(resource, at) {
            Ok((_, next)) => at = next,
            Err(AniError::UnterminatedCues { at, .. }) => {
                return Err(AniError::UnterminatedCues { record, at });
            }
            Err(e) => return Err(e),
        }
    }
    Ok(Ani {
        header,
        name: &resource[HEADER_SIZE..name_end],
        records_data: &resource[name_end..at],
        trailing: &resource[at..],
    })
}

fn parse_record(bytes: &[u8], at: usize) -> Result<(AniRecord<'_>, usize), AniError> {
    let fixed = bytes.get(at..at + RECORD_SIZE).ok_or(AniError::Truncated {
        at,
        need: RECORD_SIZE,
        have: bytes.len().saturating_sub(at),
    })?;
    let field = |n: usize| i16::from_le_bytes([fixed[n * 2], fixed[n * 2 + 1]]);
    let flags = u16::from_le_bytes(field(0).to_le_bytes());
    let mut next = at + RECORD_SIZE;
    let cue_text = if flags & FLAG_HAS_CUES != 0 {
        let tail = &bytes[next..];
        let Some(n) = tail.iter().position(|&b| b == 0) else {
            return Err(AniError::UnterminatedCues {
                record: 0,
                at: next,
            });
        };
        let cues = &tail[..n];
        next += n + 1;
        Some(cues)
    } else {
        None
    };
    Ok((
        AniRecord {
            flags,
            step_mode: field(1),
            loop_mode: field(2),
            position_mode: field(3),
            param: field(4),
            duration: field(5),
            draw_value: field(6),
            delta_x: field(7),
            delta_y: field(8),
            reserved: u16::from_le_bytes(field(9).to_le_bytes()),
            cue_text,
        },
        next,
    ))
}
