//! Monolith **REZ version 1** archive writer.
//!
//! The mirror of the reader in the crate root, and the only part of this crate
//! that needs an allocator (feature `alloc`): a directory body's length is not
//! known until its children are placed, so a growable output buffer is the
//! honest data structure. Payloads are still **borrowed** — the builder holds
//! `&[u8]` into whatever the caller already has mapped, and the single `Vec`
//! it produces is the archive image.
//!
//! # What it emits
//!
//! ```text
//! [0, 168)                    the fixed header  (BANNER + 41 bytes of fields)
//! [168, next_write_pos)       resource payloads, one contiguous run per directory
//! [next_write_pos, EOF)       directory bodies, children before parents
//! root_dir_pos                the root body, written last
//! ```
//!
//! That is retail's shape. `CRezMgr::Open` @0x13af21 sets `next_write_pos` to
//! 0xa8 for a newly created archive, i.e. payload space begins immediately
//! after the header; in all three shipped archives `next_write_pos` is exactly
//! `max(pos + size)` over every resource, i.e. the end of the payload region,
//! with the directory bodies living beyond it.
//!
//! Grouping each directory's payloads into one run is not cosmetic: it is what
//! makes `is_sorted = 1` true. See [`Rez::is_contiguous`](crate::Rez::is_contiguous).
//!
//! # What it does not reproduce
//!
//! Retail's archives are not the output of a single pass — roughly half the
//! bytes between `next_write_pos` and EOF are *orphaned* earlier copies of
//! directory bodies (850 177 dead bytes against 850 420 live ones in retail
//! `Gruntz.REZ`), left behind because each rewrite appended a fresh body rather
//! than overwriting in place. A one-pass writer has nothing to orphan, so its
//! output is shorter and its `root_dir_pos` differs. See
//! `docs/formats/rez-v1.md`.

use alloc::string::String;
use alloc::vec::Vec;

use crate::{FourCc, HEADER_SIZE, MAX_DEPTH};

/// The 127-byte banner block that opens every shipped archive, byte for byte —
/// retail `Gruntz.REZ`, retail `GRUNTZ.VRZ` and `GRUNTDEM.REZ` are identical
/// here.
///
/// Retail validates three of these bytes and no more (`CRezMgr::Open`
/// @0x13b004 / @0x13b016 / @0x13b021): `[0x00] == '\r'`, `[0x3f] == '\n'`,
/// `[0x7e] == 0x1a`. The rest is human-readable filler, and the 0x1a is a DOS
/// end-of-file marker so `TYPE GRUNTZ.REZ` stops at the banner.
pub const BANNER: [u8; 127] = banner(BANNER_LINE_1, b"");

/// Length of one banner text line, between the CRLFs.
pub const BANNER_LINE: usize = 60;

/// Retail's first banner line, space-padded to [`BANNER_LINE`] on disk.
pub const BANNER_LINE_1: &[u8] = b"RezMgr Version 1 Copyright (C) 1995 MONOLITH INC.";

const KIND_RESOURCE: u32 = 0;
const KIND_DIRECTORY: u32 = 1;

/// Assemble the banner block from its two text lines.
const fn banner(line1: &[u8], line2: &[u8]) -> [u8; 127] {
    let mut b = [b' '; 127];
    b[0x00] = b'\r';
    b[0x01] = b'\n';
    let mut i = 0;
    while i < line1.len() {
        b[0x02 + i] = line1[i];
        i += 1;
    }
    b[0x3e] = b'\r';
    b[0x3f] = b'\n';
    let mut i = 0;
    while i < line2.len() {
        b[0x40 + i] = line2[i];
        i += 1;
    }
    b[0x7c] = b'\r';
    b[0x7d] = b'\n';
    b[0x7e] = 0x1a;
    b
}

