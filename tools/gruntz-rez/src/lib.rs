//! Monolith **REZ version 1** archive reader and writer (`.REZ` and `.VRZ`) —
//! `no_std`, zero-copy on the read side.
//!
//! Clean-room: the layout below was first derived by hexdump-walking
//! `GRUNTDEM.REZ` / `Gruntz.REZ` and validated structurally — every directory
//! parse must consume its declared `size` **exactly** (see [`Rez::validate`]).
//! It was subsequently confirmed field-for-field against retail's own header
//! parser and directory parser in `GRUNTZ.EXE`; the RVAs are cited per field
//! below. No field is taken on faith from the C++ reconstruction under
//! `src/Rez/` (which models only the file-driver layer, not the container).
//!
//! # On-disk layout
//!
//! ```text
//! file header — EXACTLY 168 bytes, fixed offsets  (CRezMgr::Open @0x13ad00
//!                                                  reads 0xa8 bytes at offset 0)
//!   0x00  b"\r\n"                                 (retail asserts byte 0x00 == '\r')
//!   0x02  60 bytes  banner line 1, space padded
//!   0x3e  b"\r\n"                                 (retail asserts byte 0x3f == '\n')
//!   0x40  60 bytes  banner line 2, space padded
//!   0x7c  b"\r\n"
//!   0x7e  0x1a                                    (retail asserts byte 0x7e == 0x1a;
//!                                                  DOS `TYPE` stops here)
//!   0x7f  u32 version                             == 1, retail asserts it
//!   0x83  u32 root_dir_pos
//!   0x87  u32 root_dir_size
//!   0x8b  u32 root_dir_time
//!   0x8f  u32 next_write_pos
//!   0x93  u32 time                                time_t of the last write
//!   0x97  u32 largest_key_ary
//!   0x9b  u32 largest_dir_name_size
//!   0x9f  u32 largest_rez_name_size
//!   0xa3  u32 largest_comment_size
//!   0xa7  u8  is_sorted
//!   0xa8  first payload byte
//!
//! directory body (`root_dir_pos .. +root_dir_size`, a packed entry list;
//!                 CRezDir::ReadDirBlock @0x13a640):
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
//!   u32 keys[num_keys]       absent in every shipped archive (largest_key_ary == 0)
//! ```
//!
//! Note the asymmetry: **directories carry a name and no comment**, resources
//! carry both. That is not a guess — it is forced by the exact-size check. With
//! a comment field on directories, `AREA2`'s 0xb7-byte body over-runs; without
//! one it lands on the byte. The shipped demo/retail REZ corpora (10 553 and
//! 21 303 resources) and retail VRZ voice bank (1 517 resources) parse to the
//! byte under this rule. Retail's parser reads the two fields in that order at
//! 0x13a7f6 (name) and 0x13a825 (comment), and treats an empty comment as
//! absent.
//!
//! The header scan below looks for the 0x1a terminator rather than indexing
//! 0x7e directly. That is a superset of what retail does — retail hard-codes
//! the offsets — and it agrees on every archive whose banner is the standard
//! 127-byte block, which is all three shipped ones.
//!
//! # What `is_sorted` means
//!
//! It is **not** an ordering claim about sibling entries, and it drives no
//! search. It asserts that each directory's resource payloads occupy **one
//! contiguous span** of the file, so a whole directory can be preloaded with a
//! single read.
//!
//! `CRezDir::ReadDirBlock` accumulates, per directory, `min(pos)` and
//! `sum(size)` over its own resources (0x13a8c8..0x13a8e6). `CRezDir::Load`
//! @0x13a0f0 then reads exactly `[min(pos), min(pos) + sum(size))` into one
//! heap block, but only after checking `mgr->is_sorted != 0` and
//! `mgr->open_file_count <= 1`; otherwise it prints
//! `"CRezDir::Load Failed! (File is not sorted!)"` and returns 0. Once that
//! block exists, `CRezItm::Read` @0x139a40 serves resource bytes from
//! `blob + (item.pos - dir.min_pos)` instead of touching the file — which is
//! correct only if the resources tile that span exactly.
//!
//! All three shipped archives satisfy the predicate for every one of their
//! non-empty directories (1784 / 58 / 915), while only 290 / 0 / 171
//! directories have their entries in ascending-position or lexicographic
//! order — so the flag cannot be describing entry order. [`Rez::is_contiguous`]
//! checks the real predicate.
//!
//! In the shipped `GRUNTZ.EXE` the flag is nonetheless **inert**: the only
//! rel32 caller of `CRezDir::Load` is its own recursion at 0x13a15b and no
//! vtable slot holds its address, so `dir->blob` is always null and the
//! fast path at 0x139a40 is never taken.
//!
//! # Zero-copy
//!
//! [`Rez`] borrows the archive image; names, comments and payloads are `&str` /
//! `&[u8]` slices into it. Traversal is an [`Iterator`] driven by an explicit
//! stack of at most [`MAX_DEPTH`] frames, so nothing is allocated and nothing
//! recurses. The writer in [`write`] is the one part that needs an allocator
//! and is behind the `alloc` feature; the reader never allocates in either
//! configuration.

