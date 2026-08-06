//! `gruntz-oracle` — differential testing of the Gruntz sprite formats.
//!
//! The point is not a match percentage. It is to produce, for every claim we
//! make about the format, either "all shipped sprites agree" or a *named
//! sprite that reproduces the disagreement*. A named reproducer is a bug report
//! another lane can act on; a percentage is not.
//!
//! Three checks of increasing strength:
//!
//! * `census` — does every sprite decode, and does the token stream end exactly
//!   where the resource does?
//! * `roundtrip` — does `decode -> encode` reproduce the original bytes? This is
//!   the only real test of an encoder, and where it fails it characterises how.
//! * `decoders` — do retail's two mutually inconsistent decoders (`RunDecode1`
//!   carry vs `DecodePidData` spill) actually disagree on shipped data?
//! * `recomp` — the third implementation: run retail's OWN machine code over the
//!   corpus and compare. Neither our C++ nor our Rust can bias this one.
//!
//! `tokens` prints a token-level diff of one sprite and `dump` writes one out as
//! a `.bmp`, so a disagreement can be read and looked at rather than counted.

mod gif;
mod map;
mod midi;

use std::collections::{BTreeMap, BTreeSet};
use std::io::Write;
use std::path::{Path, PathBuf};
use std::process::{Command, ExitCode, Stdio};

use clap::{Parser, Subcommand, ValueEnum};
use gruntz_codec::{ani, bmp, pal, pcx, pid, rid, xmi};
use gruntz_rez::{Resource, Rez};

#[derive(Parser)]
#[command(
    name = "gruntz-oracle",
    about = "Differential testing of the Gruntz sprite codecs against real assets"
)]
struct Cli {
    /// A .REZ archive; repeat the flag for more. The demo and retail archives
    /// are both valid corpora, and running both catches format drift between
    /// the two builds.
    #[arg(long = "rez", required = true, action = clap::ArgAction::Append)]
    rez: Vec<PathBuf>,
    #[command(subcommand)]
    cmd: Cmd,
}

#[derive(Subcommand)]
enum Cmd {
    /// Decode every sprite; report clean decodes, header-field distributions
    /// and the exact failure taxonomy.
    Census {
        /// Print up to N example paths per failure class.
        #[arg(long, default_value_t = 5)]
        examples: usize,
    },
    /// decode -> encode -> byte-compare against the original stream.
    Roundtrip {
        #[arg(long, default_value_t = 5)]
        examples: usize,
        /// Which single bytes the encoder may spell as a bare literal.
        #[arg(long, value_enum, default_value_t = Literals::HighBitClear)]
        literals: Literals,
    },
    /// Decode each sprite with BOTH retail decoders and report disagreements.
    Decoders {
        #[arg(long, default_value_t = 10)]
        examples: usize,
    },
    /// The third implementation: run retail's OWN machine code
    /// (`CDDSurface::RunDecode1` @0x145270, mapped out of GRUNTZ.EXE by
    /// `tools/recomp/pidrun.exe` under wine) over the whole corpus and compare
    /// its pixels against ours.
    Recomp {
        /// The `pidrun.exe` built by `tools/recomp/build.sh`.
        #[arg(long, default_value = "tools/recomp/pidrun.exe")]
        harness: PathBuf,
        /// Retail GRUNTZ.EXE (defaults to $GRUNTZ_EXE).
        #[arg(long)]
        exe: Option<PathBuf>,
        /// Scratch directory for the job/result files.
        #[arg(long, default_value = "/tmp")]
        scratch: PathBuf,
        /// Stop after this many sprites (0 = all).
        #[arg(long, default_value_t = 0)]
        limit: usize,
        #[arg(long, default_value_t = 5)]
        examples: usize,
    },
    /// Settle the RLE16 row-end question (`x >= width - 1` in
    /// `CDDrawShadeBlit::EncodeRle16` @0x149694 vs `x >= width` in
    /// `CRezImage::DecodePidData` @0x176597) on the only streams that can
    /// reach `EncodeRle16`.
    ///
    /// `CDDrawShadeBlit::Build` @0x1490d0 copies the PID skip/fill stream into
    /// `m_rleData` verbatim, then calls `EncodeRle16` only when `m_srcBpp == 2`,
    /// which requires NEITHER `PID_SRC_8BPP_SHADE` (0x40) NOR `PID_SRC_8BPP`
    /// (0x200). The corpus for that function is therefore exactly the skip/fill
    /// sprites carrying neither bit, and it is tiny.
    Rle16 {
        #[arg(long, default_value_t = 10)]
        examples: usize,
    },
    /// Print retail's token stream for one sprite beside our re-encoding, so a
    /// round-trip difference can be read as tokens instead of as a byte count.
    Tokens {
        /// Full path, e.g. `AREA2\\IMAGEZ\\TREE2\\FRAME001`.
        path: String,
        /// Stop after this many tokens.
        #[arg(long, default_value_t = 40)]
        limit: usize,
        #[arg(long, value_enum, default_value_t = Literals::Decodable)]
        literals: Literals,
    },
    /// Write one resource out as a .bmp (sprites) or raw (anything else).
    Dump {
        /// Full path, e.g. `AREA2\IMAGEZ\TREE2\FRAME001`.
        path: String,
        out: PathBuf,
        #[arg(long, value_enum, default_value_t = Which::Carry)]
        decoder: Which,
        /// Palette resource to use for RID or palette-less PID data. PAL files
        /// are raw 256 x RGB bytes; if omitted, dumps use a grey ramp.
        #[arg(long)]
        palette: Option<String>,
    },
    /// Inspect an ANI control resource and optionally render a GIF preview.
    /// ANI contains no pixels, so GIF output also needs the PID/RID image-set
    /// prefix that the game object binds separately.
    Ani {
        /// Full ANI resource path.
        path: String,
        /// Write an animated GIF preview.
        #[arg(long)]
        gif: Option<PathBuf>,
        /// REZ path prefix containing numerically suffixed PID/RID frames.
        #[arg(long)]
        frames: Option<String>,
        /// Palette resource for RID or palette-less PID frames.
        #[arg(long)]
        palette: Option<String>,
        /// Runtime Grunt colour. The game stores green-indexed sprites and
        /// substitutes a TOOL/TOY palette while drawing them.
        #[arg(long, value_enum, default_value_t = GruntTint::Orange)]
        tint: GruntTint,
        /// Maximum control steps to render before stopping an open-ended ANI.
        #[arg(long, default_value_t = 256)]
        steps: usize,
        /// Initial frame index; defaults to the image set's lowest index.
        #[arg(long)]
        start_frame: Option<i32>,
    },
    /// Render every ANI whose conventional sibling IMAGEZ frame set exists.
    /// ANI resources with external or generic frame bindings are listed in an
    /// UNRESOLVED.tsv manifest instead of being paired by guesswork.
    AniAll {
        /// Output root; REZ paths are preserved below it.
        out: PathBuf,
        /// Maximum control steps per open-ended ANI.
        #[arg(long, default_value_t = 256)]
        steps: usize,
        /// Runtime Grunt colour (`source` preserves the embedded green table).
        #[arg(long, value_enum, default_value_t = GruntTint::Orange)]
        tint: GruntTint,
    },
    /// Inspect a Miles XMI music resource and optionally export one sequence
    /// as a standard MIDI file.
    Xmi {
        /// Full XMI resource path.
        path: String,
        /// Write a standard MIDI file using the original 120 Hz timing.
        #[arg(long)]
        midi: Option<PathBuf>,
        /// Synthesize an mpv-playable WAV preview using TiMidity's configured
        /// instrument bank. This is not the original Gruntz SoundFont.
        #[arg(long)]
        wav: Option<PathBuf>,
        /// Zero-based sequence index (retail Gruntz resources contain one).
        #[arg(long, default_value_t = 0)]
        sequence: usize,
    },
    /// Convert every XMI resource, preserving REZ paths.
    XmiAll {
        out: PathBuf,
        /// Also synthesize an mpv-playable WAV preview for every sequence.
        #[arg(long)]
        wav: bool,
    },
    /// Render one WWD level: the gameplay plane becomes one large PNG and
    /// every parallax/foreground plane is written beside it.
    Wwd {
        /// Full WWD resource path, e.g. `AREA1\WORLDZ\LEVEL1`.
        path: String,
        /// Destination for the main gameplay-plane PNG.
        out: PathBuf,
    },
    /// Render every WWD level while preserving its REZ path.
    WwdAll { out: PathBuf },
}

/// Mirror of `pid::LiteralRule` for the command line.
#[derive(Copy, Clone, PartialEq, Eq, ValueEnum)]
enum Literals {
    /// `(v & 0xC0) == 0` - the IMAGEZ/BOOTY sprite exporter.
    LowSixBits,
    /// `v < 0x80` - refuted; kept so it stays refuted.
    HighBitClear,
    /// `(v & 0xC0) != 0xC0` - everything the decoder accepts.
    Decodable,
    /// No bare literals: every singleton pixel is spelled `C1 v`.
    Never,
    /// Try every rule and count a sprite as reproduced if ANY of them matches,
    /// reporting which one won. This is the honest total, and it isolates the
    /// sprites no known rule explains.
    Any,
}

impl From<Literals> for pid::LiteralRule {
    fn from(l: Literals) -> Self {
        match l {
            Literals::LowSixBits => pid::LiteralRule::LowSixBits,
            Literals::HighBitClear => pid::LiteralRule::HighBitClear,
            Literals::Decodable => pid::LiteralRule::Decodable,
            Literals::Never => pid::LiteralRule::Never,
            // `Any` is a driver mode, not a rule; the runner special-cases it.
            Literals::Any => pid::LiteralRule::Decodable,
        }
    }
}

#[derive(Copy, Clone, PartialEq, Eq, ValueEnum)]
enum Which {
    /// `CDDSurface::RunDecode1` @0x145270.
    Carry,
    /// `CRezImage::DecodePidData` @0x176440.
    Spill,
}

#[derive(Copy, Clone, Debug, PartialEq, Eq, ValueEnum)]
enum GruntTint {
    /// Preserve the palette embedded in the frame.
    Source,
    Orange,
    Green,
    Blue,
    Red,
    Purple,
    Yellow,
    Hotpink,
    Black,
    Dkblue,
    Dkgreen,
    Turq,
    Dkred,
    Pink,
    Dkyellow,
    Grey,
    Cyan,
    White,
}

impl GruntTint {
    fn resource_stem(self) -> Option<&'static str> {
        match self {
            Self::Source => None,
            Self::Orange => Some("ORANGE"),
            Self::Green => Some("GREEN"),
            Self::Blue => Some("BLUE"),
            Self::Red => Some("RED"),
            Self::Purple => Some("PURPLE"),
            Self::Yellow => Some("YELLOW"),
            Self::Hotpink => Some("HOTPINK"),
            Self::Black => Some("BLACK"),
            Self::Dkblue => Some("DKBLUE"),
            Self::Dkgreen => Some("DKGREEN"),
            Self::Turq => Some("TURQ"),
            Self::Dkred => Some("DKRED"),
            Self::Pink => Some("PINK"),
            Self::Dkyellow => Some("DKYELLOW"),
            Self::Grey => Some("GREY"),
            Self::Cyan => Some("CYAN"),
            Self::White => Some("WHITE"),
        }
    }
}