/// Everything a writer can refuse to do.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum WriteError {
    /// A name was empty, held a NUL or a path separator, or was not printable
    /// ASCII.
    ///
    /// NUL is impossible because names are C strings in the body; `\` is
    /// refused because it is the separator the game addresses resources with,
    /// so a name containing one would be unreachable.
    BadName(String),
    /// Two resources in one directory share a name *and* a type — compared
    /// case-insensitively.
    ///
    /// Retail indexes a directory's resources by type and then by name
    /// (`CRezDir::ReadDirBlock` @0x13a7d5 looks the name up in the type's own
    /// hash) and *replaces* on a collision, silently dropping the earlier
    /// entry. Refusing is better than emitting an archive that loses data.
    Duplicate { dir: String, name: String },
    /// Nesting deeper than [`MAX_DEPTH`].
    TooDeep(String),
    /// The archive would exceed 4 GiB, which the u32 offsets cannot address.
    TooLarge,
    /// A banner line longer than [`BANNER_LINE`], or not printable ASCII.
    BadBanner,
}

impl core::fmt::Display for WriteError {
    fn fmt(&self, f: &mut core::fmt::Formatter<'_>) -> core::fmt::Result {
        match self {
            WriteError::BadName(n) => write!(f, "illegal REZ name {n:?}"),
            WriteError::Duplicate { dir, name } => write!(f, "{dir}: duplicate entry {name:?}"),
            WriteError::TooDeep(d) => write!(f, "{d}: nesting deeper than {MAX_DEPTH}"),
            WriteError::TooLarge => write!(f, "archive would exceed 4 GiB"),
            WriteError::BadBanner => write!(
                f,
                "banner lines must be <= {BANNER_LINE} printable ASCII bytes"
            ),
        }
    }
}

impl core::error::Error for WriteError {}

/// One resource to write. `data` is borrowed; nothing is copied until
/// [`RezBuilder::finish`].
#[derive(Debug, Clone, Copy)]
pub struct ResourceSpec<'a> {
    pub name: &'a str,
    /// Almost always empty — `largest_comment_size` is 0 in every shipped
    /// archive, and retail treats an empty comment as absent (@0x13a83a).
    pub comment: &'a str,
    pub kind: FourCc,
    pub id: u32,
    /// `time_t` of the source file, as the packer saw it. Retail's entries
    /// carry dense 1998 values; 0 is accepted by the reader.
    pub time: u32,
    /// Undetermined semantics — empty in every shipped archive. Written
    /// verbatim after the comment so a round-trip cannot lose them.
    pub keys: &'a [u32],
    pub data: &'a [u8],
}

impl<'a> ResourceSpec<'a> {
    /// A resource with no comment, id, time or keys.
    pub fn new(name: &'a str, kind: FourCc, data: &'a [u8]) -> ResourceSpec<'a> {
        ResourceSpec {
            name,
            comment: "",
            kind,
            id: 0,
            time: 0,
            keys: &[],
            data,
        }
    }
}

/// A directory being built. Obtained from [`RezBuilder::root`] or
/// [`DirSpec::dir`].
#[derive(Debug, Default)]
pub struct DirSpec<'a> {
    name: String,
    time: u32,
    dirs: Vec<DirSpec<'a>>,
    resources: Vec<ResourceSpec<'a>>,
}

impl<'a> DirSpec<'a> {
    /// This directory's name. Empty for the root, which has no entry anywhere.
    pub fn name(&self) -> &str {
        &self.name
    }

    /// Set the `time` field of this directory's entry in its parent.
    pub fn set_time(&mut self, time: u32) -> &mut Self {
        self.time = time;
        self
    }