#![no_std]

#[cfg(feature = "alloc")]
extern crate alloc;

pub mod fec;
#[cfg(feature = "alloc")]
pub mod write;

/// Size of the REZ v1 file header, in bytes.
///
/// Retail hard-codes this: `CRezMgr::Open` @0x13ad00 issues one
/// `Read(0, 0, 0xa8, buf)` and then indexes fixed offsets into `buf`. A newly
/// created archive gets `next_write_pos = 0xa8` at 0x13af21, and the first
/// payload byte of all three shipped archives is at 0xa8.
pub const HEADER_SIZE: usize = 168;

/// Offset of the `version` field, i.e. one past the 0x1a terminator.
pub const HEADER_FIELDS_AT: usize = 0x7f;

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
    /// A directory's resources do not tile one contiguous span, so the archive
    /// cannot honestly claim `is_sorted = 1`. See [`Rez::is_contiguous`].
    NotContiguous { dir: &'a str },
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
            RezError::NotContiguous { dir } => write!(
                f,
                "{dir}: resources do not tile one contiguous span; is_sorted would be a lie"
            ),
        }
    }
}

impl core::error::Error for RezError<'_> {}

/// A resource type tag, kept as its four raw bytes.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash)]
pub struct FourCc(pub u32);

impl FourCc {
    /// Build a tag from its on-screen spelling: `"PID"` becomes the dword
    /// 0x00504944, which stores as `b"DIP\0"`. The inverse of [`FourCc::as_str`].
    pub const fn from_tag(tag: &str) -> FourCc {
        let bytes = tag.as_bytes();
        let mut v = 0u32;
        let mut i = 0;
        while i < bytes.len() && i < 4 {
            v = (v << 8) | bytes[i] as u32;
            i += 1;
        }
        FourCc(v)
    }

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
    /// The `num_keys * 4` raw little-endian bytes that follow the comment, as
    /// they sit in the directory body. Empty in every shipped archive. Read
    /// them with [`Resource::keys`].
    pub keys_raw: &'a [u8],
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

    /// The key array, decoded. Retail allocates `num_keys * 4` bytes and copies
    /// that many dwords out of the body (0x13a856..0x13a86e); it is empty in
    /// every shipped archive, so what the keys *mean* is undetermined.
    pub fn keys(&self) -> impl Iterator<Item = u32> + 'a {
        self.keys_raw
            .chunks_exact(4)
            .map(|c| u32::from_le_bytes([c[0], c[1], c[2], c[3]]))
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

