//! `rezls` — walk a Monolith REZ/VRZ v1 archive.
//!
//! The library is `no_std` and borrows the archive; this binary owns the
//! `std::fs::read` and the printing.

use std::collections::BTreeMap;
use std::path::PathBuf;
use std::process::ExitCode;

use clap::{Parser, Subcommand};
use gruntz_rez::Rez;

#[derive(Parser)]
#[command(
    name = "rezls",
    about = "List / extract from a Monolith REZ/VRZ v1 archive",
    long_about = "Walks a Gruntz .REZ or .VRZ. The directory layout is validated \
                  structurally: every directory body must end exactly on its \
                  declared size, so a malformed archive (or a wrong idea of the \
                  entry layout) is reported rather than silently tolerated."
)]
struct Cli {
    /// A .REZ game-resource or .VRZ voice-resource archive.
    archive: PathBuf,
    #[command(subcommand)]
    cmd: Option<Cmd>,
}

#[derive(Subcommand)]
enum Cmd {
    /// Count and total size per resource type (the default).
    Census,
    /// Print every resource whose path or type matches PATTERN.
    Grep {
        /// Case-insensitive substring of the path, or an exact 4CC.
        pattern: String,
    },
    /// Write one resource's bytes to a file.
    Extract {
        /// Full path, e.g. `AREA2\IMAGEZ\TREE2\FRAME001`.
        path: String,
        /// Destination file.
        out: PathBuf,
    },
    /// Write EVERY resource to a directory tree mirroring the archive.
    ///
    /// `AREA2\IMAGEZ\TREE2\FRAME001` of type PID becomes
    /// `<out>/AREA2/IMAGEZ/TREE2/FRAME001.PID`. The 4CC is appended as the
    /// extension because the archive stores type and name separately, and two
    /// resources in one directory may share a name across types.
    Unpack {
        /// Destination directory. Created if missing.
        out: PathBuf,
        /// Only unpack this 4CC (e.g. `PCX`, `WAV`, `WWD`). Repeatable.
        #[arg(long = "type", value_name = "4CC")]
        types: Vec<String>,
        /// List what would be written without writing anything.
        #[arg(long)]
        dry_run: bool,
    },
}

fn main() -> ExitCode {
    let cli = Cli::parse();
    let bytes = match std::fs::read(&cli.archive) {
        Ok(b) => b,
        Err(e) => {
            eprintln!("rezls: {}: {e}", cli.archive.display());
            return ExitCode::FAILURE;
        }
    };
    let rez = match Rez::new(&bytes) {
        Ok(r) => r,
        Err(e) => {
            eprintln!("rezls: {}: {e}", cli.archive.display());
            return ExitCode::FAILURE;
        }
    };
    let count = match rez.validate() {
        Ok(n) => n,
        Err(e) => {
            eprintln!("rezls: {}: {e}", cli.archive.display());
            return ExitCode::FAILURE;
        }
    };
    eprintln!(
        "[rezls] {} : {} bytes, {count} resources, root dir @{:#x}+{:#x}, sorted={}",
        cli.archive.display(),
        bytes.len(),
        rez.header.root_dir_pos,
        rez.header.root_dir_size,
        rez.header.is_sorted
    );

    match cli.cmd.unwrap_or(Cmd::Census) {
        Cmd::Census => {
            let mut by: BTreeMap<String, (usize, u64)> = BTreeMap::new();
            for r in rez.resources().flatten() {
                let e = by.entry(r.kind.to_string()).or_insert((0, 0));
                e.0 += 1;
                e.1 += u64::from(r.size);
            }
            println!("{:<12} {:>8} {:>14}", "TYPE", "COUNT", "BYTES");
            for (k, (n, b)) in by {
                println!("{k:<12} {n:>8} {b:>14}");
            }
        }
        Cmd::Grep { pattern } => {
            let up = pattern.to_ascii_uppercase();
            for r in rez.resources().flatten() {
                let path = r.path().to_string();
                let kind = r.kind.to_string();
                if path.to_ascii_uppercase().contains(&up) || kind == up {
                    println!("{kind:<8} {:>9} {:>10x} {path}", r.size, r.pos);
                }
            }
        }
        Cmd::Extract { path, out } => {
            let up = path.to_ascii_uppercase();
            let found = rez
                .resources()
                .flatten()
                .find(|r| r.path().to_string().to_ascii_uppercase() == up);
            match found {
                Some(r) => {
                    if let Err(e) = std::fs::write(&out, r.data(rez.bytes())) {
                        eprintln!("rezls: {}: {e}", out.display());
                        return ExitCode::FAILURE;
                    }
                }
                None => {
                    eprintln!("rezls: no such resource: {path}");
                    return ExitCode::FAILURE;
                }
            }
        }
        Cmd::Unpack {
            out,
            types,
            dry_run,
        } => {
            let want: Vec<String> = types.iter().map(|t| t.to_ascii_uppercase()).collect();
            let mut written = 0usize;
            let mut bytes_out = 0u64;
            let mut skipped = 0usize;
            for r in rez.resources().flatten() {
                let kind = r.kind.to_string();
                if !want.is_empty() && !want.contains(&kind) {
                    skipped += 1;
                    continue;
                }
                // The archive separates the name from the 4CC, and two
                // resources in one directory can share a name across types, so
                // the type becomes the extension rather than being dropped.
                let mut dst = out.clone();
                for d in r.dirs.as_slice() {
                    dst.push(d);
                }
                let leaf = if kind.is_empty() {
                    r.name.to_string()
                } else {
                    format!("{}.{kind}", r.name)
                };
                dst.push(leaf);
                if dry_run {
                    println!("{:>9}  {}", r.size, dst.display());
                } else {
                    if let Some(parent) = dst.parent() {
                        if let Err(e) = std::fs::create_dir_all(parent) {
                            eprintln!("rezls: {}: {e}", parent.display());
                            return ExitCode::FAILURE;
                        }
                    }
                    if let Err(e) = std::fs::write(&dst, r.data(rez.bytes())) {
                        eprintln!("rezls: {}: {e}", dst.display());
                        return ExitCode::FAILURE;
                    }
                }
                written += 1;
                bytes_out += u64::from(r.size);
            }
            eprintln!(
                "[rezls] {} {written} resource(s), {bytes_out} bytes{}",
                if dry_run { "would write" } else { "wrote" },
                if skipped > 0 {
                    format!(" ({skipped} skipped by --type)")
                } else {
                    String::new()
                }
            );
        }
    }
    ExitCode::SUCCESS
}
