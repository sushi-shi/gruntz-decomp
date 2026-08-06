//! `fecls` — list and extract Monolith FEC movie archives.

use std::path::{Path, PathBuf};
use std::process::ExitCode;

use clap::{Parser, Subcommand};
use gruntz_rez::fec::{Fec, NAME_CAPACITY};

#[derive(Parser)]
#[command(
    name = "fecls",
    about = "List and extract Monolith FEC 1.1 movie archives"
)]
struct Cli {
    /// GRUNTZ.FEC or GRUNTZLO.FEC.
    archive: PathBuf,
    #[command(subcommand)]
    cmd: Option<Cmd>,
}

#[derive(Subcommand)]
enum Cmd {
    /// List every embedded file (the default).
    List,
    /// Extract one embedded file by decoded name.
    Extract { name: String, out: PathBuf },
    /// Extract every embedded file into a directory.
    ExtractAll { out: PathBuf },
}

fn main() -> ExitCode {
    match run() {
        Ok(()) => ExitCode::SUCCESS,
        Err(error) => {
            eprintln!("fecls: {error}");
            ExitCode::FAILURE
        }
    }
}

fn run() -> Result<(), Box<dyn std::error::Error>> {
    let cli = Cli::parse();
    let bytes = std::fs::read(&cli.archive)?;
    let fec = Fec::new(&bytes)?;
    let count = fec.validate()?;
    eprintln!(
        "[fecls] {} : {} bytes, FEC {}.{}, {} files",
        cli.archive.display(),
        bytes.len(),
        fec.header.version_major,
        fec.header.version_minor,
        count
    );

    match cli.cmd.unwrap_or(Cmd::List) {
        Cmd::List => list(fec)?,
        Cmd::Extract { name, out } => extract(fec, &name, &out)?,
        Cmd::ExtractAll { out } => extract_all(fec, &out)?,
    }
    Ok(())
}

fn list(fec: Fec<'_>) -> Result<(), Box<dyn std::error::Error>> {
    println!("{:>5} {:>12} {:>12} NAME", "INDEX", "BYTES", "OFFSET");
    let mut name = [0u8; NAME_CAPACITY];
    for entry in fec.entries() {
        let entry = entry?;
        let decoded = entry.decoded_name(&mut name)?;
        println!(
            "{:>5} {:>12} {:>12x} {decoded}",
            entry.index,
            entry.payload.len(),
            entry.payload_offset
        );
    }
    Ok(())
}

fn extract(fec: Fec<'_>, wanted: &str, out: &Path) -> Result<(), Box<dyn std::error::Error>> {
    let mut name = [0u8; NAME_CAPACITY];
    for entry in fec.entries() {
        let entry = entry?;
        let decoded = entry.decoded_name(&mut name)?;
        if decoded.eq_ignore_ascii_case(wanted) {
            if let Some(parent) = out.parent() {
                std::fs::create_dir_all(parent)?;
            }
            std::fs::write(out, entry.payload)?;
            eprintln!("[fecls] {decoded} -> {}", out.display());
            return Ok(());
        }
    }
    Err(std::io::Error::new(
        std::io::ErrorKind::NotFound,
        format!("no embedded file named {wanted}"),
    )
    .into())
}

fn extract_all(fec: Fec<'_>, out: &Path) -> Result<(), Box<dyn std::error::Error>> {
    std::fs::create_dir_all(out)?;
    let mut name = [0u8; NAME_CAPACITY];
    for entry in fec.entries() {
        let entry = entry?;
        let decoded = entry.decoded_name(&mut name)?;
        let relative = Path::new(decoded);
        if relative.components().count() != 1 || relative.file_name().is_none() {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidData,
                format!("unsafe embedded file name {decoded:?}"),
            )
            .into());
        }
        let destination = out.join(relative);
        std::fs::write(&destination, entry.payload)?;
        eprintln!("[fecls] {decoded} -> {}", destination.display());
    }
    Ok(())
}