    /// The child directory of this name, creating it if absent. Matching is
    /// case-insensitive, because retail's directory hash can be
    /// (`CRezDir::ReadDirBlock` @0x13a755 passes `mgr->case_flag == 0` to the
    /// lookup), so two names differing only in case are one directory.
    pub fn dir(&mut self, name: &str) -> Result<&mut DirSpec<'a>, WriteError> {
        check_name(name)?;
        let at = match self
            .dirs
            .iter()
            .position(|d| d.name.eq_ignore_ascii_case(name))
        {
            Some(i) => i,
            None => {
                self.dirs.push(DirSpec {
                    name: String::from(name),
                    ..DirSpec::default()
                });
                self.dirs.len() - 1
            }
        };
        Ok(&mut self.dirs[at])
    }

    /// Resolve a `\`- or `/`-separated directory path, creating as needed.
    pub fn dir_path(&mut self, path: &str) -> Result<&mut DirSpec<'a>, WriteError> {
        let mut here = self;
        for part in path.split(['\\', '/']).filter(|p| !p.is_empty()) {
            here = here.dir(part)?;
        }
        Ok(here)
    }

    /// Add a resource. Fails on a `(name, type)` collision with an existing one.
    pub fn add(&mut self, res: ResourceSpec<'a>) -> Result<&mut Self, WriteError> {
        check_name(res.name)?;
        if res.comment.bytes().any(|c| !(0x20..0x7f).contains(&c)) {
            return Err(WriteError::BadName(String::from(res.comment)));
        }
        if self
            .resources
            .iter()
            .any(|r| r.kind == res.kind && r.name.eq_ignore_ascii_case(res.name))
        {
            return Err(WriteError::Duplicate {
                dir: String::from(&self.name),
                name: String::from(res.name),
            });
        }
        self.resources.push(res);
        Ok(self)
    }

    /// How many resources this directory holds directly.
    pub fn len(&self) -> usize {
        self.resources.len()
    }

    pub fn is_empty(&self) -> bool {
        self.resources.is_empty() && self.dirs.is_empty()
    }
}

/// Builds a REZ v1 image.
///
/// ```
/// use gruntz_rez::{FourCc, Rez};
/// use gruntz_rez::write::{RezBuilder, ResourceSpec};
///
/// let mut b = RezBuilder::new();
/// b.root()
///     .dir_path("AREA2\\IMAGEZ")
///     .unwrap()
///     .add(ResourceSpec::new("TREE2", FourCc::from_tag("PID"), b"pixels"))
///     .unwrap();
/// let image = b.finish().unwrap();
///
/// let rez = Rez::new(&image).unwrap();
/// assert_eq!(rez.validate().unwrap(), 1);
/// rez.is_contiguous().unwrap();
/// let r = rez.resources().next().unwrap().unwrap();
/// assert_eq!(r.path().to_string(), "AREA2\\IMAGEZ\\TREE2");
/// assert_eq!(r.data(rez.bytes()), b"pixels");
/// ```
#[derive(Debug)]
pub struct RezBuilder<'a> {
    root: DirSpec<'a>,
    banner: [u8; 127],
    time: u32,
    root_time: u32,
}

impl Default for RezBuilder<'_> {
    fn default() -> Self {
        Self::new()
    }
}

