//! Monolith **REZ version 1** archive reader (`.REZ` and `.VRZ`) — `no_std`, no `alloc`,
//! zero-copy.
//!
//! Clean-room: the layout below was derived by hexdump-walking
//! `GRUNTDEM.REZ` / `Gruntz.REZ` and validated structurally — every directory
//! parse must consume its declared `size` **exactly** (see [`Rez::validate`]).
//! No field is taken on faith from the C++ reconstruction under `src/Rez/`.
//!
//! # On-disk layout
//!
//! ```text
//! file header:
//!   b"\r\n" <ascii comment, space padded> b"\r\n" ... 0x1a       (variable length,
//!                                                                terminated by 0x1a)
//!   u32 version              == 1
//!   u32 root_dir_pos
//!   u32 root_dir_size
//!   u32 root_dir_time
//!   u32 next_write_pos
//!   u32 time
//!   u32 largest_key_ary
//!   u32 largest_dir_name_size
//!   u32 largest_rez_name_size
//!   u32 largest_comment_size
//!   u8  is_sorted
//!
//! directory body (`root_dir_pos .. +root_dir_size`, a packed entry list):
//!   u32 kind                 1 = directory, 0 = resource
//!   u32 pos                  byte offset of the child directory body / file data
//!   u32 size                 byte length of same
//!   u32 time
//!   -- resource only --
//!   u32 id
//!   u32 type                 little-endian 4CC: bytes b"DIP\0" read as "PID"
//!   u32 num_keys
//!   -- both --
//!   cstr name
//!   -- resource only --
//!   cstr comment
//! ```
//!
//! Note the asymmetry: **directories carry a name and no comment**, resources
//! carry both. That is not a guess — it is forced by the exact-size check. With
//! a comment field on directories, `AREA2`'s 0xb7-byte body over-runs; without
//! one it lands on the byte. The shipped demo/retail REZ corpora (10 553 and
//! 21 303 resources) and retail VRZ voice bank (1 517 resources) parse to the
//! byte under this rule.
//!
//! # Zero-copy
//!
//! [`Rez`] borrows the archive image; names, comments and payloads are `&str` /
//! `&[u8]` slices into it. Traversal is an [`Iterator`] driven by an explicit
//! stack of at most [`MAX_DEPTH`] frames, so nothing is allocated and nothing
//! recurses.

#![no_std]

pub mod fec;

use core::fmt;

use gruntz_cast::AsUsize;

/// Directory nesting the walker can hold without allocating. The shipped
/// archives use three levels (`AREA2\IMAGEZ\TREE2`); this is deliberate slack.
pub const MAX_DEPTH: usize = 16;

/// Everything that can go wrong reading a REZ. Carries integers and borrowed
/// names only — never a formatted string.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum RezError<'a> {
    /// No 0x1a header terminator in the first 512 bytes.
    NoHeaderTerminator,
    /// The header terminator was found but the fixed fields ran off the end.
    ShortHeader,
    /// `version` was not 1.
    BadVersion(u32),
    /// A directory body did not end exactly on its declared size.
    DirSizeMismatch {
        dir: &'a str,
        declared: u32,
        consumed: usize,
    },
    /// An entry's `pos..pos+size` fell outside the archive.
    OutOfBounds { name: &'a str, pos: u32, size: u32 },
    /// A name/comment cstring ran past the end of its directory body.
    UnterminatedString { dir: &'a str, at: usize },
    /// An entry `kind` other than 0/1.
    BadEntryKind { dir: &'a str, kind: u32 },
    /// Nesting deeper than [`MAX_DEPTH`].
    TooDeep { dir: &'a str },
}

impl fmt::Display for RezError<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            RezError::NoHeaderTerminator => write!(f, "no 0x1a header terminator"),
            RezError::ShortHeader => write!(f, "header truncated"),
            RezError::BadVersion(v) => write!(f, "unsupported REZ version {v} (expected 1)"),
            RezError::DirSizeMismatch {
                dir,
                declared,
                consumed,
            } => write!(
                f,
                "directory {dir}: declared size {declared} but the entry list consumed {consumed}"
            ),
            RezError::OutOfBounds { name, pos, size } => {
                write!(f, "{name}: pos {pos:#x}+{size:#x} is outside the archive")
            }
            RezError::UnterminatedString { dir, at } => {
                write!(f, "{dir}: unterminated string at offset {at}")
            }
            RezError::BadEntryKind { dir, kind } => write!(
                f,
                "{dir}: entry kind {kind} is neither 0 (resource) nor 1 (directory)"
            ),
            RezError::TooDeep { dir } => write!(f, "{dir}: nesting deeper than {MAX_DEPTH}"),
        }
    }
}

