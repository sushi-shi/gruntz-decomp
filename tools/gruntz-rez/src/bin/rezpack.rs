//! `rezpack` — build a Monolith REZ v1 archive, and prove the writer against
//! the reader.
//!
//! Three commands, in order of how much they prove:
//!
//! * `roundtrip` — decode an existing archive, re-encode it from the decoded
//!   tree, re-parse the result with the same validated reader, and compare
//!   structure and payload bytes exactly. This is the backbone: it exercises
//!   every field on 21 303 resources at once and needs nothing but the file.
//! * `unpack` / `pack` — the on-disk form. `unpack` writes `NAME.TYPE` files
//!   plus a `REZ.TSV` manifest carrying the fields a filesystem cannot hold
//!   (id, time, comment, keys); `pack` reads the manifest back, so the disk
//!   round-trip is lossless too.
//! * `check` — validate an archive and test the `is_sorted` predicate
//!   (`CRezDir::Load` @0x13a0f0) that a writer must not lie about.
//!
//! The library is `no_std`; this binary owns the `std::fs` and the printing.

use std::collections::BTreeMap;
use std::io::Write;
use std::path::{Path, PathBuf};
use std::process::ExitCode;

use clap::{Parser, Subcommand};
use gruntz_rez::write::{ResourceSpec, RezBuilder};
use gruntz_rez::{FourCc, Rez};

#[derive(Parser)]
#[command(
    name = "rezpack",
    about = "Build a Monolith REZ v1 archive",
    long_about = "Writes REZ v1 archives and checks them with the reader in the \
                  same crate. `roundtrip` is the real test: decode -> re-encode \
                  -> re-parse -> compare every resource's path, type, id, time, \
                  comment and payload bytes."
)]
struct Cli {
    #[command(subcommand)]
    cmd: Cmd,
}

#[derive(Subcommand)]
enum Cmd {
    /// Decode ARCHIVE, re-encode it, re-parse, and compare.
    Roundtrip {
        archive: PathBuf,
        /// Also write the re-encoded image here.
        #[arg(long)]
        out: Option<PathBuf>,
    },
    /// Extract ARCHIVE into DIR, with a REZ.TSV manifest.
    Unpack { archive: PathBuf, dir: PathBuf },
    /// Build ARCHIVE from DIR (reads DIR/REZ.TSV if present).
    Pack {
        dir: PathBuf,
        archive: PathBuf,
        /// Header `time` field, decimal time_t. Default 0.
        #[arg(long, default_value_t = 0)]
        time: u32,
    },
    /// Validate ARCHIVE and test the is_sorted (contiguity) predicate.
    Check { archive: PathBuf },
}

const MANIFEST: &str = "REZ.TSV";

fn main() -> ExitCode {
    match run(Cli::parse()) {
        Ok(()) => ExitCode::SUCCESS,
        Err(e) => {
            eprintln!("rezpack: {e}");
            ExitCode::FAILURE
        }
    }
}

fn run(cli: Cli) -> Result<(), String> {
    match cli.cmd {
        Cmd::Roundtrip { archive, out } => roundtrip(&archive, out.as_deref()),
        Cmd::Unpack { archive, dir } => unpack(&archive, &dir),
        Cmd::Pack {
            dir,
            archive,
            time,
        } => pack(&dir, &archive, time),
        Cmd::Check { archive } => check(&archive),
    }
}

fn read(path: &Path) -> Result<Vec<u8>, String> {
    std::fs::read(path).map_err(|e| format!("{}: {e}", path.display()))
}

/// Every field of one resource, owned, so the source image can be dropped.
struct Row {
    dirs: Vec<String>,
    name: String,
    kind: FourCc,
    id: u32,
    time: u32,
    comment: String,
    keys: Vec<u32>,
    data: Vec<u8>,
}

impl Row {
    fn path(&self) -> String {
        let mut s = self.dirs.join("\\");
        if !s.is_empty() {
            s.push('\\');
        }
        s.push_str(&self.name);
        s
    }
}

/// The whole archive, owned: resources, every directory (including the ones
/// with nothing beneath them), and the two header times.
struct Decoded {
    rows: Vec<Row>,
    /// `(components, time)` for every directory, in depth-first order — parents
    /// always before children, which is what `build` relies on.
    dirs: Vec<(Vec<String>, u32)>,
    time: u32,
    root_dir_time: u32,
}