impl From<Which> for pid::RowOverrun {
    fn from(w: Which) -> Self {
        match w {
            Which::Carry => pid::RowOverrun::Carry,
            Which::Spill => pid::RowOverrun::Spill,
        }
    }
}

/// Counts keyed by a short reason, with a few example paths each.
#[derive(Default)]
struct Tally {
    counts: BTreeMap<String, usize>,
    examples: BTreeMap<String, Vec<String>>,
}

impl Tally {
    fn add(&mut self, key: impl Into<String>, path: &str, keep: usize) {
        let key = key.into();
        *self.counts.entry(key.clone()).or_default() += 1;
        let ex = self.examples.entry(key).or_default();
        if ex.len() < keep {
            ex.push(path.to_string());
        }
    }

    fn report(&self, title: &str, total: usize) {
        println!("\n{title}");
        println!("{:-<78}", "");
        for (k, n) in &self.counts {
            println!("  {n:>7}  {:>6.2}%  {k}", pct(*n, total));
            for e in self.examples.get(k).into_iter().flatten() {
                println!("                     {e}");
            }
        }
    }
}

fn main() -> ExitCode {
    let cli = Cli::parse();
    let mut archives = Vec::new();
    for p in &cli.rez {
        match std::fs::read(p) {
            Ok(b) => archives.push((p.clone(), b)),
            Err(e) => {
                eprintln!("gruntz-oracle: {}: {e}", p.display());
                return ExitCode::FAILURE;
            }
        }
    }
    match &cli.cmd {
        Cmd::Census { examples } => run(&archives, |rez| census(rez, *examples)),
        Cmd::Roundtrip { examples, literals } => {
            let any = matches!(literals, Literals::Any);
            let rule = (*literals).into();
            run(&archives, |rez| roundtrip(rez, *examples, rule, any))
        }
        Cmd::Decoders { examples } => run(&archives, |rez| decoders(rez, *examples)),
        Cmd::Rle16 { examples } => run(&archives, |rez| rle16(rez, *examples)),
        Cmd::Recomp {
            harness,
            exe,
            scratch,
            limit,
            examples,
        } => {
            let exe = exe
                .clone()
                .or_else(|| std::env::var_os("GRUNTZ_EXE").map(PathBuf::from));
            let Some(exe) = exe else {
                eprintln!("gruntz-oracle: --exe or $GRUNTZ_EXE is required");
                return ExitCode::FAILURE;
            };
            let mut rc = ExitCode::SUCCESS;
            for (name, bytes) in &archives {
                let Ok(rez) = Rez::new(bytes) else { continue };
                println!("\n================ {} ================", name.display());
                if let Err(e) = recomp(&rez, harness, &exe, scratch, *limit, *examples) {
                    eprintln!("gruntz-oracle: recomp: {e}");
                    rc = ExitCode::FAILURE;
                }
            }
            rc
        }
        Cmd::Tokens {
            path,
            limit,
            literals,
        } => {
            for (_, bytes) in &archives {
                let Ok(rez) = Rez::new(bytes) else { continue };
                let up = path.to_ascii_uppercase();
                let Some(r) = rez
                    .resources()
                    .flatten()
                    .find(|r| r.path().to_string().to_ascii_uppercase() == up)
                else {
                    continue;
                };
                return match tokens(&rez, &r, *limit, (*literals).into()) {
                    Ok(()) => ExitCode::SUCCESS,
                    Err(e) => {
                        eprintln!("gruntz-oracle: {path}: {e}");
                        ExitCode::FAILURE
                    }
                };
            }
            eprintln!("gruntz-oracle: no such resource: {path}");
            ExitCode::FAILURE
        }
        Cmd::Dump {
            path,
            out,
            decoder,
            palette,
        } => {
            for (name, bytes) in &archives {
                let Ok(rez) = Rez::new(bytes) else { continue };
                let up = path.to_ascii_uppercase();
                let Some(r) = rez
                    .resources()
                    .flatten()
                    .find(|r| r.path().to_string().to_ascii_uppercase() == up)
                else {
                    continue;
                };
                return match dump(&rez, &r, out, (*decoder).into(), palette.as_deref()) {
                    Ok(()) => {
                        eprintln!("[dump] {path} from {} -> {}", name.display(), out.display());
                        ExitCode::SUCCESS
                    }
                    Err(e) => {
                        eprintln!("gruntz-oracle: {path}: {e}");
                        ExitCode::FAILURE
                    }
                };
            }
            eprintln!("gruntz-oracle: no such resource: {path}");
            ExitCode::FAILURE
        }
        Cmd::Ani {
            path,
            gif,
            frames,
            palette,
            tint,
            steps,
            start_frame,
        } => {
            for (name, bytes) in &archives {
                let Ok(rez) = Rez::new(bytes) else { continue };
                let Some(r) = find_resource(&rez, path) else {
                    continue;
                };
                return match inspect_ani(
                    &rez,
                    &r,
                    AniPreviewOptions {
                        gif_path: gif.as_deref(),
                        frame_prefix: frames.as_deref(),
                        palette_path: palette.as_deref(),
                        tint: *tint,
                        max_steps: *steps,
                        start_frame: *start_frame,
                    },
                ) {
                    Ok(()) => {
                        if let Some(out) = gif {
                            eprintln!("[ani] {path} from {} -> {}", name.display(), out.display());
                        }
                        ExitCode::SUCCESS
                    }
                    Err(e) => {
                        eprintln!("gruntz-oracle: {path}: {e}");
                        ExitCode::FAILURE
                    }
                };
            }
            eprintln!("gruntz-oracle: no such resource: {path}");
            ExitCode::FAILURE
        }
        Cmd::AniAll { out, steps, tint } => {
            if *steps == 0 {
                eprintln!("gruntz-oracle: --steps must be greater than zero");
                return ExitCode::FAILURE;
            }
            let mut rc = ExitCode::SUCCESS;
            for (name, bytes) in &archives {
                let Ok(rez) = Rez::new(bytes) else {
                    eprintln!("gruntz-oracle: {}: invalid REZ archive", name.display());
                    rc = ExitCode::FAILURE;
                    continue;
                };
                println!("\n================ {} ================", name.display());
                if let Err(e) = export_all_animations(&rez, out, *steps, *tint) {
                    eprintln!("gruntz-oracle: ani-all: {e}");
                    rc = ExitCode::FAILURE;
                }
            }
            rc
        }
        Cmd::Xmi {
            path,
            midi,
            wav,
            sequence,
        } => {
            for (name, bytes) in &archives {
                let Ok(rez) = Rez::new(bytes) else { continue };
                let Some(resource) = find_resource(&rez, path) else {
                    continue;
                };
                return match inspect_xmi(
                    &rez,
                    &resource,
                    midi.as_deref(),
                    wav.as_deref(),
                    *sequence,
                ) {
                    Ok(()) => {
                        if let Some(out) = midi {
                            eprintln!("[xmi] {path} from {} -> {}", name.display(), out.display());
                        }
                        if let Some(out) = wav {
                            eprintln!(
                                "[xmi] {path} preview from {} -> {}",
                                name.display(),
                                out.display()
                            );
                        }
                        ExitCode::SUCCESS
                    }
                    Err(error) => {
                        eprintln!("gruntz-oracle: {path}: {error}");
                        ExitCode::FAILURE
                    }
                };
            }
            eprintln!("gruntz-oracle: no such resource: {path}");
            ExitCode::FAILURE
        }
        Cmd::XmiAll { out, wav } => {
            let mut rc = ExitCode::SUCCESS;
            for (name, bytes) in &archives {
                let Ok(rez) = Rez::new(bytes) else {
                    eprintln!("gruntz-oracle: {}: invalid REZ archive", name.display());
                    rc = ExitCode::FAILURE;
                    continue;
                };
                println!("\n================ {} ================", name.display());
                if let Err(error) = export_all_xmi(&rez, out, *wav) {
                    eprintln!("gruntz-oracle: xmi-all: {error}");
                    rc = ExitCode::FAILURE;
                }
            }
            rc
        }
        Cmd::Wwd { path, out } => {
            for (name, bytes) in &archives {
                let Ok(rez) = Rez::new(bytes) else { continue };
                let Some(resource) = find_resource(&rez, path) else {
                    continue;
                };
                return match map::render_resource(&rez, &resource, out) {
                    Ok(report) => {
                        let manifest = out.with_extension("unresolved.tsv");
                        if let Err(error) = report.write_manifest(path, &manifest) {
                            eprintln!("gruntz-oracle: {path}: {error}");
                            return ExitCode::FAILURE;
                        }
                        eprintln!(
                            "[wwd] {path} from {}: {} plane(s), {} unresolved cell reference(s)",
                            name.display(),
                            report.planes,
                            report.missing_references
                        );
                        ExitCode::SUCCESS
                    }
                    Err(error) => {
                        eprintln!("gruntz-oracle: {path}: {error}");
                        ExitCode::FAILURE
                    }
                };
            }
            eprintln!("gruntz-oracle: no such resource: {path}");
            ExitCode::FAILURE
        }
        Cmd::WwdAll { out } => {
            let mut rc = ExitCode::SUCCESS;
            for (name, bytes) in &archives {
                let Ok(rez) = Rez::new(bytes) else {
                    eprintln!("gruntz-oracle: {}: invalid REZ archive", name.display());
                    rc = ExitCode::FAILURE;
                    continue;
                };
                println!("\n================ {} ================", name.display());
                if let Err(error) = export_all_wwd(&rez, out) {
                    eprintln!("gruntz-oracle: wwd-all: {error}");
                    rc = ExitCode::FAILURE;
                }
            }
            rc
        }
    }
}

fn run(archives: &[(PathBuf, Vec<u8>)], mut f: impl FnMut(&Rez)) -> ExitCode {
    for (name, bytes) in archives {
        let rez = match Rez::new(bytes) {
            Ok(r) => r,
            Err(e) => {
                eprintln!("gruntz-oracle: {}: {e}", name.display());
                return ExitCode::FAILURE;
            }
        };
        println!("\n================ {} ================", name.display());
        f(&rez);
    }
    ExitCode::SUCCESS
}

fn of_kind<'a>(rez: &'a Rez<'a>, kind: &'static str) -> impl Iterator<Item = Resource<'a>> + 'a {
    rez.resources()
        .flatten()
        .filter(move |r| r.kind.to_string() == kind)
}

fn find_resource<'a>(rez: &'a Rez<'a>, path: &str) -> Option<Resource<'a>> {
    let wanted = path.to_ascii_uppercase();
    rez.resources()
        .flatten()
        .find(|r| r.path().to_string().to_ascii_uppercase() == wanted)
}

