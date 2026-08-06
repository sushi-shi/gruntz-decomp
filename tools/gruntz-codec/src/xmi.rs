//! **XMI** — Miles XMIDI stored as nested IFF chunks.
//!
//! The shipped files use `FORM XDIR`/`INFO` followed by `CAT XMID`, with one
//! `FORM XMID` per sequence. Event delays are sums of bytes below 0x80; a note
//! event carries its duration as a MIDI variable-length quantity. Miles' own
//! declarations identify the `TIMB`, `RBRN`, and `EVNT` chunks and a 120 Hz
//! sequencer service rate (`vendor/miles-6.0c/mss.h`).

use core::fmt;

#[derive(Debug, Clone, Copy)]
pub struct Xmi<'a> {
    pub declared_sequences: u16,
    forms: &'a [u8],
}

impl<'a> Xmi<'a> {
    pub fn sequences(self) -> Sequences<'a> {
        Sequences {
            chunks: Chunks::new(self.forms, 0),
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub struct Sequence<'a> {
    pub timbres: Option<&'a [u8]>,
    pub branches: Option<&'a [u8]>,
    pub events: &'a [u8],
}

impl<'a> Sequence<'a> {
    pub fn events(self) -> Events<'a> {
        Events {
            bytes: self.events,
            at: 0,
            time: 0,
            done: false,
        }
    }

    pub fn timbre_count(self) -> Result<usize, XmiError> {
        let Some(bytes) = self.timbres else {
            return Ok(0);
        };
        let count = usize::from(read_u16_le(bytes, 0)?);
        let need = 2usize
            .checked_add(count.checked_mul(2).ok_or(XmiError::Overflow)?)
            .ok_or(XmiError::Overflow)?;
        if bytes.len() != need {
            return Err(XmiError::BadTimbres {
                count,
                bytes: bytes.len(),
            });
        }
        Ok(count)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Event<'a> {
    Channel {
        time: u32,
        status: u8,
        data: [u8; 2],
        data_len: u8,
        duration: Option<u32>,
    },
    Meta {
        time: u32,
        kind: u8,
        data: &'a [u8],
    },
    SysEx {
        time: u32,
        status: u8,
        data: &'a [u8],
    },
}

impl Event<'_> {
    pub fn time(self) -> u32 {
        match self {
            Self::Channel { time, .. } | Self::Meta { time, .. } | Self::SysEx { time, .. } => time,
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub struct Sequences<'a> {
    chunks: Chunks<'a>,
}

impl<'a> Iterator for Sequences<'a> {
    type Item = Result<Sequence<'a>, XmiError>;

    fn next(&mut self) -> Option<Self::Item> {
        let chunk = self.chunks.next()?;
        Some(chunk.and_then(parse_sequence))
    }
}

#[derive(Debug, Clone, Copy)]
pub struct Events<'a> {
    bytes: &'a [u8],
    at: usize,
    time: u32,
    done: bool,
}

impl<'a> Iterator for Events<'a> {
    type Item = Result<Event<'a>, XmiError>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.done || self.at == self.bytes.len() {
            return None;
        }
        let result = parse_event(self.bytes, &mut self.at, &mut self.time).and_then(|event| {
            if matches!(event, Event::Meta { kind: 0x2f, .. }) {
                if let Some((relative, &byte)) = self.bytes[self.at..]
                    .iter()
                    .enumerate()
                    .find(|(_, byte)| **byte != 0)
                {
                    return Err(XmiError::DataAfterEnd {
                        at: self.at + relative,
                        byte,
                    });
                }
                self.done = true;
            }
            Ok(event)
        });
        if result.is_err() {
            self.done = true;
        }
        Some(result)
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum XmiError {
    Truncated { at: usize, need: usize, have: usize },
    BadTopLevel,
    BadDirectory,
    MissingInfo,
    BadInfoLength(usize),
    SequenceCount { declared: u16, found: usize },
    BadSequence,
    MissingEvents,
    DuplicateChunk([u8; 4]),
    BadTimbres { count: usize, bytes: usize },
    BadStatus { at: usize, status: u8 },
    BadData { at: usize, byte: u8 },
    BadVlq { at: usize },
    DataAfterEnd { at: usize, byte: u8 },
    Overflow,
}

impl fmt::Display for XmiError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::Truncated { at, need, have } => {
                write!(f, "XMI offset {at} needs {need} byte(s), has {have}")
            }
            Self::BadTopLevel => write!(f, "XMI must start with FORM XDIR followed by CAT XMID"),
            Self::BadDirectory => write!(f, "XDIR contains an invalid chunk"),
            Self::MissingInfo => write!(f, "XDIR has no INFO sequence count"),
            Self::BadInfoLength(n) => write!(f, "XDIR INFO is {n} bytes; expected 2"),
            Self::SequenceCount { declared, found } => {
                write!(
                    f,
                    "XDIR declares {declared} sequence(s), catalog has {found}"
                )
            }
            Self::BadSequence => write!(f, "sequence is not FORM XMID"),
            Self::MissingEvents => write!(f, "XMID sequence has no EVNT chunk"),
            Self::DuplicateChunk(id) => {
                write!(f, "XMID sequence repeats {}", Id(id))
            }
            Self::BadTimbres { count, bytes } => {
                write!(f, "TIMB declares {count} entries in {bytes} bytes")
            }
            Self::BadStatus { at, status } => {
                write!(
                    f,
                    "unsupported XMI status {status:#04x} at event offset {at}"
                )
            }
            Self::BadData { at, byte } => {
                write!(f, "status-bit data byte {byte:#04x} at event offset {at}")
            }
            Self::BadVlq { at } => write!(f, "invalid MIDI VLQ at event offset {at}"),
            Self::DataAfterEnd { at, byte } => {
                write!(
                    f,
                    "non-padding byte {byte:#04x} after end event at offset {at}"
                )
            }
            Self::Overflow => write!(f, "XMI size or timestamp overflow"),
        }
    }
}