impl<'a> RezBuilder<'a> {
    pub fn new() -> RezBuilder<'a> {
        RezBuilder {
            root: DirSpec::default(),
            banner: BANNER,
            time: 0,
            root_time: 0,
        }
    }

    /// The root directory. It has no name and no entry of its own; the header's
    /// `root_dir_pos` / `root_dir_size` point straight at its body.
    pub fn root(&mut self) -> &mut DirSpec<'a> {
        &mut self.root
    }

    /// The header's `time` — a `time_t`, 0x366c4c5c (1998-12-07) in retail.
    pub fn set_time(&mut self, time: u32) -> &mut Self {
        self.time = time;
        self
    }

    /// The header's `root_dir_time`.
    ///
    /// Its encoding is **undetermined**: it is not a `time_t` like `time` is.
    /// Retail `Gruntz.REZ` carries 0x0012fd1c and both `GRUNTZ.VRZ` and the
    /// demo carry 0x0040c9d8 — a stack address and an image-base address
    /// respectively, and identical across two archives built two days apart.
    /// Nothing in `GRUNTZ.EXE` reads it back except to hand it to the root
    /// `CRezDir` constructor as that directory's `time` (@0x13b062).
    /// Defaults to 0.
    pub fn set_root_time(&mut self, time: u32) -> &mut Self {
        self.root_time = time;
        self
    }

    /// Replace the two banner text lines. Each is padded with spaces to
    /// [`BANNER_LINE`]; the CRLFs and the 0x1a are not yours to change.
    pub fn set_banner(&mut self, line1: &str, line2: &str) -> Result<&mut Self, WriteError> {
        let ok = |s: &str| s.len() <= BANNER_LINE && s.bytes().all(|c| (0x20..0x7f).contains(&c));
        if !ok(line1) || !ok(line2) {
            return Err(WriteError::BadBanner);
        }
        let mut b = [b' '; 127];
        b[0x00] = b'\r';
        b[0x01] = b'\n';
        b[0x02..0x02 + line1.len()].copy_from_slice(line1.as_bytes());
        b[0x3e] = b'\r';
        b[0x3f] = b'\n';
        b[0x40..0x40 + line2.len()].copy_from_slice(line2.as_bytes());
        b[0x7c] = b'\r';
        b[0x7d] = b'\n';
        b[0x7e] = 0x1a;
        self.banner = b;
        Ok(self)
    }

    /// Lay the archive out and return the image.
    pub fn finish(&self) -> Result<Vec<u8>, WriteError> {
        let mut out = Vec::with_capacity(HEADER_SIZE + self.payload_bytes());
        out.resize(HEADER_SIZE, 0);

        // Pass 1 — payloads. Depth first, a directory's own resources in one
        // run before any descendant's, which is what makes is_sorted honest.
        let mut placed = place(&self.root, &mut out, 1)?;
        let next_write_pos = fit_u32(out.len())?;

        // Pass 2 — bodies, children before parents so a parent's entry can name
        // its child's final position.
        emit(&mut placed, &mut out)?;
        let (root_pos, root_size) = (placed.pos, placed.size);

        // Pass 3 — the header, now that every offset is known.
        let mut largest = Largest::default();
        measure(&self.root, &mut largest);
        out[..127].copy_from_slice(&self.banner);
        let f = &mut out[HEADER_SIZE - 41..];
        put(f, 0, 1); // version
        put(f, 4, root_pos);
        put(f, 8, root_size);
        put(f, 12, self.root_time);
        put(f, 16, next_write_pos);
        put(f, 20, self.time);
        put(f, 24, largest.key_ary);
        put(f, 28, largest.dir_name);
        put(f, 32, largest.rez_name);
        put(f, 36, largest.comment);
        f[40] = 1; // is_sorted — true by construction, see the module docs
        Ok(out)
    }

    fn payload_bytes(&self) -> usize {
        fn walk(d: &DirSpec<'_>) -> usize {
            d.resources.iter().map(|r| r.data.len()).sum::<usize>()
                + d.dirs.iter().map(walk).sum::<usize>()
        }
        walk(&self.root)
    }
}

/// A directory whose payloads are placed, awaiting its own body offset.
struct Placed<'s, 'a> {
    spec: &'s DirSpec<'a>,
    /// Payload offset of `spec.resources[i]`.
    res_pos: Vec<u32>,
    kids: Vec<Placed<'s, 'a>>,
    pos: u32,
    size: u32,
}

fn place<'s, 'a>(
    spec: &'s DirSpec<'a>,
    out: &mut Vec<u8>,
    depth: usize,
) -> Result<Placed<'s, 'a>, WriteError> {
    if depth > MAX_DEPTH {
        return Err(WriteError::TooDeep(String::from(&spec.name)));
    }
    let mut res_pos = Vec::with_capacity(spec.resources.len());
    for r in &spec.resources {
        res_pos.push(fit_u32(out.len())?);
        out.extend_from_slice(r.data);
    }
    let mut kids = Vec::with_capacity(spec.dirs.len());
    for d in &spec.dirs {
        kids.push(place(d, out, depth + 1)?);
    }
    Ok(Placed {
        spec,
        res_pos,
        kids,
        pos: 0,
        size: 0,
    })
}