fn decode(bytes: &[u8]) -> Result<Decoded, String> {
    let rez = Rez::new(bytes).map_err(|e| e.to_string())?;
    let mut rows = Vec::new();
    for r in rez.resources() {
        let r = r.map_err(|e| e.to_string())?;
        rows.push(Row {
            dirs: r.dirs.as_slice().iter().map(|s| (*s).to_owned()).collect(),
            name: r.name.to_owned(),
            kind: r.kind,
            id: r.id,
            time: r.time,
            comment: r.comment.to_owned(),
            keys: r.keys().collect(),
            data: r.data(rez.bytes()).to_vec(),
        });
    }
    let mut dirs = Vec::new();
    for d in rez.directories() {
        let d = d.map_err(|e| e.to_string())?;
        let mut parts: Vec<String> = d.parents.as_slice().iter().map(|s| (*s).to_owned()).collect();
        parts.push(d.name.to_owned());
        dirs.push((parts, d.time));
    }
    Ok(Decoded {
        rows,
        dirs,
        time: rez.header.time,
        root_dir_time: rez.header.root_dir_time,
    })
}

fn build(d: &Decoded) -> Result<Vec<u8>, String> {
    let mut b = RezBuilder::new();
    b.set_time(d.time);
    b.set_root_time(d.root_dir_time);
    // Directories first, in the source's own depth-first order, so an empty one
    // survives and every one keeps its `time`.
    for (parts, time) in &d.dirs {
        let mut dir = b.root();
        for p in parts {
            dir = dir.dir(p).map_err(|e| e.to_string())?;
        }
        dir.set_time(*time);
    }
    for row in &d.rows {
        let mut dir = b.root();
        for p in &row.dirs {
            dir = dir.dir(p).map_err(|e| e.to_string())?;
        }
        dir.add(ResourceSpec {
            name: &row.name,
            comment: &row.comment,
            kind: row.kind,
            id: row.id,
            time: row.time,
            keys: &row.keys,
            data: &row.data,
        })
        .map_err(|e| e.to_string())?;
    }
    b.finish().map_err(|e| e.to_string())
}