fn export_all_wwd(rez: &Rez, out: &Path) -> Result<(), Box<dyn std::error::Error>> {
    std::fs::create_dir_all(out)?;
    let mut levels = 0usize;
    let mut planes = 0usize;
    let mut unresolved = 0usize;
    let mut failures = Vec::new();
    let mut manifest = String::from("level\tplane\timage_set\tframe\tcells\treason\n");
    for resource in of_kind(rez, "WWD") {
        let path = resource.path().to_string();
        let mut relative = PathBuf::new();
        for component in path.split(['\\', '/']) {
            relative.push(component);
        }
        relative.set_extension("png");
        match map::render_resource(rez, &resource, &out.join(relative)) {
            Ok(report) => {
                levels += 1;
                planes += report.planes;
                unresolved += report.missing_references;
                report.append_manifest_rows(&path, &mut manifest);
            }
            Err(error) => failures.push(format!("{path}: {error}")),
        }
    }
    std::fs::write(out.join("UNRESOLVED.tsv"), manifest)?;
    println!("WWD levels rendered    : {levels}");
    println!("Plane PNGs generated   : {planes}");
    println!("Unresolved tile cells  : {unresolved}");
    if !failures.is_empty() {
        for failure in failures.iter().take(10) {
            eprintln!("  {failure}");
        }
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            format!("{} WWD resource(s) failed", failures.len()),
        )
        .into());
    }
    Ok(())
}

fn inspect_xmi(
    rez: &Rez,
    resource: &Resource,
    midi_path: Option<&Path>,
    wav_path: Option<&Path>,
    sequence_index: usize,
) -> Result<(), Box<dyn std::error::Error>> {
    if resource.kind.to_string() != "XMI" {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidInput,
            format!("{} is {}, not XMI", resource.path(), resource.kind),
        )
        .into());
    }
    let music = xmi::split(resource.data(rez.bytes()))?;
    let sequences = music.sequences().collect::<Result<Vec<_>, _>>()?;
    println!(
        "{}: sequences={} bytes={}",
        resource.path(),
        sequences.len(),
        resource.data(rez.bytes()).len()
    );
    for (index, sequence) in sequences.iter().copied().enumerate() {
        let mut channel = 0usize;
        let mut notes = 0usize;
        let mut meta = 0usize;
        let mut sysex = 0usize;
        let mut end = 0u32;
        for event in sequence.events() {
            let event = event?;
            end = end.max(event.time());
            match event {
                xmi::Event::Channel { duration, .. } => {
                    channel += 1;
                    notes += usize::from(duration.is_some());
                }
                xmi::Event::Meta { .. } => meta += 1,
                xmi::Event::SysEx { .. } => sysex += 1,
            }
        }
        println!(
            "  sequence {index}: timbres={} branches={} event_bytes={} channel={} notes={} meta={} sysex={} last_tick={end}",
            sequence.timbre_count()?,
            sequence.branches.map_or(0, <[u8]>::len),
            sequence.events.len(),
            channel,
            notes,
            meta,
            sysex
        );
    }
    if midi_path.is_some() || wav_path.is_some() {
        let sequence = sequences.get(sequence_index).copied().ok_or_else(|| {
            std::io::Error::new(
                std::io::ErrorKind::InvalidInput,
                format!(
                    "sequence {sequence_index} does not exist; resource has {}",
                    sequences.len()
                ),
            )
        })?;
        let rendered = midi::render(sequence)?;
        if let Some(out) = midi_path {
            if let Some(parent) = out.parent() {
                std::fs::create_dir_all(parent)?;
            }
            std::fs::write(out, &rendered)?;
        }
        if let Some(out) = wav_path {
            synthesize_wav(&rendered, out)?;
        }
    }
    Ok(())
}

fn synthesize_wav(midi: &[u8], out: &Path) -> Result<(), Box<dyn std::error::Error>> {
    if let Some(parent) = out.parent() {
        std::fs::create_dir_all(parent)?;
    }
    let mut child = Command::new("timidity")
        .args(["-Ow", "-o"])
        .arg(out)
        .arg("-")
        .stdin(Stdio::piped())
        .spawn()
        .map_err(|error| {
            std::io::Error::new(
                error.kind(),
                format!("could not start timidity for WAV preview: {error}"),
            )
        })?;
    child
        .stdin
        .take()
        .ok_or_else(|| std::io::Error::other("timidity stdin was not piped"))?
        .write_all(midi)?;
    let status = child.wait()?;
    if !status.success() {
        return Err(std::io::Error::other(format!("timidity failed with status {status}")).into());
    }
    Ok(())
}

fn export_all_xmi(
    rez: &Rez,
    out: &Path,
    render_wav: bool,
) -> Result<(), Box<dyn std::error::Error>> {
    std::fs::create_dir_all(out)?;
    let mut resources = 0usize;
    let mut sequences = 0usize;
    let mut wavs = 0usize;
    let mut failures = Vec::new();
    for resource in of_kind(rez, "XMI") {
        resources += 1;
        let path = resource.path().to_string();
        let result = (|| -> Result<(usize, usize), Box<dyn std::error::Error>> {
            let music = xmi::split(resource.data(rez.bytes()))?;
            let parsed = music.sequences().collect::<Result<Vec<_>, _>>()?;
            for (index, sequence) in parsed.iter().copied().enumerate() {
                let mut relative = PathBuf::new();
                for component in path.split(['\\', '/']) {
                    relative.push(component);
                }
                if parsed.len() == 1 {
                    relative.set_extension("mid");
                } else {
                    relative.set_file_name(format!("{}-{index:03}.mid", resource.name));
                }
                let destination = out.join(&relative);
                if let Some(parent) = destination.parent() {
                    std::fs::create_dir_all(parent)?;
                }
                let rendered = midi::render(sequence)?;
                std::fs::write(destination, &rendered)?;
                if render_wav {
                    relative.set_extension("wav");
                    synthesize_wav(&rendered, &out.join(relative))?;
                }
            }
            Ok((parsed.len(), usize::from(render_wav) * parsed.len()))
        })();
        match result {
            Ok((midi_count, wav_count)) => {
                sequences += midi_count;
                wavs += wav_count;
            }
            Err(error) => failures.push(format!("{path}: {error}")),
        }
    }
    println!("XMI resources         : {resources}");
    println!("MIDI files generated  : {sequences}");
    if render_wav {
        println!("WAV previews generated: {wavs}");
    }
    if !failures.is_empty() {
        for failure in failures.iter().take(10) {
            eprintln!("  {failure}");
        }
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            format!("{} XMI resource(s) failed", failures.len()),
        )
        .into());
    }
    Ok(())
}

// ---------------------------------------------------------------------------
// check 1: does everything decode, and is the stream consumed exactly?
// ---------------------------------------------------------------------------

