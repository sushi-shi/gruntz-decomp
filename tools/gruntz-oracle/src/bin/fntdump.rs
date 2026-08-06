//! Inspect Gruntz bitmap fonts and render their 256 glyphs as indexed BMPs.

use std::path::{Path, PathBuf};
use std::process::ExitCode;

use clap::Parser;
use gruntz_codec::{bmp, fnt};

#[derive(Parser)]
#[command(
    name = "fntdump",
    about = "Inspect Gruntz .FNT bitmap fonts and optionally render glyph atlases"
)]
struct Cli {
    /// One or more LARGE/MEDIUM/SMALL/TINY.FNT files.
    #[arg(required = true)]
    fonts: Vec<PathBuf>,
    /// Write one <font>.bmp atlas per input below this directory.
    #[arg(long)]
    out: Option<PathBuf>,
    /// Number of glyph cells per atlas row.
    #[arg(long, default_value_t = 16)]
    columns: usize,
}

fn main() -> ExitCode {
    let cli = Cli::parse();
    if cli.columns == 0 {
        eprintln!("fntdump: --columns must be greater than zero");
        return ExitCode::FAILURE;
    }
    if let Some(out) = &cli.out {
        if let Err(error) = std::fs::create_dir_all(out) {
            eprintln!("fntdump: {}: {error}", out.display());
            return ExitCode::FAILURE;
        }
    }

    let mut failed = false;
    for path in &cli.fonts {
        if let Err(error) = inspect_font(path, cli.out.as_deref(), cli.columns) {
            eprintln!("fntdump: {}: {error}", path.display());
            failed = true;
        }
    }
    if failed {
        ExitCode::FAILURE
    } else {
        ExitCode::SUCCESS
    }
}

fn inspect_font(
    path: &Path,
    out_dir: Option<&Path>,
    columns: usize,
) -> Result<(), Box<dyn std::error::Error>> {
    let bytes = std::fs::read(path)?;
    let font = fnt::split(&bytes)?;
    let mut nonempty = 0usize;
    let mut max_width = 0usize;
    let mut max_height = 0usize;
    for glyph in font.glyphs() {
        nonempty += usize::from(glyph.width != 0 && glyph.height != 0);
        max_width = max_width.max(glyph.width);
        max_height = max_height.max(glyph.height);
    }
    println!(
        "{}: {} bytes, {} glyphs, {} non-empty, max {}x{}",
        path.display(),
        bytes.len(),
        font.count(),
        nonempty,
        max_width,
        max_height
    );

    let Some(out_dir) = out_dir else {
        return Ok(());
    };
    let overflow =
        || std::io::Error::new(std::io::ErrorKind::InvalidData, "font atlas is too large");
    let cell_width = max_width.checked_add(1).ok_or_else(overflow)?;
    let cell_height = max_height.checked_add(1).ok_or_else(overflow)?;
    let rows = font.count().div_ceil(columns);
    let width = columns.checked_mul(cell_width).ok_or_else(overflow)?;
    let height = rows.checked_mul(cell_height).ok_or_else(overflow)?;
    let mut pixels = vec![0u8; width.checked_mul(height).ok_or_else(overflow)?];
    for glyph in font.glyphs() {
        let left = (glyph.index % columns) * cell_width;
        let top = (glyph.index / columns) * cell_height;
        for row in 0..glyph.height {
            let src = &glyph.pixels[row * glyph.width..(row + 1) * glyph.width];
            let at = (top + row) * width + left;
            pixels[at..at + glyph.width].copy_from_slice(src);
        }
    }
    let mut encoded = vec![0u8; bmp::indexed_len(width, height)];
    let written = bmp::write_indexed_into(&pixels, width, height, None, &mut encoded)?;
    let stem = path
        .file_stem()
        .ok_or("font path has no file name")?
        .to_string_lossy();
    let destination = out_dir.join(format!("{stem}.bmp"));
    std::fs::write(&destination, &encoded[..written])?;
    eprintln!("[fnt] {} -> {}", path.display(), destination.display());
    Ok(())
}