impl core::error::Error for XmiError {}

struct Id([u8; 4]);

impl fmt::Display for Id {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for byte in self.0 {
            write!(f, "{}", char::from(byte))?;
        }
        Ok(())
    }
}

pub fn split(resource: &[u8]) -> Result<Xmi<'_>, XmiError> {
    let mut top = Chunks::new(resource, 0);
    let directory = top.next().ok_or(XmiError::BadTopLevel)??;
    let catalog = top.next().ok_or(XmiError::BadTopLevel)??;
    if top.next().is_some()
        || directory.id != *b"FORM"
        || directory.kind != Some(*b"XDIR")
        || catalog.id != *b"CAT "
        || catalog.kind != Some(*b"XMID")
    {
        return Err(XmiError::BadTopLevel);
    }

    let mut info = None;
    for chunk in Chunks::new(directory.body, directory.body_at) {
        let chunk = chunk.map_err(|_| XmiError::BadDirectory)?;
        if chunk.id == *b"INFO" {
            if info.is_some() {
                return Err(XmiError::DuplicateChunk(*b"INFO"));
            }
            if chunk.data.len() != 2 {
                return Err(XmiError::BadInfoLength(chunk.data.len()));
            }
            info = Some(read_u16_le(chunk.data, chunk.data_at)?);
        }
    }
    let declared_sequences = info.ok_or(XmiError::MissingInfo)?;

    let mut found = 0usize;
    for sequence in Chunks::new(catalog.body, catalog.body_at) {
        parse_sequence(sequence?)?;
        found = found.checked_add(1).ok_or(XmiError::Overflow)?;
    }
    if found != usize::from(declared_sequences) {
        return Err(XmiError::SequenceCount {
            declared: declared_sequences,
            found,
        });
    }
    Ok(Xmi {
        declared_sequences,
        forms: catalog.body,
    })
}

fn parse_sequence(chunk: Chunk<'_>) -> Result<Sequence<'_>, XmiError> {
    if chunk.id != *b"FORM" || chunk.kind != Some(*b"XMID") {
        return Err(XmiError::BadSequence);
    }
    let mut timbres = None;
    let mut branches = None;
    let mut events = None;
    for child in Chunks::new(chunk.body, chunk.body_at) {
        let child = child?;
        let slot = match &child.id {
            b"TIMB" => Some(&mut timbres),
            b"RBRN" => Some(&mut branches),
            b"EVNT" => Some(&mut events),
            _ => None,
        };
        if let Some(slot) = slot {
            if slot.replace(child.data).is_some() {
                return Err(XmiError::DuplicateChunk(child.id));
            }
        }
    }
    let sequence = Sequence {
        timbres,
        branches,
        events: events.ok_or(XmiError::MissingEvents)?,
    };
    sequence.timbre_count()?;
    for event in sequence.events() {
        event?;
    }
    Ok(sequence)
}

fn parse_event<'a>(bytes: &'a [u8], at: &mut usize, time: &mut u32) -> Result<Event<'a>, XmiError> {
    let start = *at;
    let mut delay = 0u32;
    while let Some(&byte) = bytes.get(*at) {
        if byte & 0x80 != 0 {
            break;
        }
        delay = delay
            .checked_add(u32::from(byte))
            .ok_or(XmiError::Overflow)?;
        *at += 1;
    }
    *time = time.checked_add(delay).ok_or(XmiError::Overflow)?;
    let status = take(bytes, at, 1, start)?[0];
    if status & 0x80 == 0 {
        return Err(XmiError::BadStatus { at: start, status });
    }

    match status {
        0x80..=0xef => {
            let data_len = if status & 0xe0 == 0xc0 { 1 } else { 2 };
            let data_bytes = take(bytes, at, data_len, start)?;
            for (i, &byte) in data_bytes.iter().enumerate() {
                if byte & 0x80 != 0 {
                    return Err(XmiError::BadData {
                        at: *at - data_len + i,
                        byte,
                    });
                }
            }
            let mut data = [0u8; 2];
            data[..data_len].copy_from_slice(data_bytes);
            let duration = if status & 0xf0 == 0x90 {
                Some(read_vlq(bytes, at)?)
            } else {
                None
            };
            Ok(Event::Channel {
                time: *time,
                status,
                data,
                data_len: u8::try_from(data_len).map_err(|_| XmiError::Overflow)?,
                duration,
            })
        }
        0xff => {
            let kind = take(bytes, at, 1, start)?[0];
            let len = usize::try_from(read_vlq(bytes, at)?).map_err(|_| XmiError::Overflow)?;
            let data = take(bytes, at, len, start)?;
            Ok(Event::Meta {
                time: *time,
                kind,
                data,
            })
        }
        0xf0 | 0xf7 => {
            let len = usize::try_from(read_vlq(bytes, at)?).map_err(|_| XmiError::Overflow)?;
            let data = take(bytes, at, len, start)?;
            Ok(Event::SysEx {
                time: *time,
                status,
                data,
            })
        }
        _ => Err(XmiError::BadStatus { at: start, status }),
    }
}