fn roundtrip(archive: &Path, out: Option<&Path>) -> Result<(), String> {
    let bytes = read(archive)?;
    let src_d = decode(&bytes)?;
    println!(
        "source     {} : {} bytes, {} resources, {} directories",
        archive.display(),
        bytes.len(),
        src_d.rows.len(),
        src_d.dirs.len()
    );

    let image = build(&src_d)?;
    let back = decode(&image)?;
    let rez = Rez::new(&image).map_err(|e| e.to_string())?;
    rez.validate().map_err(|e| e.to_string())?;
    rez.is_contiguous().map_err(|e| e.to_string())?;

    println!(
        "re-encoded {} bytes, {} resources, {} directories",
        image.len(),
        back.rows.len(),
        back.dirs.len()
    );

    // Structure and payloads must agree exactly. Sibling ORDER is not part of
    // the format's meaning (retail's own order is hash-bucket order, and
    // lookups go through the hash), so compare as maps keyed by path.
    let key = |r: &Row| (r.path(), r.kind.0);
    let mut a: BTreeMap<_, _> = src_d.rows.iter().map(|r| (key(r), r)).collect();
    let b: BTreeMap<_, _> = back.rows.iter().map(|r| (key(r), r)).collect();
    if a.len() != src_d.rows.len() {
        return Err("source has duplicate (path, type) keys".into());
    }
    let mut bad = 0usize;
    for (k, rb) in &b {
        let Some(ra) = a.remove(k) else {
            println!("  ONLY IN RE-ENCODED: {}", rb.path());
            bad += 1;
            continue;
        };
        for (what, ok) in [
            ("payload", ra.data == rb.data),
            ("id", ra.id == rb.id),
            ("time", ra.time == rb.time),
            ("comment", ra.comment == rb.comment),
            ("keys", ra.keys == rb.keys),
        ] {
            if !ok {
                println!("  {what} DIFFERS: {}", ra.path());
                bad += 1;
            }
        }
    }
    for (_, ra) in a {
        println!("  MISSING FROM RE-ENCODED: {}", ra.path());
        bad += 1;
    }
    let da: BTreeMap<_, _> = src_d.dirs.iter().map(|(p, t)| (p.join("\\"), *t)).collect();
    let db: BTreeMap<_, _> = back.dirs.iter().map(|(p, t)| (p.join("\\"), *t)).collect();
    if da != db {
        for (p, t) in &da {
            match db.get(p) {
                None => println!("  DIRECTORY MISSING: {p}"),
                Some(t2) if t2 != t => println!("  DIRECTORY time DIFFERS: {p} {t} -> {t2}"),
                _ => continue,
            }
            bad += 1;
        }
        for p in db.keys() {
            if !da.contains_key(p) {
                println!("  DIRECTORY ONLY IN RE-ENCODED: {p}");
                bad += 1;
            }
        }
    }
    if back.time != src_d.time {
        println!("  header time differs: {} -> {}", src_d.time, back.time);
        bad += 1;
    }
    if back.root_dir_time != src_d.root_dir_time {
        println!(
            "  root_dir_time differs: {:#x} -> {:#x}",
            src_d.root_dir_time, back.root_dir_time
        );
        bad += 1;
    }

    println!();
    println!("structural differences vs retail (expected, not defects):");
    let src = Rez::new(&bytes).map_err(|e| e.to_string())?;
    println!(
        "  file size        {} -> {}   ({} bytes smaller)",
        bytes.len(),
        image.len(),
        bytes.len() as i64 - image.len() as i64
    );
    println!(
        "  root_dir_pos     {:#x} -> {:#x}",
        src.header.root_dir_pos, rez.header.root_dir_pos
    );
    println!(
        "  next_write_pos   {} -> {}",
        src.header.next_write_pos, rez.header.next_write_pos
    );
    println!(
        "  is_sorted        {} -> {}",
        src.header.is_sorted, rez.header.is_sorted
    );

    if let Some(p) = out {
        std::fs::write(p, &image).map_err(|e| format!("{}: {e}", p.display()))?;
        println!("  wrote            {}", p.display());
    }

    if bad == 0 {
        println!();
        println!(
            "ROUND TRIP OK: {} resources and {} directories identical",
            src_d.rows.len(),
            src_d.dirs.len()
        );
        Ok(())
    } else {
        Err(format!("{bad} differences"))
    }
}

fn check(archive: &Path) -> Result<(), String> {
    let bytes = read(archive)?;
    let rez = Rez::new(&bytes).map_err(|e| e.to_string())?;
    let n = rez.validate().map_err(|e| e.to_string())?;
    let h = &rez.header;
    println!("{}: {} bytes, {n} resources", archive.display(), bytes.len());
    println!("  version          {}", h.version);
    println!("  root_dir         {:#x} + {:#x}", h.root_dir_pos, h.root_dir_size);
    println!("  root_dir_time    {:#x}", h.root_dir_time);
    println!("  next_write_pos   {}", h.next_write_pos);
    println!("  time             {}", h.time);
    println!("  largest_key_ary  {}", h.largest_key_ary);
    println!("  largest_dir_name {}", h.largest_dir_name_size);
    println!("  largest_rez_name {}", h.largest_rez_name_size);
    println!("  largest_comment  {}", h.largest_comment_size);
    println!("  is_sorted        {}", h.is_sorted);
    match rez.is_contiguous() {
        Ok(()) => {
            println!("  contiguity       OK - every directory's resources tile one span");
            if h.is_sorted == 0 {
                println!("  NOTE: is_sorted is 0 although the layout would allow 1");
            }
            Ok(())
        }
        Err(e) => {
            println!("  contiguity       FAILED - {e}");
            if h.is_sorted != 0 {
                Err("is_sorted = 1 but the layout does not support CRezDir::Load".into())
            } else {
                Ok(())
            }
        }
    }
}