fn emit(p: &mut Placed<'_, '_>, out: &mut Vec<u8>) -> Result<(), WriteError> {
    for k in &mut p.kids {
        emit(k, out)?;
    }
    let start = fit_u32(out.len())?;
    // Retail writes subdirectory entries first, then resources: all 20 mixed
    // bodies in `Gruntz.REZ` and all 11 in the demo are `d*r*`, never `r*d*`
    // and never interleaved.
    for k in &p.kids {
        out.extend_from_slice(&KIND_DIRECTORY.to_le_bytes());
        out.extend_from_slice(&k.pos.to_le_bytes());
        out.extend_from_slice(&k.size.to_le_bytes());
        out.extend_from_slice(&k.spec.time.to_le_bytes());
        out.extend_from_slice(k.spec.name.as_bytes());
        out.push(0);
    }
    for (i, r) in p.spec.resources.iter().enumerate() {
        let size = fit_u32(r.data.len())?;
        out.extend_from_slice(&KIND_RESOURCE.to_le_bytes());
        out.extend_from_slice(&p.res_pos[i].to_le_bytes());
        out.extend_from_slice(&size.to_le_bytes());
        out.extend_from_slice(&r.time.to_le_bytes());
        out.extend_from_slice(&r.id.to_le_bytes());
        out.extend_from_slice(&r.kind.0.to_le_bytes());
        out.extend_from_slice(&fit_u32(r.keys.len())?.to_le_bytes());
        out.extend_from_slice(r.name.as_bytes());
        out.push(0);
        out.extend_from_slice(r.comment.as_bytes());
        out.push(0);
        for k in r.keys {
            out.extend_from_slice(&k.to_le_bytes());
        }
    }
    p.pos = start;
    p.size = fit_u32(out.len())? - start;
    Ok(())
}

/// The four `largest_*` header fields.
///
/// They are maxima over what was written, not limits. Measured against all
/// three shipped archives, each name field is `max(strlen) + 1` — the buffer
/// retail allocates for the string, NUL included (retail `Gruntz.REZ`: longest
/// directory name 20 -> 21, longest resource name 24 -> 25). `largest_comment`
/// is 0 there while every comment is the empty string, which fixes the other
/// half of the rule: an absent (empty) string does not participate.
///
/// `largest_key_ary` is written as the largest **element count**. With no
/// archive carrying a key array, count-versus-bytes is undetermined; it is
/// zero either way for anything this writer has been asked to produce.
#[derive(Default)]
struct Largest {
    key_ary: u32,
    dir_name: u32,
    rez_name: u32,
    comment: u32,
}

fn measure(d: &DirSpec<'_>, m: &mut Largest) {
    for k in &d.dirs {
        m.dir_name = m.dir_name.max(saturating_u32(k.name.len()) + 1);
        measure(k, m);
    }
    for r in &d.resources {
        m.rez_name = m.rez_name.max(saturating_u32(r.name.len()) + 1);
        if !r.comment.is_empty() {
            m.comment = m.comment.max(saturating_u32(r.comment.len()) + 1);
        }
        m.key_ary = m.key_ary.max(saturating_u32(r.keys.len()));
    }
}

fn saturating_u32(n: usize) -> u32 {
    u32::try_from(n).unwrap_or(u32::MAX - 1)
}

fn fit_u32(n: usize) -> Result<u32, WriteError> {
    u32::try_from(n).map_err(|_| WriteError::TooLarge)
}

fn put(buf: &mut [u8], at: usize, v: u32) {
    buf[at..at + 4].copy_from_slice(&v.to_le_bytes());
}

fn check_name(name: &str) -> Result<(), WriteError> {
    let bad = name.is_empty()
        || name.len() > 255
        || name
            .bytes()
            .any(|c| c == b'\\' || c == b'/' || !(0x20..0x7f).contains(&c));
    if bad {
        return Err(WriteError::BadName(String::from(name)));
    }
    Ok(())
}