fn census(rez: &Rez, keep: usize) {
    let mut total = 0usize;
    let mut ok = 0usize;
    let mut pcx_marker = 0usize;
    let mut t = Tally::default();
    let mut flags_hist: BTreeMap<u32, usize> = BTreeMap::new();
    let mut desc_hist: BTreeMap<u32, usize> = BTreeMap::new();
    let mut unk1_hist: BTreeMap<u32, usize> = BTreeMap::new();
    let mut grammar_hist: BTreeMap<&str, usize> = BTreeMap::new();
    let mut nonmul4 = 0usize;
    let mut buf = Vec::new();

    for r in of_kind(rez, "PID") {
        total += 1;
        let path = r.path().to_string();
        let data = r.data(rez.bytes());
        let p = match pid::split(data) {
            Ok(p) => p,
            Err(e) => {
                t.add(format!("split failed: {e}"), &path, keep);
                continue;
            }
        };
        *flags_hist.entry(p.header.flags).or_default() += 1;
        *desc_hist.entry(p.header.file_desc).or_default() += 1;
        *unk1_hist.entry(p.header.unk1).or_default() += 1;
        *grammar_hist
            .entry(match p.header.grammar() {
                pid::Grammar::Rle => "Rle     (0xC0 runs, flags&0x20 clear)",
                pid::Grammar::SkipRun => "SkipRun (0x80 fill, flags&0x20 set)",
            })
            .or_default() += 1;
        if !p.header.decodepid_would_accept() {
            nonmul4 += 1;
        }
        let dims = match p.header.dims() {
            Ok(d) => d,
            Err(e) => {
                t.add(format!("bad dims: {e}"), &path, keep);
                continue;
            }
        };
        buf.clear();
        buf.resize(dims.pixel_len(), 0);
        match p.decode_into(&mut buf, pid::RowOverrun::Carry) {
            Ok(used) if used == p.stream.len() => ok += 1,
            // A single trailing 0x0C is the PCX end-of-image palette marker,
            // left in place by whatever converted these sprites from PCX. It
            // is not a decode failure: retail addresses the palette from EOF
            // (`hdr + size - 0x300`) and stops the token loop when the last row
            // is full, so the byte is never read either way.
            Ok(used) if used + 1 == p.stream.len() && p.stream[used] == 0x0c => {
                ok += 1;
                pcx_marker += 1;
            }
            Ok(used) => t.add(
                format!(
                    "stream NOT consumed exactly: {} byte(s) left over [{:?}], first leftover byte {:#04x}",
                    p.stream.len() - used,
                    p.header.grammar(),
                    p.stream[used]
                ),
                &path,
                keep,
            ),
            Err(e) => t.add(format!("decode failed: {e}"), &path, keep),
        }
    }

    println!("PID sprites          : {total}");
    println!("decoded clean        : {ok}  ({:.2}%)", pct(ok, total));
    println!(
        "  of which carry a trailing PCX 0x0C palette marker: {pcx_marker}  ({:.2}%)",
        pct(pcx_marker, total)
    );
    println!(
        "width % 4 != 0       : {nonmul4}  (CDDSurface::DecodePid @0x145b3d rejects these outright)"
    );
    println!("\ngrammar (flags & 0x20):");
    for (g, n) in &grammar_hist {
        println!("  {n:>7}  {g}");
    }
    println!("\nflags histogram:");
    for (f, n) in &flags_hist {
        println!("  {n:>7}  {f:#06x}  {}", describe_flags(*f));
    }
    println!("\nfile_desc (+0x00) histogram (top 24):");
    for (d, n) in desc_hist.iter().take(24) {
        println!("  {n:>7}  {d}");
    }
    println!("\nunk1 (+0x1c) histogram (top 8):");
    for (d, n) in unk1_hist.iter().take(8) {
        println!("  {n:>7}  {d:#x}");
    }
    if t.counts.is_empty() {
        println!("\nno PID failures.");
    } else {
        t.report("PID failures", total);
    }

    // PCX gets the same treatment; there are few enough to list every failure.
    let pcxs: Vec<_> = of_kind(rez, "PCX").collect();
    if !pcxs.is_empty() {
        let mut pok = 0usize;
        let mut pt = Tally::default();
        for r in &pcxs {
            let path = r.path().to_string();
            match pcx::split(r.data(rez.bytes())) {
                Ok(p) => {
                    let mut dst = vec![0u8; p.pixel_len()];
                    let mut scratch = vec![0u8; p.scratch_len()];
                    match p.decode_into(&mut dst, &mut scratch) {
                        Ok(used) if used == p.stream.len() => pok += 1,
                        Ok(used) => pt.add(
                            format!(
                                "stream not consumed exactly: {} byte(s) left over",
                                p.stream.len() - used
                            ),
                            &path,
                            keep,
                        ),
                        Err(e) => pt.add(format!("decode failed: {e}"), &path, keep),
                    }
                }
                Err(e) => pt.add(format!("split failed: {e}"), &path, keep),
            }
        }
        println!("\nPCX images           : {}", pcxs.len());
        println!(
            "decoded clean        : {pok}  ({:.2}%)",
            pct(pok, pcxs.len())
        );
        if !pt.counts.is_empty() {
            pt.report("PCX failures", pcxs.len());
        }
    }

    let anis: Vec<_> = of_kind(rez, "ANI").collect();
    if !anis.is_empty() {
        let mut clean = 0usize;
        let mut exact = 0usize;
        let mut records = 0usize;
        let mut step_hist = BTreeMap::new();
        let mut loop_hist = BTreeMap::new();
        let mut position_hist = BTreeMap::new();
        let mut at = Tally::default();
        for r in &anis {
            let path = r.path().to_string();
            let data = r.data(rez.bytes());
            let a = match ani::split(data) {
                Ok(a) => a,
                Err(e) => {
                    at.add(format!("split failed: {e}"), &path, keep);
                    continue;
                }
            };
            if a.trailing.is_empty() {
                clean += 1;
            } else {
                at.add(
                    format!("{} unparsed trailing byte(s)", a.trailing.len()),
                    &path,
                    keep,
                );
            }
            for record in a.records() {
                records += 1;
                *step_hist.entry(record.step_mode).or_insert(0usize) += 1;
                *loop_hist.entry(record.loop_mode).or_insert(0usize) += 1;
                *position_hist.entry(record.position_mode).or_insert(0usize) += 1;
            }
            let mut encoded = vec![0u8; a.encoded_len()];
            if a.encode_into(&mut encoded).is_ok() && encoded == data {
                exact += 1;
            } else {
                at.add("semantic re-encode differs", &path, keep);
            }
        }
        println!("\nANI programs         : {}", anis.len());
        println!(
            "parsed exactly       : {clean}  ({:.2}%)",
            pct(clean, anis.len())
        );
        println!(
            "byte-exact re-encode : {exact}  ({:.2}%)",
            pct(exact, anis.len())
        );
        println!("records              : {records}");
        println!("step modes           : {step_hist:?}");
        println!("loop modes           : {loop_hist:?}");
        println!("position modes       : {position_hist:?}");
        if !at.counts.is_empty() {
            at.report("ANI failures", anis.len());
        }
    }

    let rids: Vec<_> = of_kind(rez, "RID").collect();
    if !rids.is_empty() {
        let mut clean = 0usize;
        let mut exact = 0usize;
        let mut rt = Tally::default();
        for r in &rids {
            let path = r.path().to_string();
            let data = r.data(rez.bytes());
            let image = match rid::split(data) {
                Ok(image) => image,
                Err(e) => {
                    rt.add(format!("split failed: {e}"), &path, keep);
                    continue;
                }
            };
            if image.trailing.is_empty() {
                clean += 1;
            } else {
                rt.add(
                    format!("{} unparsed trailing byte(s)", image.trailing.len()),
                    &path,
                    keep,
                );
            }
            let mut encoded = vec![0u8; image.encoded_len()];
            if image.encode_into(&mut encoded).is_ok() && encoded == data {
                exact += 1;
            } else {
                rt.add("semantic re-encode differs", &path, keep);
            }
        }
        println!("\nRID images           : {}", rids.len());
        println!(
            "parsed exactly       : {clean}  ({:.2}%)",
            pct(clean, rids.len())
        );
        println!(
            "byte-exact re-encode : {exact}  ({:.2}%)",
            pct(exact, rids.len())
        );
        if !rt.counts.is_empty() {
            rt.report("RID failures", rids.len());
        }
    }

    let palettes: Vec<_> = of_kind(rez, "PAL").collect();
    if !palettes.is_empty() {
        let mut clean = 0usize;
        let mut pt = Tally::default();
        for resource in &palettes {
            match pal::split(resource.data(rez.bytes())) {
                Ok(_) => clean += 1,
                Err(error) => pt.add(
                    format!("split failed: {error}"),
                    &resource.path().to_string(),
                    keep,
                ),
            }
        }
        println!("\nPAL tables           : {}", palettes.len());
        println!(
            "parsed exactly       : {clean}  ({:.2}%)",
            pct(clean, palettes.len())
        );
        if !pt.counts.is_empty() {
            pt.report("PAL failures", palettes.len());
        }
    }

    let music: Vec<_> = of_kind(rez, "XMI").collect();
    if !music.is_empty() {
        let mut clean = 0usize;
        let mut sequences = 0usize;
        let mut xt = Tally::default();
        for resource in &music {
            let result = (|| -> Result<usize, Box<dyn std::error::Error>> {
                let parsed = xmi::split(resource.data(rez.bytes()))?;
                let tracks = parsed.sequences().collect::<Result<Vec<_>, _>>()?;
                for track in tracks.iter().copied() {
                    midi::render(track)?;
                }
                Ok(tracks.len())
            })();
            match result {
                Ok(count) => {
                    clean += 1;
                    sequences += count;
                }
                Err(error) => xt.add(
                    format!("parse/export failed: {error}"),
                    &resource.path().to_string(),
                    keep,
                ),
            }
        }
        println!("\nXMI music            : {}", music.len());
        println!(
            "parse + MIDI export  : {clean}  ({:.2}%)",
            pct(clean, music.len())
        );
        println!("sequences            : {sequences}");
        if !xt.counts.is_empty() {
            xt.report("XMI failures", music.len());
        }
    }
}

fn describe_flags(f: u32) -> String {
    let mut s = Vec::new();
    let known = [
        (pid::flags::TRANSPARENCY, "TRANSPARENCY"),
        (pid::flags::VIDEO_MEMORY, "VIDEO_MEMORY"),
        (pid::flags::SYSTEM_MEMORY, "SYSTEM_MEMORY"),
        (pid::flags::COMPRESSION, "GRAMMAR_SKIPRUN"),
        (pid::flags::SRC_8BPP_SHADE, "SRC_8BPP_SHADE"),
        (pid::flags::EMBEDDED_PALETTE, "EMBEDDED_PALETTE"),
        (pid::flags::FILL_IS_WORD, "FILL_IS_WORD"),
        (pid::flags::SRC_8BPP, "SRC_8BPP"),
    ];
    let mut mask = 0u32;
    for (bit, name) in known {
        mask |= bit;
        if f & bit != 0 {
            s.push(name.to_string());
        }
    }
    if f & !mask != 0 {
        s.push(format!("UNKNOWN({:#x})", f & !mask));
    }
    s.join("|")
}

// ---------------------------------------------------------------------------
// check 2: round-trip
// ---------------------------------------------------------------------------

const ALL_RULES: [pid::LiteralRule; 4] = [
    pid::LiteralRule::Decodable,
    pid::LiteralRule::LowSixBits,
    pid::LiteralRule::HighBitClear,
    pid::LiteralRule::Never,
];

fn roundtrip(rez: &Rez, keep: usize, rule: pid::LiteralRule, any: bool) {
    let mut total = 0usize;
    let mut exact = 0usize;
    let mut same_len = 0usize;
    let mut ours_smaller = 0usize;
    let mut ours_larger = 0usize;
    let mut delta_bytes: i64 = 0;
    let mut t = Tally::default();
    // (exact, total) keyed by grammar, and by "grammar / first path element",
    // because a single global percentage hides the fact that different asset
    // groups were exported by different tools.
    let mut by_grammar: BTreeMap<String, (usize, usize)> = BTreeMap::new();
    let mut by_group: BTreeMap<String, (usize, usize)> = BTreeMap::new();
    let mut rule_wins: BTreeMap<String, usize> = BTreeMap::new();
    let (mut pixels, mut reenc, mut redec) = (Vec::new(), Vec::new(), Vec::new());

    for r in of_kind(rez, "PID") {
        total += 1;
        let path = r.path().to_string();
        let data = r.data(rez.bytes());
        let Ok(p) = pid::split(data) else { continue };
        let Ok(dims) = p.header.dims() else { continue };
        pixels.clear();
        pixels.resize(dims.pixel_len(), 0);
        let Ok(used) = p.decode_into(&mut pixels, pid::RowOverrun::Carry) else {
            continue;
        };
        // Compare against the bytes the decoder actually consumed, not the
        // whole slice: a resource with trailing slack is not an encoder bug.
        let original = &p.stream[..used];
        // In `any` mode the winning rule is the one that reproduces the bytes;
        // otherwise there is only ever one candidate.
        let candidates: &[pid::LiteralRule] = if any { &ALL_RULES } else { &[rule] };
        let mut won = candidates[0];
        for (i, cand) in candidates.iter().enumerate() {
            reenc.clear();
            reenc.resize(p.encoded_len(&pixels, dims, *cand), 0);
            let Ok(n) = p.encode_into(&pixels, dims, *cand, &mut reenc) else {
                t.add("re-encode failed", &path, keep);
                break;
            };
            reenc.truncate(n);
            won = *cand;
            if reenc == original || i + 1 == candidates.len() {
                break;
            }
        }
        if any {
            *rule_wins
                .entry(if reenc == original {
                    format!("{won:?}")
                } else {
                    "NONE".to_string()
                })
                .or_default() += 1;
        }
        let g = format!("{:?}", p.header.grammar());
        let group = format!(
            "{g:<8} {}",
            r.dirs.as_slice().get(1).copied().unwrap_or("<top>")
        );
        let hit = usize::from(reenc == original);
        let e = by_grammar.entry(g).or_default();
        e.0 += hit;
        e.1 += 1;
        let e = by_group.entry(group).or_default();
        e.0 += hit;
        e.1 += 1;
        if hit == 1 {
            exact += 1;
            continue;
        }
        delta_bytes +=
            i64::try_from(reenc.len()).unwrap_or(0) - i64::try_from(original.len()).unwrap_or(0);
        match reenc.len().cmp(&original.len()) {
            std::cmp::Ordering::Equal => same_len += 1,
            std::cmp::Ordering::Less => ours_smaller += 1,
            std::cmp::Ordering::Greater => ours_larger += 1,
        }
        // Does OUR stream at least decode back to the same pixels? If yes the
        // difference is a spelling choice, not a correctness bug - and that
        // distinction is the whole value of this check.
        redec.clear();
        redec.resize(dims.pixel_len(), 0);
        let equivalent = match p.header.grammar() {
            pid::Grammar::Rle => {
                pid::decode_rle_into(&reenc, &mut redec, dims, pid::RowOverrun::Carry).is_ok()
            }
            pid::Grammar::SkipRun => {
                pid::decode_skiprun_into(&reenc, &mut redec, dims, p.header.fill_byte()).is_ok()
            }
        } && redec == pixels;
        let first = original
            .iter()
            .zip(&reenc)
            .position(|(a, b)| a != b)
            .unwrap_or(original.len().min(reenc.len()));
        let kind = if equivalent {
            "differs but decodes to the same pixels (encoder spelling)"
        } else {
            "DIFFERS AND DECODES DIFFERENTLY (real disagreement)"
        };
        t.add(
            format!("{kind}; first difference at stream byte {first}"),
            &format!("{path}  ours={} retail={}", reenc.len(), original.len()),
            keep,
        );
    }

    println!("PID sprites          : {total}");
    println!("literal rule         : {rule:?}");
    println!(
        "byte-exact round-trip: {exact}  ({:.2}%)",
        pct(exact, total)
    );
    println!(
        "differing            : {}  (same length {same_len}, ours smaller {ours_smaller}, ours larger {ours_larger})",
        total - exact
    );
    println!("net size delta       : {delta_bytes:+} bytes");
    if any {
        println!("\nwinning literal rule per sprite:");
        for (r, n) in &rule_wins {
            println!("  {n:>7}  {:>6.2}%  {r}", pct(*n, total));
        }
    }
    println!("\nby grammar:");
    for (g, (ok, n)) in &by_grammar {
        println!("  {ok:>7}/{n:<7} {:>6.2}%  {g}", pct(*ok, *n));
    }
    println!("\nby asset group (grammar / second path element):");
    for (g, (ok, n)) in by_group.iter().filter(|(_, (_, n))| *n >= 20) {
        println!("  {ok:>7}/{n:<7} {:>6.2}%  {g}", pct(*ok, *n));
    }
    if !t.counts.is_empty() {
        t.report("round-trip differences", total);
    }
}