    /// Depth-first iterator over every directory in the archive, root excluded
    /// (the root has no entry anywhere — the header points straight at its
    /// body).
    ///
    /// [`Rez::resources`] cannot stand in for this: an archive may hold
    /// directories with no resources beneath them at all — retail `Gruntz.REZ`
    /// has 25 and the demo 11 — and their `time` fields exist only here.
    pub fn directories(&self) -> Directories<'a> {
        let mut it = Directories {
            bytes: self.bytes,
            stack: [DirFrame::EMPTY; MAX_DEPTH],
            depth: 1,
            dirs: Dirs {
                names: [""; MAX_DEPTH],
                len: 0,
            },
            failed: false,
        };
        it.stack[0] = DirFrame {
            pos: self.header.root_dir_pos,
            size: self.header.root_dir_size,
            name: "<root>",
            next_child: 0,
        };
        it
    }

    /// Check the predicate `is_sorted` actually asserts: that **every**
    /// directory's own resources tile one contiguous span
    /// `[min(pos), min(pos) + sum(size))` with no gap and no overlap.
    ///
    /// That is the precondition `CRezDir::Load` @0x13a0f0 relies on when it
    /// slurps a whole directory into one block and `CRezItm::Read` @0x139a40
    /// then serves each resource from `blob + (pos - min_pos)`. An archive that
    /// claims `is_sorted = 1` without it would hand out the wrong bytes.
    ///
    /// Empty directories are vacuously contiguous. Returns the offending
    /// directory's name on failure.
    pub fn is_contiguous(&self) -> Result<(), RezError<'a>> {
        // Depth-first with one frame per NESTING level, so MAX_DEPTH bounds it.
        // `next_child` is an index into the frame's directory entries rather
        // than a saved cursor, which costs a rescan per descent and keeps the
        // frame Copy.
        #[derive(Clone, Copy)]
        struct Frame<'a> {
            pos: u32,
            size: u32,
            name: &'a str,
            next_child: usize,
        }
        let mut stack = [Frame {
            pos: 0,
            size: 0,
            name: "",
            next_child: 0,
        }; MAX_DEPTH];
        stack[0] = Frame {
            pos: self.header.root_dir_pos,
            size: self.header.root_dir_size,
            name: "<root>",
            next_child: 0,
        };
        let mut depth = 1;
        while depth > 0 {
            let f = stack[depth - 1];
            if f.next_child == 0 {
                self.check_tiling(f.pos, f.size, f.name)?;
            }
            let mut seen = 0usize;
            let mut found = None;
            for e in Body::new(self.bytes, f.pos, f.size, f.name) {
                if let Entry::Dir {
                    pos, size, name, ..
                } = e?
                {
                    if seen == f.next_child {
                        found = Some(Frame {
                            pos,
                            size,
                            name,
                            next_child: 0,
                        });
                        break;
                    }
                    seen += 1;
                }
            }
            stack[depth - 1].next_child += 1;
            match found {
                Some(child) => {
                    if depth >= MAX_DEPTH {
                        return Err(RezError::TooDeep { dir: child.name });
                    }
                    stack[depth] = child;
                    depth += 1;
                }
                None => depth -= 1,
            }
        }
        Ok(())
    }

    /// One directory's own resources must tile `[min(pos), min(pos) + sum(size))`.
    fn check_tiling(&self, pos: u32, size: u32, name: &'a str) -> Result<(), RezError<'a>> {
        let mut min_pos = u32::MAX;
        let mut total = 0u64;
        let mut count = 0usize;
        for e in Body::new(self.bytes, pos, size, name) {
            if let Entry::Res { pos, size, .. } = e? {
                min_pos = min_pos.min(pos);
                total += u64::from(size);
                count += 1;
            }
        }
        if count == 0 {
            return Ok(());
        }
        // Walk the span: at each cursor there must be at least one resource
        // starting exactly there, and the sizes of all such resources are the
        // step forward. O(n^2) in a directory's resource count -- 299 at worst
        // in retail, 933k entry decodes for the whole archive -- and it needs
        // no allocation, which a sort would.
        let mut cursor = u64::from(min_pos);
        let end = cursor + total;
        let mut consumed = 0usize;
        while consumed < count {
            let mut step = 0u64;
            let mut hits = 0usize;
            for e in Body::new(self.bytes, pos, size, name) {
                if let Entry::Res { pos, size, .. } = e? {
                    if u64::from(pos) == cursor {
                        step += u64::from(size);
                        hits += 1;
                    }
                }
            }
            consumed += hits;
            cursor += step;
            if hits == 0 || (step == 0 && consumed < count) {
                return Err(RezError::NotContiguous { dir: name });
            }
        }
        if cursor != end {
            return Err(RezError::NotContiguous { dir: name });
        }
        Ok(())
    }
}

/// A directory entry as it appears in its parent's body.
#[derive(Debug, Clone, Copy)]
pub struct Directory<'a> {
    pub name: &'a str,
    pub pos: u32,
    pub size: u32,
    pub time: u32,
    /// The enclosing chain, outermost first, NOT including `name` itself.
    pub parents: Dirs<'a>,
}

impl<'a> Directory<'a> {
    /// `AREA2\IMAGEZ\TREE2`, formatted on demand.
    pub fn path(&self) -> DirPath<'a, '_> {
        DirPath(self)
    }
}

/// `Display` adaptor for a directory's full path.
pub struct DirPath<'a, 'd>(&'d Directory<'a>);

impl fmt::Display for DirPath<'_, '_> {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        for d in self.0.parents.as_slice() {
            f.write_str(d)?;
            f.write_str("\\")?;
        }
        f.write_str(self.0.name)
    }
}

#[derive(Clone, Copy)]
struct DirFrame<'a> {
    pos: u32,
    size: u32,
    name: &'a str,
    /// How many of this body's directory entries have already been descended
    /// into. Re-scanning the body per descent keeps the frame `Copy` and the
    /// whole walk allocation-free; a body holds a few hundred entries at most.
    next_child: usize,
}

impl DirFrame<'_> {
    const EMPTY: DirFrame<'static> = DirFrame {
        pos: 0,
        size: 0,
        name: "",
        next_child: 0,
    };
}

/// Depth-first directory iterator. Yields `Err` once and then stops.
pub struct Directories<'a> {
    bytes: &'a [u8],
    stack: [DirFrame<'a>; MAX_DEPTH],
    depth: usize,
    dirs: Dirs<'a>,
    failed: bool,
}

impl<'a> Iterator for Directories<'a> {
    type Item = Result<Directory<'a>, RezError<'a>>;