fn unpack(archive: &Path, dir: &Path) -> Result<(), String> {
    let bytes = read(archive)?;
    let d = decode(&bytes)?;
    std::fs::create_dir_all(dir).map_err(|e| format!("{}: {e}", dir.display()))?;
    let mut manifest = String::new();
    manifest.push_str("# gruntz-rez manifest\theader_time\troot_dir_time\n");
    manifest.push_str(&format!("#!\t{}\t{}\n", d.time, d.root_dir_time));
    // Directories get a row too, so an empty one and every `time` survive the
    // trip through the filesystem.
    for (parts, time) in &d.dirs {
        manifest.push_str(&format!("#d\t{}\t{time}\n", parts.join("\\")));
        let mut out = dir.to_path_buf();
        for p in parts {
            out.push(p);
        }
        std::fs::create_dir_all(&out).map_err(|e| format!("{}: {e}", out.display()))?;
    }
    manifest.push_str("path\ttype\tid\ttime\tkeys\tcomment\n");
    for r in &d.rows {
        let mut out = dir.to_path_buf();
        for d in &r.dirs {
            out.push(d);
        }
        std::fs::create_dir_all(&out).map_err(|e| format!("{}: {e}", out.display()))?;
        out.push(file_name(&r.name, r.kind));
        std::fs::write(&out, &r.data).map_err(|e| format!("{}: {e}", out.display()))?;
        let keys = r
            .keys
            .iter()
            .map(|k| k.to_string())
            .collect::<Vec<_>>()
            .join(",");
        manifest.push_str(&format!(
            "{}\t{}\t{}\t{}\t{}\t{}\n",
            r.path(),
            r.kind,
            r.id,
            r.time,
            keys,
            r.comment
        ));
    }
    let mpath = dir.join(MANIFEST);
    std::fs::write(&mpath, manifest).map_err(|e| format!("{}: {e}", mpath.display()))?;
    println!(
        "unpacked {} resources and {} directories into {} (+ {MANIFEST})",
        d.rows.len(),
        d.dirs.len(),
        dir.display()
    );
    Ok(())
}

fn file_name(name: &str, kind: FourCc) -> String {
    let tag = kind.to_string();
    if tag.is_empty() || tag == "?" {
        name.to_owned()
    } else {
        format!("{name}.{tag}")
    }
}

/// One line of the manifest: everything the filesystem cannot carry.
///
/// `rank` is the row's position, which is the source archive's own traversal
/// order. Honouring it makes `unpack` -> `pack` reproduce the same image the
/// in-memory round trip produces, byte for byte; without it the two differ
/// (same size, different payload order) because `read_dir` order is not the
/// archive's.
#[derive(Default, Clone)]
struct Meta {
    id: u32,
    time: u32,
    keys: Vec<u32>,
    comment: String,
    rank: usize,
}