impl core::error::Error for RezError<'_> {}

/// A resource type tag, kept as its four raw bytes.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct FourCc(pub u32);

impl FourCc {
    /// The tag as it reads on screen: stored `b"DIP\0"` is `PID`.
    pub fn as_str(self, buf: &mut [u8; 4]) -> &str {
        let raw = self.0.to_le_bytes();
        let mut n = 0;
        for &c in raw.iter().rev() {
            if c != 0 && c.is_ascii_graphic() {
                buf[n] = c;
                n += 1;
            }
        }
        core::str::from_utf8(&buf[..n]).unwrap_or("?")
    }
}

impl fmt::Display for FourCc {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        let mut buf = [0u8; 4];
        let s = self.as_str(&mut buf);
        if s.is_empty() {
            write!(f, "{:#010x}", self.0)
        } else {
            f.write_str(s)
        }
    }
}

/// The parsed file header. `comment` borrows the archive.
#[derive(Debug, Clone, Copy)]
pub struct RezHeader<'a> {
    pub comment: &'a str,
    pub version: u32,
    pub root_dir_pos: u32,
    pub root_dir_size: u32,
    pub root_dir_time: u32,
    pub next_write_pos: u32,
    pub time: u32,
    pub largest_key_ary: u32,
    pub largest_dir_name_size: u32,
    pub largest_rez_name_size: u32,
    pub largest_comment_size: u32,
    pub is_sorted: u8,
}

/// A resource (leaf file). Every field borrows the archive image.
#[derive(Debug, Clone, Copy)]
pub struct Resource<'a> {
    pub name: &'a str,
    pub comment: &'a str,
    pub kind: FourCc,
    pub id: u32,
    pub pos: u32,
    pub size: u32,
    pub time: u32,
    pub num_keys: u32,
    /// The enclosing directory chain, outermost first — e.g.
    /// `["AREA2", "IMAGEZ", "TREE2"]`. Renders as a path via [`Path`].
    pub dirs: Dirs<'a>,
}

impl<'a> Resource<'a> {
    /// The resource bytes. The range was bounds-checked during the walk.
    pub fn data(&self, archive: &'a [u8]) -> &'a [u8] {
        let start = self.pos.as_usize();
        &archive[start..start + self.size.as_usize()]
    }

    /// `AREA2\IMAGEZ\TREE2\FRAME001`, formatted on demand — no string is built
    /// unless someone asks.
    pub fn path(&self) -> Path<'a, '_> {
        Path(self)
    }
}

/// An inline directory chain — a fixed-capacity stack, no allocation.
#[derive(Debug, Clone, Copy)]
pub struct Dirs<'a> {
    names: [&'a str; MAX_DEPTH],
    len: usize,
}

impl<'a> Dirs<'a> {
    pub fn as_slice(&self) -> &[&'a str] {
        &self.names[..self.len]
    }
    pub fn len(&self) -> usize {
        self.len
    }
    pub fn is_empty(&self) -> bool {
        self.len == 0
    }
}

impl fmt::Display for Dirs<'_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for (i, d) in self.as_slice().iter().enumerate() {
            if i > 0 {
                f.write_str("\\")?;
            }
            f.write_str(d)?;
        }
        Ok(())
    }
}

/// `Display` adaptor for a resource's full path.
pub struct Path<'a, 'r>(&'r Resource<'a>);