    fn next(&mut self) -> Option<Self::Item> {
        loop {
            if self.failed || self.depth == 0 {
                return None;
            }
            let f = self.stack[self.depth - 1];
            let mut seen = 0usize;
            let mut found = None;
            for e in Body::new(self.bytes, f.pos, f.size, f.name) {
                match e {
                    Ok(Entry::Dir {
                        pos,
                        size,
                        time,
                        name,
                    }) => {
                        if seen == f.next_child {
                            found = Some((pos, size, time, name));
                            break;
                        }
                        seen += 1;
                    }
                    Ok(Entry::Res { .. }) => {}
                    Err(e) => {
                        self.failed = true;
                        return Some(Err(e));
                    }
                }
            }
            self.stack[self.depth - 1].next_child += 1;
            match found {
                Some((pos, size, time, name)) => {
                    if self.depth >= MAX_DEPTH || self.dirs.len >= MAX_DEPTH {
                        self.failed = true;
                        return Some(Err(RezError::TooDeep { dir: name }));
                    }
                    let out = Directory {
                        name,
                        pos,
                        size,
                        time,
                        parents: self.dirs,
                    };
                    self.dirs.names[self.dirs.len] = name;
                    self.dirs.len += 1;
                    self.stack[self.depth] = DirFrame {
                        pos,
                        size,
                        name,
                        next_child: 0,
                    };
                    self.depth += 1;
                    return Some(Ok(out));
                }
                None => {
                    self.depth -= 1;
                    if self.dirs.len > 0 {
                        self.dirs.len -= 1;
                    }
                }
            }
        }
    }
}

/// One decoded entry of a directory body.
#[derive(Debug, Clone, Copy)]
enum Entry<'a> {
    Dir {
        pos: u32,
        size: u32,
        time: u32,
        name: &'a str,
    },
    Res {
        pos: u32,
        size: u32,
        #[allow(dead_code)]
        name: &'a str,
    },
}

/// Entries of ONE directory body, without descending. Kept separate from
/// [`Resources`] so the depth-first walk that every other caller uses is not
/// disturbed.
struct Body<'a> {
    bytes: &'a [u8],
    at: usize,
    end: usize,
    dir: &'a str,
    done: bool,
}

impl<'a> Body<'a> {
    fn new(bytes: &'a [u8], pos: u32, size: u32, dir: &'a str) -> Body<'a> {
        let start = pos.as_usize();
        Body {
            bytes,
            at: start,
            end: start.saturating_add(size.as_usize()).min(bytes.len()),
            dir,
            done: false,
        }
    }
}

impl<'a> Iterator for Body<'a> {
    type Item = Result<Entry<'a>, RezError<'a>>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.done || self.at >= self.end {
            return None;
        }
        let fail = |s: &mut Self| {
            s.done = true;
            Some(Err(RezError::UnterminatedString {
                dir: s.dir,
                at: s.at,
            }))
        };
        let b = self.bytes;
        let mut at = self.at;
        if at + 16 > self.end {
            return fail(self);
        }
        let (Some(kind), Some(pos), Some(size), Some(time)) = (
            rd_u32(b, at),
            rd_u32(b, at + 4),
            rd_u32(b, at + 8),
            rd_u32(b, at + 12),
        ) else {
            return fail(self);
        };
        at += 16;
        let num_keys = if kind == 0 {
            if at + 12 > self.end {
                return fail(self);
            }
            let Some(n) = rd_u32(b, at + 8) else {
                return fail(self);
            };
            at += 12;
            n
        } else if kind == 1 {
            0
        } else {
            self.done = true;
            return Some(Err(RezError::BadEntryKind {
                dir: self.dir,
                kind,
            }));
        };
        let name = match read_cstr(b, &mut at, self.end, self.dir) {
            Ok(s) => s,
            Err(e) => {
                self.done = true;
                return Some(Err(e));
            }
        };
        if kind == 0 {
            if let Err(e) = read_cstr(b, &mut at, self.end, self.dir) {
                self.done = true;
                return Some(Err(e));
            }
            match num_keys.as_usize().checked_mul(4) {
                Some(n) if at + n <= self.end => at += n,
                _ => return fail(self),
            }
        }
        self.at = at;
        Some(Ok(if kind == 1 {
            Entry::Dir {
                pos,
                size,
                time,
                name,
            }
        } else {
            Entry::Res { pos, size, name }
        }))
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
            // The key array trails the comment (0x13a856: `malloc(num_keys * 4)`
            // then a dword copy loop that advances the body cursor). Every
            // shipped archive has num_keys == 0, so this is normally a no-op.
            let keys_raw = match num_keys.as_usize().checked_mul(4) {
                Some(n) if at + n <= frame.end => {
                    let s = &b[at..at + n];
                    at += n;
                    s
                }
                _ => return self.fail(short),
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
                keys_raw,
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
