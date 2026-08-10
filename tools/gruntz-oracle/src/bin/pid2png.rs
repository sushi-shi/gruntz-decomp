//! `pid2png` — render Gruntz `.PID` sprites as PNG.
//!
//! A PID stores palette INDICES, not colours. Most sprites therefore carry no
//! palette of their own and only become an image once one is supplied — which
//! is exactly how the game recolours a grunt's toy or tool: the same sprite
//! bytes drawn through `GRUNTZ\PALETTEZ\REDTOY.PAL` or `\BLUETOY.PAL`. That
//! makes `--palette` a creative control, not just a fallback, so any sprite can
//! be rendered under any palette in the archive.
//!
//! Output is an INDEXED PNG (colour type 3): one byte per pixel plus a 768-byte
//! `PLTE`, so a rendered sprite is about the size of the decoded original
//! rather than four times it. `--transparent` marks the header's fill index
//! fully transparent via `tRNS`.

use std::fs::File;
use std::io::BufWriter;
use std::path::{Path, PathBuf};
use std::process::ExitCode;

use clap::Parser;
use gruntz_codec::{pal, pid};

#[derive(Parser)]
#[command(
    name = "pid2png",
    about = "Render Gruntz .PID sprites as indexed PNG",
    long_about = "Decodes PID sprites and writes indexed PNGs.\n\n\
                  A PID holds palette INDICES, so colours come from elsewhere: \
                  a sprite with flags & 0x80 carries its own 768-byte VGA \
                  palette, and every other sprite needs --palette. Because the \
                  palette is external, ANY palette may be applied to ANY \
                  sprite - which is how the game itself recolours toyz and \
                  toolz (see GRUNTZ/PALETTEZ/*.PAL)."
)]
struct Cli {
    /// `.PID` files, or directories to walk when --recursive is given.
    #[arg(required = true)]
    inputs: Vec<PathBuf>,
    /// Destination directory. Input tree structure is preserved below it.
    #[arg(long, short)]
    out: PathBuf,
    /// A 768-byte `.PAL`. Overrides a sprite's embedded palette when both exist.
    #[arg(long)]
    palette: Option<PathBuf>,
    /// Walk directories for *.PID.
    #[arg(long, short)]
    recursive: bool,
    /// Mark the header's fill index fully transparent.
    #[arg(long)]
    transparent: bool,
    /// Use the CRezImage decoder's row-overrun rule (spill) instead of
    /// CDDSurface::RunDecode1's (carry). They differ only on malformed streams.
    #[arg(long)]
    spill: bool,
    /// Report what would be written without writing it.
    #[arg(long)]
    dry_run: bool,
}

fn collect(path: &Path, recursive: bool, into: &mut Vec<PathBuf>) -> std::io::Result<()> {
    if path.is_dir() {
        if !recursive {
            return Ok(());
        }
        let mut entries: Vec<_> = std::fs::read_dir(path)?.collect::<Result<_, _>>()?;
        entries.sort_by_key(std::fs::DirEntry::path);
        for e in entries {
            collect(&e.path(), recursive, into)?;
        }
    } else if path
        .extension()
        .is_some_and(|e| e.eq_ignore_ascii_case("PID"))
    {
        into.push(path.to_path_buf());
    }
    Ok(())
}

/// Where a rendered sprite lands: the input's path relative to the root it was
/// found under, with the extension swapped, so a walked tree keeps its shape.
fn dest(out: &Path, root: &Path, file: &Path) -> PathBuf {
    let rel = file.strip_prefix(root).unwrap_or(file);
    let mut d = out.join(rel);
    d.set_extension("png");
    d
}