// ---------------------------------------------------------------------------
// check 3: retail's two decoders against each other
// ---------------------------------------------------------------------------

fn decoders(rez: &Rez, keep: usize) {
    let mut total = 0usize;
    let mut agree = 0usize;
    let mut crossing = 0usize;
    let mut t = Tally::default();
    let (mut a, mut b) = (Vec::new(), Vec::new());

    for r in of_kind(rez, "PID") {
        let path = r.path().to_string();
        let data = r.data(rez.bytes());
        let Ok(p) = pid::split(data) else { continue };
        if p.header.grammar() != pid::Grammar::Rle {
            continue; // the two decoders only differ on the 0xC0 grammar
        }
        let Ok(dims) = p.header.dims() else { continue };
        total += 1;
        a.clear();
        a.resize(dims.pixel_len(), 0);
        b.clear();
        b.resize(dims.pixel_len(), 0);
        let ra = pid::decode_rle_into(p.stream, &mut a, dims, pid::RowOverrun::Carry);
        let rb = pid::decode_rle_into(p.stream, &mut b, dims, pid::RowOverrun::Spill);
        match (ra, rb) {
            (Ok(_), Ok(_)) if a == b => agree += 1,
            (Ok(_), Ok(_)) => {
                crossing += 1;
                let first = a.iter().zip(&b).position(|(x, y)| x != y).unwrap_or(0);
                t.add(
                    format!(
                        "decoders disagree from pixel {first} (row {}, col {})",
                        first / dims.width(),
                        first % dims.width()
                    ),
                    &path,
                    keep,
                );
            }
            (Ok(_), Err(e)) => t.add(
                format!("Spill failed where Carry succeeded: {e}"),
                &path,
                keep,
            ),
            (Err(e), Ok(_)) => t.add(
                format!("Carry failed where Spill succeeded: {e}"),
                &path,
                keep,
            ),
            (Err(e), Err(_)) => t.add(format!("both failed: {e}"), &path, keep),
        }
    }
    println!("Rle-grammar sprites  : {total}");
    println!(
        "both decoders agree  : {agree}  ({:.2}%)",
        pct(agree, total)
    );
    println!("row-crossing runs    : {crossing}");
    if t.counts.is_empty() {
        println!(
            "\nNo shipped sprite contains a run that crosses a scanline, so retail's\n\
             carry-vs-spill split is unobservable on this corpus."
        );
    } else {
        t.report("decoder disagreements", total);
    }
}

// ---------------------------------------------------------------------------
// check 4: the RLE16 row-end question
// ---------------------------------------------------------------------------

/// Walk a skip/fill stream under one row-end rule; return
/// `(rows completed, bytes consumed, ran out of stream)`.
fn walk_rows(stream: &[u8], width: usize, height: usize, minus_one: bool) -> (usize, usize, bool) {
    let limit = if minus_one { width - 1 } else { width };
    let (mut p, mut x, mut y) = (0usize, 0usize, 0usize);
    while y < height {
        let Some(&t) = stream.get(p) else {
            return (y, p, true);
        };
        let n = if t & 0x80 != 0 {
            p += 1;
            usize::from(t - 0x80)
        } else {
            let n = usize::from(t);
            if p + 1 + n > stream.len() {
                return (y, p, true);
            }
            p += 1 + n;
            n
        };
        x += n;
        if x >= limit {
            y += 1;
            x = 0;
        }
    }
    (y, p, false)
}

fn rle16(rez: &Rez, keep: usize) {
    let mut skiprun = 0usize;
    let mut reachable = 0usize;
    let mut agree = 0usize;
    let mut t = Tally::default();

    for r in of_kind(rez, "PID") {
        let path = r.path().to_string();
        let Ok(p) = pid::split(r.data(rez.bytes())) else {
            continue;
        };
        if p.header.grammar() != pid::Grammar::SkipRun {
            continue;
        }
        skiprun += 1;
        // CDDrawShadeBlit::Build 0x1490df/0x1490e3: either bit forces
        // m_srcBpp = 1, and EncodeRle16 runs only when m_srcBpp == 2.
        if p.header.flags & (pid::flags::SRC_8BPP_SHADE | pid::flags::SRC_8BPP) != 0 {
            continue;
        }
        reachable += 1;
        let Ok(dims) = p.header.dims() else { continue };
        let (w, h) = (dims.width(), dims.height());
        let a = walk_rows(p.stream, w, h, false); // DecodePidData: x >= width
        let b = walk_rows(p.stream, w, h, true); // EncodeRle16:    x >= width - 1
        if a == b {
            agree += 1;
        } else {
            t.add(
                format!(
                    "x>=width consumed {}/{} in {} rows{}; x>=width-1 consumed {}/{} in {} rows{}",
                    a.1,
                    p.stream.len(),
                    a.0,
                    if a.2 { " (RAN OUT)" } else { "" },
                    b.1,
                    p.stream.len(),
                    b.0,
                    if b.2 { " (RAN OUT)" } else { "" },
                ),
                &format!("{path}  {w}x{h} flags={:#06x}", p.header.flags),
                keep,
            );
        }
    }

    println!("skip/fill sprites            : {skiprun}");
    println!("reachable by EncodeRle16     : {reachable}  (neither 0x40 nor 0x200 set)");
    if reachable == 0 {
        println!(
            "\nNothing in this archive can reach EncodeRle16: every skip/fill sprite\n\
             carries 0x40 or 0x200, which forces m_srcBpp = 1 and skips the call.\n\
             The `width - 1` row terminator is therefore DEAD CODE on this data."
        );
        return;
    }
    println!(
        "both row-end rules agree     : {agree}  ({:.2}%)",
        pct(agree, reachable)
    );
    if !t.counts.is_empty() {
        t.report("row-end rule disagreements", reachable);
    }
}

// ---------------------------------------------------------------------------
// check 5: retail's own machine code (the "recomp" implementation)
// ---------------------------------------------------------------------------

const JOB_MAGIC: u32 = 0x424f_4a50; // 'PJOB'
const RES_MAGIC: u32 = 0x5345_5250; // 'PRES'

/// Batch the whole corpus through `pidrun.exe`, which maps retail GRUNTZ.EXE at
/// 0x400000 and calls `CDDSurface::RunDecode1` for real.
///
/// One wine invocation for the entire archive: 30 000 process launches would
/// dominate the runtime and prove nothing extra.
fn recomp(
    rez: &Rez,
    harness: &Path,
    exe: &Path,
    scratch: &Path,
    limit: usize,
    keep: usize,
) -> Result<(), Box<dyn std::error::Error>> {
    let mut jobs: Vec<u8> = Vec::new();
    let mut meta: Vec<(String, pid::Dims, Vec<u8>)> = Vec::new();
    jobs.extend_from_slice(&JOB_MAGIC.to_le_bytes());
    jobs.extend_from_slice(&0u32.to_le_bytes()); // patched below

    for r in of_kind(rez, "PID") {
        if limit != 0 && meta.len() >= limit {
            break;
        }
        let Ok(p) = pid::split(r.data(rez.bytes())) else {
            continue;
        };
        // RunDecode1 only implements the 0xC0 grammar; the skip/fill grammar
        // lives inside CRezImage::DecodePidData, which needs a DIB section and
        // the statically-linked CRT and is therefore out of the harness's scope.
        if p.header.grammar() != pid::Grammar::Rle {
            continue;
        }
        let Ok(dims) = p.header.dims() else { continue };
        let mut ours = vec![0u8; dims.pixel_len()];
        if p.decode_into(&mut ours, pid::RowOverrun::Carry).is_err() {
            continue;
        }
        jobs.extend_from_slice(&u32::try_from(dims.width())?.to_le_bytes());
        jobs.extend_from_slice(&u32::try_from(dims.height())?.to_le_bytes());
        jobs.extend_from_slice(&u32::try_from(p.stream.len())?.to_le_bytes());
        jobs.extend_from_slice(p.stream);
        meta.push((r.path().to_string(), dims, ours));
    }
    let n = u32::try_from(meta.len())?;
    jobs[4..8].copy_from_slice(&n.to_le_bytes());

    let job_path = scratch.join("gruntz-recomp-jobs.bin");
    let res_path = scratch.join("gruntz-recomp-results.bin");
    std::fs::write(&job_path, &jobs)?;
    let _ = std::fs::remove_file(&res_path);

    let status = std::process::Command::new("wine")
        .arg(harness)
        .arg(exe)
        .arg(&job_path)
        .arg(&res_path)
        .status()?;
    if !res_path.exists() {
        // wine returns odd exit codes and spews unrelated noise; "the output
        // file exists" is the real success signal, same as cc_wrap.py.
        return Err(format!("pidrun produced no results (wine status {status})").into());
    }

    let res = std::fs::read(&res_path)?;
    let rd =
        |at: usize| -> u32 { u32::from_le_bytes([res[at], res[at + 1], res[at + 2], res[at + 3]]) };
    if res.len() < 8 || rd(0) != RES_MAGIC {
        return Err("bad result magic".into());
    }
    let got = rd(4);
    if got != n {
        return Err(format!("harness returned {got} results, expected {n}").into());
    }

    let mut at = 8usize;
    let mut agree = 0usize;
    let mut t = Tally::default();
    for (path, dims, ours) in &meta {
        let rc = rd(at);
        let len = rd(at + 4);
        at += 8;
        let plen = usize::try_from(len)?;
        let theirs = &res[at..at + plen];
        at += plen;
        if rc != 1 {
            t.add(
                format!("retail RunDecode1 returned {rc}, not 1"),
                path,
                keep,
            );
            continue;
        }
        if theirs == ours.as_slice() {
            agree += 1;
        } else {
            let first = theirs
                .iter()
                .zip(ours)
                .position(|(a, b)| a != b)
                .unwrap_or(0);
            t.add(
                format!(
                    "pixels differ from retail at {first} (row {}, col {})",
                    first / dims.width(),
                    first % dims.width()
                ),
                path,
                keep,
            );
        }
    }
    println!("sprites through retail: {}", meta.len());
    println!(
        "identical pixels      : {agree}  ({:.2}%)",
        pct(agree, meta.len())
    );
    if t.counts.is_empty() {
        println!("\nOur decoder agrees with retail's own machine code on every sprite.");
    } else {
        t.report("differences vs retail machine code", meta.len());
    }
    Ok(())
}