impl fmt::Display for Path<'_, '_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for d in self.0.dirs.as_slice() {
            f.write_str(d)?;
            f.write_str("\\")?;
        }
        f.write_str(self.0.name)
    }
}

/// A borrowed REZ archive.
#[derive(Debug, Clone, Copy)]
pub struct Rez<'a> {
    bytes: &'a [u8],
    pub header: RezHeader<'a>,
}

fn rd_u32(b: &[u8], at: usize) -> Option<u32> {
    let s: &[u8; 4] = b.get(at..at + 4)?.try_into().ok()?;
    Some(u32::from_le_bytes(*s))
}

impl<'a> Rez<'a> {
    /// Parse the file header only. Directory traversal is lazy — see
    /// [`Rez::resources`].
    pub fn new(bytes: &'a [u8]) -> Result<Rez<'a>, RezError<'a>> {
        let term = bytes
            .iter()
            .take(512)
            .position(|&c| c == 0x1a)
            .ok_or(RezError::NoHeaderTerminator)?;
        let comment = core::str::from_utf8(&bytes[..term])
            .unwrap_or("")
            .trim_matches(|c: char| c == '\r' || c == '\n' || c == ' ');
        let f = |i: usize| rd_u32(bytes, term + 1 + i * 4).ok_or(RezError::ShortHeader);
        let header = RezHeader {
            comment,
            version: f(0)?,
            root_dir_pos: f(1)?,
            root_dir_size: f(2)?,
            root_dir_time: f(3)?,
            next_write_pos: f(4)?,
            time: f(5)?,
            largest_key_ary: f(6)?,
            largest_dir_name_size: f(7)?,
            largest_rez_name_size: f(8)?,
            largest_comment_size: f(9)?,
            is_sorted: *bytes.get(term + 1 + 40).ok_or(RezError::ShortHeader)?,
        };
        if header.version != 1 {
            return Err(RezError::BadVersion(header.version));
        }
        Ok(Rez { bytes, header })
    }

    pub fn bytes(&self) -> &'a [u8] {
        self.bytes
    }

    /// Depth-first iterator over every resource in the archive.
    pub fn resources(&self) -> Resources<'a> {
        let mut it = Resources {
            bytes: self.bytes,
            stack: [Frame::EMPTY; MAX_DEPTH],
            depth: 0,
            dirs: Dirs {
                names: [""; MAX_DEPTH],
                len: 0,
            },
            failed: false,
        };
        it.push(
            self.header.root_dir_pos,
            self.header.root_dir_size,
            "<root>",
        );
        it
    }

    /// Walk the whole directory tree purely to check it, returning the resource
    /// count. Any structural inconsistency — an entry list that does not land
    /// exactly on its directory's declared size above all — surfaces here.
    pub fn validate(&self) -> Result<usize, RezError<'a>> {
        let mut n = 0usize;
        for r in self.resources() {
            r?;
            n += 1;
        }
        Ok(n)
    }
}

#[derive(Debug, Clone, Copy)]
struct Frame<'a> {
    /// Absolute offset of the next unread entry.
    at: usize,
    /// Absolute end of this directory body.
    end: usize,
    /// Name, for error reporting.
    dir: &'a str,
    /// Declared size, for error reporting.
    declared: u32,
    /// Absolute start, so `consumed` can be reported.
    start: usize,
}

impl Frame<'_> {
    const EMPTY: Frame<'static> = Frame {
        at: 0,
        end: 0,
        dir: "",
        declared: 0,
        start: 0,
    };
}

/// Depth-first resource iterator. Yields `Err` once and then stops.
pub struct Resources<'a> {
    bytes: &'a [u8],
    stack: [Frame<'a>; MAX_DEPTH],
    depth: usize,
    dirs: Dirs<'a>,
    failed: bool,
}

impl<'a> Resources<'a> {
    fn push(&mut self, pos: u32, size: u32, dir: &'a str) -> bool {
        if self.depth >= MAX_DEPTH {
            return false;
        }
        let start = pos.as_usize();
        self.stack[self.depth] = Frame {
            at: start,
            end: start + size.as_usize(),
            dir,
            declared: size,
            start,
        };
        self.depth += 1;
        true
    }

