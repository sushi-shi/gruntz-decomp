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

use std::collections::BTreeMap;
use std::path::{Path, PathBuf};
use std::process::ExitCode;

use clap::{Parser, Subcommand, ValueEnum};
use gruntz_codec::{bmp, pcx, pid};
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
    },
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
        Cmd::Dump { path, out, decoder } => {
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
                return match dump(&rez, &r, out, (*decoder).into()) {
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
}

fn describe_flags(f: u32) -> String {
    let mut s = Vec::new();
    let known = [
        (pid::flags::TRANSPARENCY, "TRANSPARENCY"),
        (pid::flags::VIDEO_MEMORY, "VIDEO_MEMORY?"),
        (pid::flags::SYSTEM_MEMORY, "SYSTEM_MEMORY?"),
        (pid::flags::COMPRESSION, "COMPRESSION"),
        (pid::flags::EMBEDDED_PALETTE, "EMBEDDED_PALETTE"),
        (pid::flags::FILL_IS_WORD, "FILL_IS_WORD"),
        (0x40, "unexplained(0x40)"),
        (0x200, "unexplained(0x200)"),
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
// check 4: retail's own machine code (the "recomp" implementation)
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
) -> Result<(), Box<dyn std::error::Error>> {
    let data = r.data(rez.bytes());
    if r.kind.to_string() != "PID" {
        std::fs::write(out, data)?;
        return Ok(());
    }
    let p = pid::split(data)?;
    let dims = p.header.dims()?;
    let mut pixels = vec![0u8; dims.pixel_len()];
    let used = p.decode_into(&mut pixels, overrun)?;
    let mut file = vec![0u8; bmp::indexed_len(dims.width(), dims.height())];
    let n = bmp::write_indexed_into(&pixels, dims.width(), dims.height(), p.palette, &mut file)?;
    file.truncate(n);
    std::fs::write(out, &file)?;
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
        p.palette.is_some()
    );
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