// ---------------------------------------------------------------------------
// tokens
// ---------------------------------------------------------------------------

/// Decompose a `SkipRun` stream into `(offset, text, row)` tokens.
fn skiprun_tokens_of(stream: &[u8], dims: pid::Dims) -> Vec<(usize, String, usize)> {
    let mut out = Vec::new();
    let (mut p, mut x, mut y) = (0usize, 0usize, 0usize);
    while y < dims.height() && p < stream.len() {
        let at = p;
        let tok = stream[p];
        let n = if tok & 0x80 != 0 {
            let n = usize::from(tok - 0x80);
            out.push((at, format!("fill {n:>3}"), y));
            p += 1;
            n
        } else {
            let n = usize::from(tok);
            if p + 1 + n > stream.len() {
                break;
            }
            out.push((at, format!("lit  {n:>3}"), y));
            p += 1 + n;
            n
        };
        x += n;
        if x >= dims.width() {
            x = 0;
            y += 1;
        }
    }
    out
}

/// Decompose an `Rle` stream into `(offset, text, row)` tokens.
fn rle_tokens_of(stream: &[u8], dims: pid::Dims) -> Vec<(usize, String, usize)> {
    let mut out = Vec::new();
    let (mut p, mut x, mut y) = (0usize, 0usize, 0usize);
    while y < dims.height() && p < stream.len() {
        let at = p;
        let tok = stream[p];
        p += 1;
        let (text, n) = if tok & 0xc0 == 0xc0 {
            let n = usize::from(tok & 0x3f);
            let Some(&v) = stream.get(p) else { break };
            p += 1;
            (format!("run  {n:>2} x {v:#04x}"), n)
        } else {
            (format!("lit       {tok:#04x}"), 1)
        };
        out.push((at, text, y));
        x += n;
        if x >= dims.width() {
            x -= dims.width();
            y += 1;
        }
    }
    out
}

fn tokens(
    rez: &Rez,
    r: &Resource,
    limit: usize,
    rule: pid::LiteralRule,
) -> Result<(), Box<dyn std::error::Error>> {
    let p = pid::split(r.data(rez.bytes()))?;
    let dims = p.header.dims()?;
    let mut pixels = vec![0u8; dims.pixel_len()];
    p.decode_into(&mut pixels, pid::RowOverrun::Carry)?;
    let mut ours = vec![0u8; p.encoded_len(&pixels, dims, rule)];
    let n = p.encode_into(&pixels, dims, rule, &mut ours)?;
    ours.truncate(n);

    let decompose = match p.header.grammar() {
        pid::Grammar::Rle => rle_tokens_of,
        pid::Grammar::SkipRun => skiprun_tokens_of,
    };
    let a = decompose(p.stream, dims);
    let b = decompose(&ours, dims);
    println!(
        "{} : {}x{} flags={:#06x}  retail {} bytes / {} tokens   ours ({rule:?}) {} bytes / {} tokens",
        r.path(),
        dims.width(),
        dims.height(),
        p.header.flags,
        p.stream.len(),
        a.len(),
        ours.len(),
        b.len()
    );
    println!(
        "{:<6} {:<22} | {:<6} {:<22}  row",
        "off", "RETAIL", "off", "OURS"
    );
    println!("{:-<70}", "");
    for i in 0..a.len().max(b.len()).min(limit) {
        let l = a.get(i);
        let rr = b.get(i);
        let mark = if l.map(|x| &x.1) == rr.map(|x| &x.1) {
            ' '
        } else {
            '*'
        };
        let fmt1 = |t: Option<&(usize, String, usize)>| match t {
            Some((o, s, _)) => format!("{o:<6} {s:<22}"),
            None => format!("{:<6} {:<22}", "", "-"),
        };
        println!(
            "{}{} | {} {}",
            mark,
            fmt1(l),
            fmt1(rr),
            l.map(|x| x.2).unwrap_or_default()
        );
    }
    Ok(())
}

// ---------------------------------------------------------------------------
// dump
// ---------------------------------------------------------------------------

fn dump(
    rez: &Rez,
    r: &Resource,
    out: &Path,
    overrun: pid::RowOverrun,
    palette_path: Option<&str>,
) -> Result<(), Box<dyn std::error::Error>> {
    let data = r.data(rez.bytes());
    let external_palette = load_palette(rez, palette_path)?;
    match r.kind.to_string().as_str() {
        "PID" => {
            let p = pid::split(data)?;
            let dims = p.header.dims()?;
            let mut pixels = vec![0u8; dims.pixel_len()];
            let used = p.decode_into(&mut pixels, overrun)?;
            write_bmp(out, &pixels, dims, p.palette.or(external_palette))?;
            eprintln!(
                "[dump] {}x{} flags={:#06x} offset=({},{}) fill={:#x} stream={}/{} bytes palette={}",
                dims.width(),
                dims.height(),
                p.header.flags,
                p.header.offset_x,
                p.header.offset_y,
                p.header.fill,
                used,
                p.stream.len(),
                p.palette.is_some() || external_palette.is_some()
            );
        }
        "RID" => {
            let image = rid::split(data)?;
            write_bmp(out, image.pixels, image.dims, external_palette)?;
            eprintln!(
                "[dump] RID {}x{} offset=({},{}) trailing={} bytes palette={}",
                image.dims.width(),
                image.dims.height(),
                image.header.offset_x,
                image.header.offset_y,
                image.trailing.len(),
                external_palette.is_some()
            );
        }
        _ => std::fs::write(out, data)?,
    }
    Ok(())
}

fn write_bmp(
    out: &Path,
    pixels: &[u8],
    dims: pid::Dims,
    palette: Option<&[u8]>,
) -> Result<(), Box<dyn std::error::Error>> {
    let mut file = vec![0u8; bmp::indexed_len(dims.width(), dims.height())];
    let n = bmp::write_indexed_into(pixels, dims.width(), dims.height(), palette, &mut file)?;
    file.truncate(n);
    std::fs::write(out, &file)?;
    Ok(())
}

fn load_palette<'a>(
    rez: &'a Rez<'a>,
    path: Option<&str>,
) -> Result<Option<&'a [u8]>, Box<dyn std::error::Error>> {
    let Some(path) = path else { return Ok(None) };
    let resource = find_resource(rez, path).ok_or_else(|| {
        std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!("no such palette resource: {path}"),
        )
    })?;
    Ok(Some(pal::split(resource.data(rez.bytes()))?.as_bytes()))
}

#[derive(Debug, Clone, Copy)]
struct TintPalettes<'a> {
    green_tool: &'a [u8],
    green_toy: &'a [u8],
    selected_tool: &'a [u8],
    selected_toy: &'a [u8],
}

fn tint_palettes<'a>(
    rez: &'a Rez<'a>,
    tint: GruntTint,
) -> Result<Option<TintPalettes<'a>>, Box<dyn std::error::Error>> {
    let Some(stem) = tint.resource_stem() else {
        return Ok(None);
    };
    let get = |name: &str| -> Result<&'a [u8], Box<dyn std::error::Error>> {
        let path = format!("GRUNTZ\\PALETTEZ\\{name}");
        load_palette(rez, Some(&path))?
            .ok_or_else(|| std::io::Error::new(std::io::ErrorKind::NotFound, path).into())
    };
    Ok(Some(TintPalettes {
        green_tool: get("GREENTOOL")?,
        green_toy: get("GREENTOY")?,
        selected_tool: get(&format!("{stem}TOOL"))?,
        selected_toy: get(&format!("{stem}TOY"))?,
    }))
}

#[derive(Debug)]
struct FrameImage {
    width: usize,
    height: usize,
    offset_x: i32,
    offset_y: i32,
    pixels: Vec<u8>,
    palette: [u8; pid::PALETTE_SIZE],
    transparent: Option<u8>,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
struct ControlState {
    record: usize,
    frame: i32,
    x: i32,
    y: i32,
}

#[derive(Debug, Clone, Copy)]
struct PreviewState {
    frame: i32,
    x: i32,
    y: i32,
    delay_ms: u32,
}

#[derive(Debug, Clone, Copy)]
struct AniPreviewOptions<'a> {
    gif_path: Option<&'a Path>,
    frame_prefix: Option<&'a str>,
    palette_path: Option<&'a str>,
    tint: GruntTint,
    max_steps: usize,
    start_frame: Option<i32>,
}

fn inspect_ani(
    rez: &Rez,
    resource: &Resource,
    options: AniPreviewOptions<'_>,
) -> Result<(), Box<dyn std::error::Error>> {
    if resource.kind.to_string() != "ANI" {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidInput,
            format!("{} is {}, not ANI", resource.path(), resource.kind),
        )
        .into());
    }
    let animation = ani::split(resource.data(rez.bytes()))?;
    println!(
        "{}: name={:?} flags={:#x} records={} trailing={}",
        resource.path(),
        String::from_utf8_lossy(animation.name),
        animation.header.flags,
        animation.header.count,
        animation.trailing.len()
    );
    println!(
        "{:>3} {:>5} {:>5} {:>5} {:>6} {:>7} {:>5} {:>6} {:>6}  cues",
        "#", "step", "loop", "pos", "param", "time", "draw", "dx", "dy"
    );
    for (i, record) in animation.records().enumerate() {
        let cues = record
            .cues()
            .map(String::from_utf8_lossy)
            .collect::<Vec<_>>()
            .join(" ");
        println!(
            "{i:>3} {:>5} {:>5} {:>5} {:>6} {:>5}ms {:>5} {:>6} {:>6}  {cues}",
            record.step_mode,
            record.loop_mode,
            record.position_mode,
            record.param,
            record.duration_ms(),
            record.draw_value,
            record.delta_x,
            record.delta_y
        );
    }

    let Some(out) = options.gif_path else {
        return Ok(());
    };
    let prefix = options.frame_prefix.ok_or_else(|| {
        std::io::Error::new(
            std::io::ErrorKind::InvalidInput,
            "--gif requires --frames because ANI stores no image-set path",
        )
    })?;
    if options.max_steps == 0 {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidInput,
            "--steps must be greater than zero",
        )
        .into());
    }
    let frames = load_frame_set(
        rez,
        prefix,
        options.palette_path,
        tint_palettes(rez, options.tint)?,
    )?;
    let states = preview_states(&animation, &frames, options.max_steps, options.start_frame)?;
    write_ani_gif(out, &frames, &states)?;
    eprintln!(
        "[ani] rendered {} control step(s) using {} frame(s) from {prefix}",
        states.len(),
        frames.len()
    );
    Ok(())
}