fn pack(dir: &Path, archive: &Path, time_arg: u32) -> Result<(), String> {
    let mut meta: BTreeMap<(String, u32), Meta> = BTreeMap::new();
    // Directory rows, in manifest order so parents precede children.
    let mut dir_meta: Vec<(Vec<String>, u32)> = Vec::new();
    let mut header_time = time_arg;
    let mut root_dir_time = 0u32;
    let mpath = dir.join(MANIFEST);
    if let Ok(text) = std::fs::read_to_string(&mpath) {
        for line in text.lines() {
            if let Some(rest) = line.strip_prefix("#!\t") {
                let mut it = rest.split('\t');
                if let Some(t) = it.next() {
                    if time_arg == 0 {
                        header_time = t.parse().unwrap_or(0);
                    }
                }
                if let Some(t) = it.next() {
                    root_dir_time = t.parse().unwrap_or(0);
                }
                continue;
            }
            if let Some(rest) = line.strip_prefix("#d\t") {
                let mut it = rest.rsplitn(2, '\t');
                let time = it.next().and_then(|t| t.parse().ok()).unwrap_or(0);
                if let Some(p) = it.next() {
                    dir_meta.push((p.split('\\').map(str::to_owned).collect(), time));
                }
                continue;
            }
            if line.starts_with('#') || line.starts_with("path\t") {
                continue;
            }
            let f: Vec<&str> = line.split('\t').collect();
            if f.len() < 6 {
                continue;
            }
            let keys = f[4]
                .split(',')
                .filter(|s| !s.is_empty())
                .filter_map(|s| s.parse().ok())
                .collect();
            let rank = meta.len();
            meta.insert(
                (f[0].to_ascii_uppercase(), FourCc::from_tag(f[1]).0),
                Meta {
                    id: f[2].parse().unwrap_or(0),
                    time: f[3].parse().unwrap_or(0),
                    keys,
                    comment: f[5].to_owned(),
                    rank,
                },
            );
        }
    }

    // Read every file up front: the builder borrows payloads, so they must
    // outlive it. The alternative is copying them into the builder, which for a
    // 77 MB archive would double the peak instead of halving the code.
    let mut files: Vec<(Vec<String>, String, FourCc, Vec<u8>)> = Vec::new();
    let mut on_disk: Vec<Vec<String>> = Vec::new();
    collect(dir, &mut Vec::new(), &mut files, &mut on_disk)?;
    let rank_of = |dirs: &Vec<String>, name: &String, kind: FourCc| {
        let mut p = dirs.join("\\");
        if !p.is_empty() {
            p.push('\\');
        }
        p.push_str(name);
        meta.get(&(p.to_ascii_uppercase(), kind.0))
            .map_or(usize::MAX, |m| m.rank)
    };
    files.sort_by_key(|(d, n, k, _)| (rank_of(d, n, *k), d.clone(), n.clone(), k.0));
    // A directory that exists on disk but not in the manifest still belongs in
    // the archive; it just has no recorded time.
    on_disk.sort();
    for d in on_disk {
        if !dir_meta.iter().any(|(p, _)| *p == d) {
            dir_meta.push((d, 0));
        }
    }

    let empty = Meta::default();
    let specs: Vec<(&Vec<String>, ResourceSpec<'_>, &Meta)> = files
        .iter()
        .map(|(dirs, name, kind, data)| {
            let mut p = dirs.join("\\");
            if !p.is_empty() {
                p.push('\\');
            }
            p.push_str(name);
            let m = meta
                .get(&(p.to_ascii_uppercase(), kind.0))
                .unwrap_or(&empty);
            (
                dirs,
                ResourceSpec {
                    name,
                    comment: &m.comment,
                    kind: *kind,
                    id: m.id,
                    time: m.time,
                    keys: &m.keys,
                    data,
                },
                m,
            )
        })
        .collect();

    let mut b = RezBuilder::new();
    b.set_time(header_time);
    b.set_root_time(root_dir_time);
    for (parts, time) in &dir_meta {
        let mut d = b.root();
        for part in parts {
            d = d.dir(part).map_err(|e| e.to_string())?;
        }
        d.set_time(*time);
    }
    for (dirs, spec, _) in &specs {
        let mut d = b.root();
        for part in dirs.iter() {
            d = d.dir(part).map_err(|e| e.to_string())?;
        }
        d.add(*spec).map_err(|e| e.to_string())?;
    }
    let image = b.finish().map_err(|e| e.to_string())?;
    std::fs::write(archive, &image).map_err(|e| format!("{}: {e}", archive.display()))?;

    let rez = Rez::new(&image).map_err(|e| e.to_string())?;
    let n = rez.validate().map_err(|e| e.to_string())?;
    rez.is_contiguous().map_err(|e| e.to_string())?;
    let mut out = std::io::stdout();
    let _ = writeln!(
        out,
        "wrote {} : {} bytes, {n} resources, re-read and contiguity-checked",
        archive.display(),
        image.len()
    );
    Ok(())
}

fn collect(
    dir: &Path,
    stack: &mut Vec<String>,
    out: &mut Vec<(Vec<String>, String, FourCc, Vec<u8>)>,
    dirs: &mut Vec<Vec<String>>,
) -> Result<(), String> {
    let rd = std::fs::read_dir(dir).map_err(|e| format!("{}: {e}", dir.display()))?;
    for e in rd {
        let e = e.map_err(|e| format!("{}: {e}", dir.display()))?;
        let name = e.file_name().to_string_lossy().into_owned();
        let path = e.path();
        if path.is_dir() {
            stack.push(name.to_ascii_uppercase());
            dirs.push(stack.clone());
            collect(&path, stack, out, dirs)?;
            stack.pop();
        } else {
            if name == MANIFEST {
                continue;
            }
            let (stem, tag) = match name.rsplit_once('.') {
                Some((s, t)) if !s.is_empty() => (s.to_owned(), t.to_ascii_uppercase()),
                _ => (name.clone(), String::new()),
            };
            let data = read(&path)?;
            out.push((
                stack.clone(),
                stem.to_ascii_uppercase(),
                FourCc::from_tag(&tag),
                data,
            ));
        }
    }
    Ok(())
}