fn render(
    file: &Path,
    dst: &Path,
    override_pal: Option<&[u8]>,
    transparent: bool,
    overrun: pid::RowOverrun,
    dry_run: bool,
) -> Result<(), String> {
    let bytes = std::fs::read(file).map_err(|e| e.to_string())?;
    let sprite = pid::split(&bytes).map_err(|e| e.to_string())?;
    let dims = sprite.header.dims().map_err(|e| e.to_string())?;

    // An explicit --palette wins over the embedded one: applying a foreign
    // palette is the point, not an accident.
    let palette: &[u8] = match (override_pal, sprite.palette) {
        (Some(p), _) => p,
        (None, Some(p)) => p,
        (None, None) => {
            return Err(
                "no palette: sprite has none embedded (flags & 0x80 clear), pass --palette".into(),
            )
        }
    };
    if palette.len() != pal::BYTE_LEN {
        return Err(format!(
            "palette is {} bytes, expected {}",
            palette.len(),
            pal::BYTE_LEN
        ));
    }

    let mut pixels = vec![0u8; dims.pixel_len()];
    sprite
        .decode_into(&mut pixels, overrun)
        .map_err(|e| e.to_string())?;

    if dry_run {
        println!(
            "{:>5}x{:<5} {}",
            dims.width(),
            dims.height(),
            dst.display()
        );
        return Ok(());
    }

    if let Some(parent) = dst.parent() {
        std::fs::create_dir_all(parent).map_err(|e| e.to_string())?;
    }
    let file = File::create(dst).map_err(|e| e.to_string())?;
    let mut enc = png::Encoder::new(
        BufWriter::new(file),
        dims.width() as u32,
        dims.height() as u32,
    );
    enc.set_color(png::ColorType::Indexed);
    enc.set_depth(png::BitDepth::Eight);
    enc.set_palette(palette.to_vec());
    if transparent {
        // tRNS is a prefix of the palette: every index up to the fill stays
        // opaque, the fill itself becomes fully transparent.
        let fill = sprite.header.fill_byte() as usize;
        let mut trns = vec![0xffu8; fill + 1];
        trns[fill] = 0;
        enc.set_trns(trns);
    }
    let mut writer = enc.write_header().map_err(|e| e.to_string())?;
    writer.write_image_data(&pixels).map_err(|e| e.to_string())
}

fn main() -> ExitCode {
    let cli = Cli::parse();

    let override_pal = match &cli.palette {
        Some(p) => match std::fs::read(p) {
            Ok(b) => match pal::split(&b) {
                Ok(_) => Some(b),
                Err(e) => {
                    eprintln!("pid2png: {}: {e}", p.display());
                    return ExitCode::FAILURE;
                }
            },
            Err(e) => {
                eprintln!("pid2png: {}: {e}", p.display());
                return ExitCode::FAILURE;
            }
        },
        None => None,
    };
    let overrun = if cli.spill {
        pid::RowOverrun::Spill
    } else {
        pid::RowOverrun::Carry
    };

    let mut ok = 0usize;
    let mut failed = 0usize;
    for root in &cli.inputs {
        let mut files = Vec::new();
        if let Err(e) = collect(root, cli.recursive, &mut files) {
            eprintln!("pid2png: {}: {e}", root.display());
            failed += 1;
            continue;
        }
        if files.is_empty() && !root.is_dir() {
            files.push(root.clone());
        }
        // A single named file lands directly in --out; a walked tree keeps its
        // shape below it.
        let base = if root.is_dir() {
            root.as_path()
        } else {
            root.parent().unwrap_or(Path::new(""))
        };
        for f in &files {
            let dst = dest(&cli.out, base, f);
            match render(
                f,
                &dst,
                override_pal.as_deref(),
                cli.transparent,
                overrun,
                cli.dry_run,
            ) {
                Ok(()) => ok += 1,
                Err(e) => {
                    eprintln!("pid2png: {}: {e}", f.display());
                    failed += 1;
                }
            }
        }
    }

    eprintln!(
        "[pid2png] {} {ok} sprite(s){}",
        if cli.dry_run { "would render" } else { "rendered" },
        if failed > 0 {
            format!(", {failed} failed")
        } else {
            String::new()
        }
    );
    if failed > 0 && ok == 0 {
        return ExitCode::FAILURE;
    }
    ExitCode::SUCCESS
}