fn export_all_animations(
    rez: &Rez,
    out: &Path,
    max_steps: usize,
    tint: GruntTint,
) -> Result<(), Box<dyn std::error::Error>> {
    std::fs::create_dir_all(out)?;
    let tint_palettes = tint_palettes(rez, tint)?;
    let mut total = 0usize;
    let mut generated = 0usize;
    let mut unresolved = String::from("ani\tframe_prefix\treason\n");
    let mut bindings = String::from("ani\tframe_prefix\n");
    let mut failures = Tally::default();

    for resource in of_kind(rez, "ANI") {
        total += 1;
        let path = resource.path().to_string();
        let candidates = animation_frame_prefixes(&path);
        let mut errors = Vec::new();
        let mut used = None;
        for prefix in &candidates {
            match render_animation(rez, &resource, prefix, out, max_steps, tint_palettes) {
                Ok(()) => {
                    used = Some(prefix);
                    break;
                }
                Err(e) => errors.push(e.to_string().replace(['\t', '\n'], " ")),
            }
        }
        if let Some(prefix) = used {
            generated += 1;
            bindings.push_str(&format!("{path}\t{prefix}\n"));
        } else {
            let prefixes = candidates.join("|");
            let reason = errors.join(" | ");
            unresolved.push_str(&format!("{path}\t{prefixes}\t{reason}\n"));
            let class = if errors
                .iter()
                .all(|reason| reason.starts_with("no numerically suffixed"))
            {
                "no conventional sibling IMAGEZ set"
            } else if errors
                .iter()
                .any(|reason| reason.contains("selects missing frame"))
            {
                "candidate frame set does not satisfy ANI"
            } else {
                "other render failure"
            };
            failures.add(class, &path, 10);
        }
    }

    std::fs::write(out.join("UNRESOLVED.tsv"), unresolved)?;
    std::fs::write(out.join("BINDINGS.tsv"), bindings)?;
    println!("ANI resources         : {total}");
    println!(
        "GIFs generated       : {generated}  ({:.2}%)",
        pct(generated, total)
    );
    println!(
        "unresolved manifest  : {}",
        out.join("UNRESOLVED.tsv").display()
    );
    if !failures.counts.is_empty() {
        failures.report("ANI resources requiring an external frame binding", total);
    }
    Ok(())
}

fn render_animation(
    rez: &Rez,
    resource: &Resource,
    prefix: &str,
    out: &Path,
    max_steps: usize,
    tint_palettes: Option<TintPalettes<'_>>,
) -> Result<(), Box<dyn std::error::Error>> {
    let animation = ani::split(resource.data(rez.bytes()))?;
    let frames = load_frame_set(rez, prefix, None, tint_palettes)?;
    let states = preview_states(&animation, &frames, max_steps, None)?;
    let path = resource.path().to_string();
    let mut relative = PathBuf::new();
    for component in path.split(['\\', '/']) {
        relative.push(component);
    }
    relative.set_extension("gif");
    let destination = out.join(relative);
    if let Some(parent) = destination.parent() {
        std::fs::create_dir_all(parent)?;
    }
    write_ani_gif(&destination, &frames, &states)?;
    Ok(())
}

fn animation_frame_prefixes(path: &str) -> Vec<String> {
    let parts = path.split('\\').collect::<Vec<_>>();
    let mut out = Vec::new();
    push_candidate(&mut out, path.replace("\\ANIZ\\", "\\IMAGEZ\\"));
    let Some(root) = parts.first().copied() else {
        return out;
    };
    match root {
        root if is_area_root(root) && parts.len() == 3 => {
            let image = match parts[2] {
                "ROLLINGBALLEXPLOSION" => Some("ROLLINGBALL\\EXPLOSION"),
                "ROLLINGBALLFALL" => Some("ROLLINGBALL\\FALL"),
                "ROLLINGBALLSINKDEATH" | "ROLLINGBALLSINKHOLE" | "ROLLINGBALLSINKWATER" => {
                    Some("ROLLINGBALL\\SINK")
                }
                "STATICHAZARDGO" | "STATICHAZARDIDLE" => Some("STATICHAZARD"),
                "DROPPEDOBJECT" | "DROPPEDOBJECTHIT" => Some("OBJECTDROPPER\\OBJECT"),
                "DROPPEDOBJECTSHADOW" => Some("OBJECTDROPPER\\SHADOW"),
                "TOGGLEBRIDGEDOWN" | "TOGGLEBRIDGEUP" => Some("TOGGLEWATERBRIDGE"),
                "BRIDGEDOWN" | "BRIDGEUP" => Some("WATERBRIDGE"),
                "CRUMBLEBRIDGE" => Some("CRUMBLEWATERBRIDGE"),
                "WATER3" => Some("WATER2"),
                _ => None,
            };
            if let Some(image) = image {
                push_candidate(&mut out, format!("{root}\\IMAGEZ\\{image}"));
            }
        }
        "GAME" if parts.len() == 3 => {
            let image = match parts[2] {
                "TIMEBOMBFAST" | "TIMEBOMBSLOW" => Some("TIMEBOMB"),
                "EXPLOSION1" | "EXPLOSION2" | "EXPLOSION3" => Some("EXPLOSION"),
                "WATER1" | "WATER2" | "WATER3" => Some("WATER"),
                "FLASH" => Some("LIGHTING\\FLASH"),
                "HIDDENITEM" => Some("LIGHTING\\HIDDENITEM"),
                "TARGETCURSOR" => Some("LIGHTING\\TARGETCURSOR"),
                "CHECKPOINTFLAGSET" => Some("CHECKPOINTFLAG"),
                "TELEPORTER" | "TELEPORTEROPEN" | "TELEPORTERCLOSE" => Some("WORMHOLE"),
                _ => None,
            };
            if let Some(image) = image {
                push_candidate(&mut out, format!("GAME\\IMAGEZ\\{image}"));
            }
            let bound = match parts[2] {
                "GRUNTBOMBSPRINT" => Some("GRUNTZ\\IMAGEZ\\BOMBGRUNT\\WEST\\ITEM"),
                "GRUNTFLEX" => Some("GRUNTZ\\IMAGEZ\\EXITZ"),
                "GRUNTTWITCH" => Some("GRUNTZ\\IMAGEZ\\NORMALGRUNT\\DEATH"),
                _ => None,
            };
            if let Some(bound) = bound {
                push_candidate(&mut out, bound.to_string());
            }
        }
        "GRUNTZ" => gruntz_frame_prefixes(&parts, &mut out),
        _ => {}
    }
    out
}

fn is_area_root(root: &str) -> bool {
    matches!(
        root,
        "AREA1" | "AREA2" | "AREA3" | "AREA4" | "AREA5" | "AREA6" | "AREA7" | "AREA8"
    )
}

fn gruntz_frame_prefixes(parts: &[&str], out: &mut Vec<String>) {
    let Some(group) = parts.get(2).copied() else {
        return;
    };
    if parts.len() == 3 && group == "DISAPPEAR" {
        push_candidate(out, "GRUNTZ\\IMAGEZ\\EXITZ".to_string());
        return;
    }
    let Some(action) = parts.last().copied() else {
        return;
    };
    match group {
        "EXITZ" | "GRUNTPUDDLE" | "PICKUPS" => {
            push_candidate(out, format!("GRUNTZ\\IMAGEZ\\{group}"));
        }
        "ENTRANCEZ" => {
            let suffix = if action == "DROP" { "\\DROP" } else { "" };
            push_candidate(out, format!("GRUNTZ\\IMAGEZ\\ENTRANCEZ{suffix}"));
        }
        "WARLORDZ" if parts.len() >= 5 => {
            let family = if action == "BOOTY" {
                Some("JOY")
            } else {
                action_family(action)
            };
            if let Some(family) = family {
                push_candidate(
                    out,
                    format!("GRUNTZ\\IMAGEZ\\WARLORDZ\\{}\\{family}", parts[3]),
                );
            }
        }
        "BABYWALKERGRUNT" | "GOKARTGRUNT" => {
            let suffix = if action == "TOY-BREAK" {
                "BREAK"
            } else {
                "SOUTH"
            };
            push_candidate(out, format!("GRUNTZ\\IMAGEZ\\{group}\\{suffix}"));
        }
        "BEACHBALLGRUNT" | "JACKINTHEBOXGRUNT" => {
            push_candidate(out, format!("GRUNTZ\\IMAGEZ\\{group}"));
        }
        "SCROLLGRUNT" | "JUMPROPEGRUNT" | "YOYOGRUNT" | "SQUEAKTOYGRUNT" => {
            push_candidate(out, format!("GRUNTZ\\IMAGEZ\\{group}"));
        }
        "BIGWHEELGRUNT" | "POGOSTICKGRUNT" => {
            let suffix = if action == "TOY-BREAK" {
                "BREAK"
            } else {
                "SOUTH"
            };
            push_candidate(out, format!("GRUNTZ\\IMAGEZ\\{group}\\{suffix}"));
        }
        "SPRINGGRUNT" if action == "ITEM" => {
            push_candidate(out, "GRUNTZ\\IMAGEZ\\SPRINGGRUNT\\LOSEITEM".to_string());
        }
        "DEATHZ" => {}
        _ if group.ends_with("GRUNT") => {
            if action.starts_with("PROJECTILE") {
                push_candidate(out, format!("GRUNTZ\\IMAGEZ\\{group}\\PROJECTILE\\OBJECT"));
            } else if action != "DEATH" {
                if let Some(family) = action_family(action) {
                    push_candidate(out, format!("GRUNTZ\\IMAGEZ\\{group}\\SOUTH\\{family}"));
                }
            }
        }
        _ => {}
    }
}

fn action_family(action: &str) -> Option<&'static str> {
    if action.starts_with("IDLE") {
        Some("IDLE")
    } else if action.starts_with("STRUCK") {
        Some("STRUCK")
    } else if action.starts_with("ATTACK") || action.starts_with("BLOCK") {
        Some("ATTACK")
    } else if action == "ITEM" || action == "ITEM2" || action == "USE-ITEM" {
        Some("ITEM")
    } else if action == "WALK" {
        Some("WALK")
    } else if action.starts_with("BATTLECRY") {
        Some("BATTLECRY")
    } else if action == "JOY" {
        Some("JOY")
    } else if action == "MOVING" {
        Some("MOVING")
    } else if action == "PANIC" {
        Some("PANIC")
    } else if action == "DEATH" {
        Some("DEATH")
    } else {
        None
    }
}

fn push_candidate(out: &mut Vec<String>, candidate: String) {
    if !out.contains(&candidate) {
        out.push(candidate);
    }
}