fn read_vlq(bytes: &[u8], at: &mut usize) -> Result<u32, XmiError> {
    let start = *at;
    let mut value = 0u32;
    for _ in 0..4 {
        let byte = *bytes.get(*at).ok_or(XmiError::BadVlq { at: start })?;
        *at += 1;
        value = value
            .checked_mul(128)
            .and_then(|v| v.checked_add(u32::from(byte & 0x7f)))
            .ok_or(XmiError::Overflow)?;
        if byte & 0x80 == 0 {
            return Ok(value);
        }
    }
    Err(XmiError::BadVlq { at: start })
}

#[derive(Debug, Clone, Copy)]
struct Chunk<'a> {
    id: [u8; 4],
    kind: Option<[u8; 4]>,
    data: &'a [u8],
    body: &'a [u8],
    data_at: usize,
    body_at: usize,
}

#[derive(Debug, Clone, Copy)]
struct Chunks<'a> {
    bytes: &'a [u8],
    at: usize,
    base: usize,
    done: bool,
}

impl<'a> Chunks<'a> {
    fn new(bytes: &'a [u8], base: usize) -> Self {
        Self {
            bytes,
            at: 0,
            base,
            done: false,
        }
    }
}

impl<'a> Iterator for Chunks<'a> {
    type Item = Result<Chunk<'a>, XmiError>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.done || self.at == self.bytes.len() {
            return None;
        }
        let result = parse_chunk(self.bytes, &mut self.at, self.base);
        if result.is_err() {
            self.done = true;
        }
        Some(result)
    }
}

fn parse_chunk<'a>(bytes: &'a [u8], at: &mut usize, base: usize) -> Result<Chunk<'a>, XmiError> {
    let start = *at;
    let header = take(bytes, at, 8, base + start)?;
    let id = [header[0], header[1], header[2], header[3]];
    let size = usize::try_from(u32::from_be_bytes([
        header[4], header[5], header[6], header[7],
    ]))
    .map_err(|_| XmiError::Overflow)?;
    let data_at = base.checked_add(*at).ok_or(XmiError::Overflow)?;
    let data = take(bytes, at, size, data_at)?;
    if size & 1 != 0 {
        take(bytes, at, 1, base + *at)?;
    }
    let (kind, body, body_at) = if id == *b"FORM" || id == *b"CAT " || id == *b"LIST" {
        let kind = data.get(..4).ok_or(XmiError::Truncated {
            at: data_at,
            need: 4,
            have: data.len(),
        })?;
        (
            Some([kind[0], kind[1], kind[2], kind[3]]),
            &data[4..],
            data_at + 4,
        )
    } else {
        (None, data, data_at)
    };
    Ok(Chunk {
        id,
        kind,
        data,
        body,
        data_at,
        body_at,
    })
}

fn take<'a>(
    bytes: &'a [u8],
    at: &mut usize,
    count: usize,
    absolute_at: usize,
) -> Result<&'a [u8], XmiError> {
    let end = at.checked_add(count).ok_or(XmiError::Overflow)?;
    let Some(result) = bytes.get(*at..end) else {
        return Err(XmiError::Truncated {
            at: absolute_at,
            need: count,
            have: bytes.len().saturating_sub(*at),
        });
    };
    *at = end;
    Ok(result)
}

fn read_u16_le(bytes: &[u8], at: usize) -> Result<u16, XmiError> {
    let word = bytes.get(..2).ok_or(XmiError::Truncated {
        at,
        need: 2,
        have: bytes.len(),
    })?;
    Ok(u16::from_le_bytes([word[0], word[1]]))
}