    fn fail(&mut self, e: RezError<'a>) -> Option<Result<Resource<'a>, RezError<'a>>> {
        self.failed = true;
        Some(Err(e))
    }
}

impl<'a> Iterator for Resources<'a> {
    type Item = Result<Resource<'a>, RezError<'a>>;

    fn next(&mut self) -> Option<Self::Item> {
        loop {
            if self.failed || self.depth == 0 {
                return None;
            }
            let top = self.depth - 1;
            let frame = self.stack[top];
            if frame.at >= frame.end {
                // A directory body must land EXACTLY on its declared size; an
                // over-run means the entry layout is wrong.
                if frame.at != frame.end {
                    return self.fail(RezError::DirSizeMismatch {
                        dir: frame.dir,
                        declared: frame.declared,
                        consumed: frame.at - frame.start,
                    });
                }
                self.depth -= 1;
                if self.dirs.len > 0 && self.depth > 0 {
                    self.dirs.len -= 1;
                }
                continue;
            }

            let b = self.bytes;
            let short = RezError::DirSizeMismatch {
                dir: frame.dir,
                declared: frame.declared,
                consumed: frame.at - frame.start,
            };
            let mut at = frame.at;
            let too_far = |at: usize, n: usize| at + n > frame.end;
            if too_far(at, 16) {
                return self.fail(short);
            }
            let (Some(kind), Some(pos), Some(size), Some(time)) = (
                rd_u32(b, at),
                rd_u32(b, at + 4),
                rd_u32(b, at + 8),
                rd_u32(b, at + 12),
            ) else {
                return self.fail(short);
            };
            at += 16;
            let (id, raw_kind, num_keys) = match kind {
                0 => {
                    if too_far(at, 12) {
                        return self.fail(short);
                    }
                    let (Some(a), Some(bb), Some(c)) =
                        (rd_u32(b, at), rd_u32(b, at + 4), rd_u32(b, at + 8))
                    else {
                        return self.fail(short);
                    };
                    at += 12;
                    (a, bb, c)
                }
                1 => (0, 0, 0),
                _ => {
                    return self.fail(RezError::BadEntryKind {
                        dir: frame.dir,
                        kind,
                    })
                }
            };
            let name = match read_cstr(b, &mut at, frame.end, frame.dir) {
                Ok(s) => s,
                Err(e) => return self.fail(e),
            };
            let comment = if kind == 0 {
                match read_cstr(b, &mut at, frame.end, frame.dir) {
                    Ok(s) => s,
                    Err(e) => return self.fail(e),
                }
            } else {
                ""
            };
            self.stack[top].at = at;

            if pos.as_usize() + size.as_usize() > b.len() {
                return self.fail(RezError::OutOfBounds { name, pos, size });
            }
            if kind == 1 {
                if self.dirs.len >= MAX_DEPTH {
                    return self.fail(RezError::TooDeep { dir: name });
                }
                self.dirs.names[self.dirs.len] = name;
                self.dirs.len += 1;
                if !self.push(pos, size, name) {
                    return self.fail(RezError::TooDeep { dir: name });
                }
                continue;
            }
            return Some(Ok(Resource {
                name,
                comment,
                kind: FourCc(raw_kind),
                id,
                pos,
                size,
                time,
                num_keys,
                dirs: self.dirs,
            }));
        }
    }
}

fn read_cstr<'a>(
    b: &'a [u8],
    at: &mut usize,
    end: usize,
    dir: &'a str,
) -> Result<&'a str, RezError<'a>> {
    let rest = b
        .get(*at..end)
        .ok_or(RezError::UnterminatedString { dir, at: *at })?;
    let n = rest
        .iter()
        .position(|&c| c == 0)
        .ok_or(RezError::UnterminatedString { dir, at: *at })?;
    let s = core::str::from_utf8(&rest[..n]).unwrap_or("<non-utf8>");
    *at += n + 1;
    Ok(s)
}