fn load_frame_set(
    rez: &Rez,
    prefix: &str,
    palette_path: Option<&str>,
    tint_palettes: Option<TintPalettes<'_>>,
) -> Result<BTreeMap<i32, FrameImage>, Box<dyn std::error::Error>> {
    let wanted = prefix.trim_end_matches(['\\', '/']).to_ascii_uppercase();
    let external_palette = load_palette(rez, palette_path)?;
    let mut frames = BTreeMap::new();
    for resource in rez.resources().flatten() {
        let path = resource.path().to_string();
        let upper = path.to_ascii_uppercase();
        if !upper.starts_with(&wanted)
            || upper
                .as_bytes()
                .get(wanted.len())
                .is_some_and(|&b| b != b'\\' && b != b'/')
        {
            continue;
        }
        let relative = upper[wanted.len()..].trim_start_matches(['\\', '/']);
        if relative.contains(['\\', '/']) {
            continue;
        }
        let kind = resource.kind.to_string();
        if kind != "PID" && kind != "RID" {
            continue;
        }
        let Some(index) = trailing_number(resource.name) else {
            continue;
        };
        let image = decode_frame(rez, &resource, external_palette, tint_palettes)?;
        if frames.insert(index, image).is_some() {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidData,
                format!("duplicate frame index {index} below {prefix}"),
            )
            .into());
        }
    }
    if frames.is_empty() {
        return Err(std::io::Error::new(
            std::io::ErrorKind::NotFound,
            format!("no numerically suffixed PID/RID frames below {prefix}"),
        )
        .into());
    }
    Ok(frames)
}

fn trailing_number(name: &str) -> Option<i32> {
    let bytes = name.as_bytes();
    let start = bytes
        .iter()
        .rposition(|b| !b.is_ascii_digit())
        .map_or(0, |i| i + 1);
    (start < bytes.len())
        .then(|| name[start..].parse().ok())
        .flatten()
}

fn decode_frame(
    rez: &Rez,
    resource: &Resource,
    external_palette: Option<&[u8]>,
    tint_palettes: Option<TintPalettes<'_>>,
) -> Result<FrameImage, Box<dyn std::error::Error>> {
    let data = resource.data(rez.bytes());
    let (header, dims, pixels, embedded): (_, _, Vec<u8>, Option<&[u8]>) =
        match resource.kind.to_string().as_str() {
            "PID" => {
                let image = pid::split(data)?;
                let dims = image.header.dims()?;
                let mut pixels = vec![0u8; dims.pixel_len()];
                image.decode_into(&mut pixels, pid::RowOverrun::Carry)?;
                (image.header, dims, pixels, image.palette)
            }
            "RID" => {
                let image = rid::split(data)?;
                (image.header, image.dims, image.pixels.to_vec(), None)
            }
            kind => {
                return Err(std::io::Error::new(
                    std::io::ErrorKind::InvalidInput,
                    format!("{} has unsupported frame type {kind}", resource.path()),
                )
                .into());
            }
        };
    let mut palette = [0u8; pid::PALETTE_SIZE];
    let source = embedded.or(external_palette).map(|source| {
        if let Some(tables) = tint_palettes {
            if source == tables.green_tool {
                return tables.selected_tool;
            }
            if source == tables.green_toy {
                return tables.selected_toy;
            }
        }
        source
    });
    if let Some(source) = source {
        palette.copy_from_slice(&source[..pid::PALETTE_SIZE]);
    } else {
        for (i, rgb) in palette.chunks_exact_mut(3).enumerate() {
            let grey = u8::try_from(i).unwrap_or(u8::MAX);
            rgb.fill(grey);
        }
    }
    let transparent = if header.flags & pid::flags::TRANSPARENCY != 0 {
        // CGruntzMgr::SetColorDepth uses RGB(255, 0, 132) as the retail
        // source colour key at 24bpp. Embedded palettes do not reserve one
        // fixed index for it: GAME sprites observed here use indices 12, 133
        // and 143, while skip/fill GRUNTZ sprites often mirror the index in
        // `fill`. GIF needs the palette index, so resolve the RGB key first.
        palette
            .chunks_exact(3)
            .position(|rgb| rgb == [0xff, 0x00, 0x84])
            .and_then(|index| u8::try_from(index).ok())
            .or_else(|| Some(header.fill_byte()))
    } else {
        None
    };
    Ok(FrameImage {
        width: dims.width(),
        height: dims.height(),
        offset_x: header.offset_x,
        offset_y: header.offset_y,
        pixels,
        palette,
        transparent,
    })
}

fn preview_states(
    animation: &ani::Ani<'_>,
    frames: &BTreeMap<i32, FrameImage>,
    max_steps: usize,
    start_frame: Option<i32>,
) -> Result<Vec<PreviewState>, Box<dyn std::error::Error>> {
    let records: Vec<_> = animation.records().collect();
    if records.is_empty() {
        return Err(
            std::io::Error::new(std::io::ErrorKind::InvalidData, "ANI has no records").into(),
        );
    }
    let min_frame = *frames.keys().next().unwrap_or(&0);
    let max_frame = *frames.keys().next_back().unwrap_or(&0);
    let mut state = ControlState {
        record: 0,
        frame: start_frame.unwrap_or(min_frame),
        x: 0,
        y: 0,
    };
    if !frames.contains_key(&state.frame) {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidInput,
            format!("initial frame {} is absent from the image set", state.frame),
        )
        .into());
    }
    let mut seen = BTreeSet::new();
    let mut out = Vec::new();
    while out.len() < max_steps && seen.insert(state) {
        let record = records[state.record];
        state.frame = match record.step_mode {
            1 => {
                let next = state.frame.saturating_add(1);
                if frames.contains_key(&next) {
                    next
                } else {
                    min_frame
                }
            }
            2 => {
                if state.frame == min_frame {
                    max_frame
                } else {
                    state.frame.saturating_sub(1)
                }
            }
            3 => i32::from(record.param),
            4 => min_frame,
            5 => max_frame,
            6 => {
                let next = state.frame.saturating_add(i32::from(record.param));
                if frames.contains_key(&next) {
                    next
                } else {
                    max_frame
                }
            }
            7 => {
                let next = state.frame.saturating_sub(i32::from(record.param));
                if frames.contains_key(&next) {
                    next
                } else {
                    min_frame
                }
            }
            _ => state.frame,
        };
        let mut plot_x = 0;
        let mut plot_y = 0;
        match record.position_mode {
            1 => {
                plot_x = i32::from(record.delta_x);
                plot_y = i32::from(record.delta_y);
            }
            2 => {
                state.x = state.x.saturating_add(i32::from(record.delta_x));
                state.y = state.y.saturating_add(i32::from(record.delta_y));
            }
            3 => {
                state.x = i32::from(record.delta_x);
                state.y = i32::from(record.delta_y);
            }
            _ => {}
        }
        out.push(PreviewState {
            frame: state.frame,
            x: state.x.saturating_add(plot_x),
            y: state.y.saturating_add(plot_y),
            delay_ms: record.duration_ms(),
        });

        let advance = match record.loop_mode {
            0 => true,
            1 => state.frame == i32::from(record.param),
            2 => state.frame == min_frame,
            3 => state.frame == max_frame,
            4 => state.frame == min_frame.saturating_add(1),
            5 => state.frame == max_frame.saturating_sub(1),
            7 => {
                state.record = usize::from(records.len() > 1);
                continue;
            }
            8 => {
                state.record = 0;
                continue;
            }
            9 => break,
            _ => false,
        };
        if advance {
            state.record += 1;
            if state.record == records.len() {
                state.record = 0;
            }
        }
    }
    if out.is_empty() {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            "ANI produced no preview states",
        )
        .into());
    }
    Ok(out)
}

fn write_ani_gif(
    out: &Path,
    frames: &BTreeMap<i32, FrameImage>,
    states: &[PreviewState],
) -> Result<(), Box<dyn std::error::Error>> {
    let representative = frames.values().next().ok_or_else(|| {
        std::io::Error::new(std::io::ErrorKind::InvalidInput, "image set has no frames")
    })?;
    let mut left = i64::MAX;
    let mut top = i64::MAX;
    let mut right = i64::MIN;
    let mut bottom = i64::MIN;
    for state in states {
        let Some(frame) = frames.get(&state.frame) else {
            continue;
        };
        let x = i64::from(state.x) + i64::from(frame.offset_x);
        let y = i64::from(state.y) + i64::from(frame.offset_y);
        left = left.min(x);
        top = top.min(y);
        right = right.max(x + i64::try_from(frame.width)?);
        bottom = bottom.max(y + i64::try_from(frame.height)?);
    }
    if left == i64::MAX {
        left = i64::from(representative.offset_x);
        top = i64::from(representative.offset_y);
        right = left + i64::try_from(representative.width)?;
        bottom = top + i64::try_from(representative.height)?;
    }
    let width = usize::try_from(right - left)?;
    let height = usize::try_from(bottom - top)?;
    let pixel_len = width.checked_mul(height).ok_or_else(|| {
        std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            "GIF canvas dimensions overflow",
        )
    })?;
    let mut canvases = Vec::with_capacity(states.len());
    for state in states {
        let frame = frames.get(&state.frame);
        let background = frame
            .and_then(|image| image.transparent)
            .or(representative.transparent)
            .unwrap_or(0);
        let mut canvas = vec![background; pixel_len];
        if let Some(frame) = frame {
            let x = usize::try_from(i64::from(state.x) + i64::from(frame.offset_x) - left)?;
            let y = usize::try_from(i64::from(state.y) + i64::from(frame.offset_y) - top)?;
            for row in 0..frame.height {
                let src = &frame.pixels[row * frame.width..(row + 1) * frame.width];
                let dst_at = (y + row) * width + x;
                canvas[dst_at..dst_at + frame.width].copy_from_slice(src);
            }
        }
        canvases.push(canvas);
    }
    let gif_frames = states
        .iter()
        .zip(&canvases)
        .map(|(state, pixels)| {
            // `WWDSTEP_SET` stores a null layer when the requested frame is
            // absent. Retail ANI uses that as a blank terminal frame, so the
            // GIF represents it with a transparent palette entry.
            let frame = frames.get(&state.frame).unwrap_or(representative);
            gif::Frame {
                pixels,
                palette: Some(&frame.palette),
                transparent: if frames.contains_key(&state.frame) {
                    frame.transparent
                } else {
                    Some(frame.transparent.unwrap_or(0))
                },
                delay_ms: state.delay_ms,
            }
        })
        .collect::<Vec<_>>();
    let bytes = gif::encode(width, height, &gif_frames)?;
    std::fs::write(out, bytes)?;
    Ok(())
}

fn pct(n: usize, total: usize) -> f64 {
    if total == 0 {
        return 0.0;
    }
    // f64 has 53 bits of mantissa; these counts are in the tens of thousands.
    let (n, total) = (
        u32::try_from(n).unwrap_or(u32::MAX),
        u32::try_from(total).unwrap_or(u32::MAX),
    );
    100.0 * f64::from(n) / f64::from(total)
}
