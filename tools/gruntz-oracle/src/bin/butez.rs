//! `butez` — extract and decrypt the Blowfish-wrapped text resources of a
//! Gruntz REZ archive.
//!
//! Two of retail's four `.TXT` resources are ciphertext: `GAME\ATTRIBUTEZ`
//! (the bute file the engine parses through `g_buteMgr`) and
//! `STATEZ\CREDITZ\PALETTEZ\CHEATZ`. The cipher, key and framing are in
//! `gruntz_codec::bute`; this binary is the file IO around it.
//!
//! ```text
//! butez ~/gruntz-wine/game/Gruntz.REZ list
//! butez ~/gruntz-wine/game/Gruntz.REZ cat 'GAME\ATTRIBUTEZ'
//! butez ~/gruntz-wine/game/Gruntz.REZ dump out/            # every .TXT
//! butez ~/gruntz-wine/game/Gruntz.REZ crack 'STATEZ\CREDITZ\PALETTEZ\CHEATZ' --words keys.txt
//! ```

use std::io::Write;
use std::path::PathBuf;
use std::process::ExitCode;

use clap::{Parser, Subcommand};
use gruntz_codec::bute::{self, Blowfish};
use gruntz_rez::{Resource, Rez};

#[derive(Parser)]
#[command(
    name = "butez",
    about = "Extract / decrypt the Blowfish-wrapped .TXT resources of a Gruntz REZ"
)]
struct Cli {
    /// A .REZ or .VRZ archive.
    archive: PathBuf,
    /// Blowfish key. Retail uses "1212C" truncated to four bytes; pass the
    /// full literal and the truncation is applied for you.
    #[arg(long, default_value = "1212C", global = true)]
    key: String,
    /// How many key bytes the schedule actually consumes. Retail hard-codes 4
    /// in `Blowfish_InitKey` @0x0016f6c0.
    #[arg(long, default_value_t = 4, global = true)]
    key_bytes: usize,
    #[command(subcommand)]
    cmd: Cmd,
}

#[derive(Subcommand)]
enum Cmd {
    /// Every `.TXT` resource with its size and whether it is a bute stream.
    List,
    /// Decrypt one resource to stdout.
    Cat {
        /// Full path, e.g. `GAME\ATTRIBUTEZ`.
        path: String,
        /// Write the raw (still encrypted) bytes instead.
        #[arg(long)]
        raw: bool,
    },
    /// The `[CheatN]` table of `GAME\ATTRIBUTEZ`, with `Text` de-obfuscated.
    ///
    /// `CCheatMgr::CheckCode` @0x00023090 upper-cases the player's input and
    /// adds `0x3d` before looking it up, so the stored field is the code
    /// shifted by `+0x3d`. See `gruntz_codec::bute::CHEAT_SHIFT`.
    Cheatz {
        /// Which resource to read the table out of.
        #[arg(long, default_value = "GAME\\ATTRIBUTEZ")]
        path: String,
    },
    /// Write every `.TXT` into a directory, decrypting the bute streams.
    Dump {
        /// Destination directory (created if absent).
        outdir: PathBuf,
        /// Also dump resources of these types (repeatable 4CCs).
        #[arg(long, action = clap::ArgAction::Append)]
        r#type: Vec<String>,
    },
    /// Exhaust the FOUR key bytes that `Blowfish_InitKey` actually consumes.
    ///
    /// `CChatBoxOwner::ProcessCheatInput` @0x000205c0 takes the key from the
    /// text the player types (`Enable Cheatzfile <NAME> <KEY>`), so a resource
    /// like `STATEZ\CREDITZ\PALETTEZ\CHEATZ` has no key anywhere in the shipped
    /// files. But `Blowfish_InitKey` hard-codes `keybytes = 4`, so the search
    /// space is four bytes wide however long the developer's password was.
    Brute {
        /// Full path, e.g. `STATEZ\CREDITZ\PALETTEZ\CHEATZ`.
        path: String,
        /// Byte values to try per position.
        #[arg(long, value_enum, default_value_t = Alphabet::Printable)]
        alphabet: Alphabet,
        /// Blocks that must all decode to text before a candidate is reported.
        ///
        /// A printability probe is weak on THIS corpus, and not for the usual
        /// reason: the file opens on a `/****...` banner, so cipher blocks 1..6
        /// and 14..18 are all the SAME block. Asking for "4 blocks of text" is
        /// really asking for 2 distinct ones, and 3 junk keys clear that bar.
        /// Prefer `--known`.
        #[arg(long, default_value_t = 3)]
        blocks: usize,
        /// A file holding this resource's known leading PLAINTEXT; every whole
        /// block it covers must decrypt to it exactly.
        ///
        /// For `CHEATZ` that is not a guess: ECB block-EQUALITY is a property of
        /// the plaintext, not of the key, and CHEATZ's first 19 cipher blocks
        /// have the pattern `ABBBBBBCDEFGHIBBBBB` -- byte-for-byte the same
        /// pattern as `GAME\ATTRIBUTEZ`, whose plaintext we can read. So the
        /// first 152 bytes are the same shape. What that pins EXACTLY is the
        /// 11 repeated blocks (1..6 and 14..18): eleven copies of one 8-byte
        /// plaintext, in a text file opening on a banner, is `********`. The
        /// blocks BETWEEN them are the banner's wording, which is free to
        /// differ, so a NUL byte in this file means DON'T CARE -- the plaintext
        /// is ASCII, so a real NUL cannot collide with the wildcard.
        #[arg(long)]
        known: Option<PathBuf>,
        /// Worker threads.
        #[arg(long, default_value_t = 0)]
        threads: usize,
    },
    /// Try a wordlist against one resource and report anything that decodes to
    /// mostly-printable text. The scoring is deliberately crude: a Blowfish
    /// stream under the wrong key is ~38% printable, under the right one ~95%.
    Crack {
        /// Full path, e.g. `STATEZ\CREDITZ\PALETTEZ\CHEATZ`.
        path: String,
        /// One candidate key per line. `-` reads stdin.
        #[arg(long)]
        words: PathBuf,
        /// Also try each candidate truncated to 4, 8 and 16 bytes.
        #[arg(long)]
        truncations: bool,
        /// Report anything at or above this printable fraction.
        #[arg(long, default_value_t = 0.80)]
        threshold: f64,
    },
}

fn main() -> ExitCode {
    let cli = Cli::parse();
    let bytes = match std::fs::read(&cli.archive) {
        Ok(b) => b,
        Err(e) => {
            eprintln!("butez: {}: {e}", cli.archive.display());
            return ExitCode::FAILURE;
        }
    };
    let rez = match Rez::new(&bytes) {
        Ok(r) => r,
        Err(e) => {
            eprintln!("butez: {}: {e}", cli.archive.display());
            return ExitCode::FAILURE;
        }
    };
    let key = cli.key.as_bytes();
    if cli.key_bytes == 0 || cli.key_bytes > key.len() {
        eprintln!(
            "butez: --key-bytes {} is not in 1..={}",
            cli.key_bytes,
            key.len()
        );
        return ExitCode::FAILURE;
    }
    let explicit_key = std::env::args().any(|a| a == "--key" || a.starts_with("--key="));

    match cli.cmd {
        Cmd::List => {
            println!("{:<10} {:>9} {:>9}  PATH", "TYPE", "SIZE", "PLAIN");
            for r in rez.resources().flatten() {
                if r.kind.to_string() != "TXT" {
                    continue;
                }
                let data = r.data(rez.bytes());
                let plain = match bute::decoded_len(data) {
                    Ok(n) => format!("{n}"),
                    Err(_) => "-".into(),
                };
                println!(
                    "{:<10} {:>9} {:>9}  {}",
                    r.kind.to_string(),
                    r.size,
                    plain,
                    r.path()
                );
            }
        }
        Cmd::Cat { path, raw } => {
            let Some(r) = find(&rez, &path) else {
                eprintln!("butez: no such resource: {path}");
                return ExitCode::FAILURE;
            };
            let data = r.data(rez.bytes());
            let bf = schedule_for(&path, &cli.key, cli.key_bytes, explicit_key);
            let out = if raw {
                data.to_vec()
            } else {
                match decrypt(&bf, data) {
                    Some(p) => p,
                    None => data.to_vec(),
                }
            };
            if std::io::stdout().write_all(&out).is_err() {
                return ExitCode::FAILURE;
            }
        }
        Cmd::Cheatz { path } => {
            let Some(r) = find(&rez, &path) else {
                eprintln!("butez: no such resource: {path}");
                return ExitCode::FAILURE;
            };
            let data = r.data(rez.bytes());
            let bf = schedule_for(&path, &cli.key, cli.key_bytes, explicit_key);
            let Some(plain) = decrypt(&bf, data) else {
                eprintln!("butez: {path} is not a bute stream");
                return ExitCode::FAILURE;
            };
            let want = bute_int(&plain, "Cheatz", "NumCheatz").unwrap_or(0);
            println!(
                "{:<4} {:<28} {:>7} {:<8} COMMENT",
                "N", "CODE", "VALUE", "NONCHEAT"
            );
            let mut found = 0;
            for n in 1..=want {
                let sec = format!("Cheat{n}");
                let Some((text, comment)) = bute_str(&plain, &sec, "Text") else {
                    // NumCheatz over-counts the table: retail guards every read
                    // with `Exists(group, "Text")` (CCheatMgr::LoadCheatConfig
                    // @0x00022e60), so a gap is skipped, not an error.
                    println!("{n:<4} -- absent --");
                    continue;
                };
                let mut code = text;
                bute::cheat_deobfuscate(&mut code);
                let value = bute_int(&plain, &sec, "Value").unwrap_or(0x807b);
                let noncheat = bute_int(&plain, &sec, "NonCheat").unwrap_or(0);
                println!(
                    "{n:<4} {:<28} {value:>7} {noncheat:<8} {comment}",
                    String::from_utf8_lossy(&code)
                );
                found += 1;
            }
            eprintln!("[butez] NumCheatz={want}, {found} sections present");
        }
        Cmd::Dump { outdir, r#type } => {
            let mut want: Vec<String> = r#type.iter().map(|t| t.to_ascii_uppercase()).collect();
            if want.is_empty() {
                want.push("TXT".into());
            }
            let mut n = 0usize;
            for r in rez.resources().flatten() {
                if !want.contains(&r.kind.to_string()) {
                    continue;
                }
                let rel = r.path().to_string().replace('\\', "/");
                let dst = outdir.join(format!("{rel}.{}", r.kind.to_string().to_lowercase()));
                if let Some(parent) = dst.parent() {
                    if let Err(e) = std::fs::create_dir_all(parent) {
                        eprintln!("butez: {}: {e}", parent.display());
                        return ExitCode::FAILURE;
                    }
                }
                let data = r.data(rez.bytes());
                let bf = schedule_for(&rel, &cli.key, cli.key_bytes, explicit_key);
                let (out, how) = match decrypt(&bf, data) {
                    Some(p) => (p, "decrypted"),
                    None => (data.to_vec(), "stored"),
                };
                if let Err(e) = std::fs::write(&dst, &out) {
                    eprintln!("butez: {}: {e}", dst.display());
                    return ExitCode::FAILURE;
                }
                eprintln!("{how:>10}  {:>9}  {}", out.len(), dst.display());
                n += 1;
            }
            eprintln!("[butez] {n} resources -> {}", outdir.display());
        }
        Cmd::Brute {
            path,
            alphabet,
            blocks,
            known,
            threads,
        } => {
            let Some(r) = find(&rez, &path) else {
                eprintln!("butez: no such resource: {path}");
                return ExitCode::FAILURE;
            };
            let data = r.data(rez.bytes());
            if bute::decoded_len(data).is_err() {
                eprintln!("butez: {path} is not a bute stream");
                return ExitCode::FAILURE;
            }
            // A known-plaintext probe when we have one, else printability.
            let expect: Option<Vec<u8>> = match &known {
                Some(kp) => match std::fs::read(kp) {
                    Ok(b) => Some(b),
                    Err(e) => {
                        eprintln!("butez: {}: {e}", kp.display());
                        return ExitCode::FAILURE;
                    }
                },
                None => None,
            };
            let want = match &expect {
                Some(kp) => (kp.len() / 8).min((data.len() - 1) / 8),
                None => blocks.max(1).min((data.len() - 1) / 8),
            };
            if want == 0 {
                eprintln!("butez: nothing to probe (need at least one whole block)");
                return ExitCode::FAILURE;
            }
            let probe: Vec<[u8; 8]> = (0..want)
                .map(|b| {
                    let mut blk = [0u8; 8];
                    blk.copy_from_slice(&data[b * 8..b * 8 + 8]);
                    blk
                })
                .collect();
            let alpha = alphabet.bytes();
            let nthreads = if threads == 0 {
                std::thread::available_parallelism().map_or(4, |n| n.get())
            } else {
                threads
            };
            eprintln!(
                "[butez] brute-forcing 4 key bytes over {} values ({} candidates) \
                 on {nthreads} threads, {want} probe blocks ({})",
                alpha.len(),
                (alpha.len() as u64).pow(4),
                if expect.is_some() {
                    "known plaintext"
                } else {
                    "printability"
                }
            );
            let hits = std::sync::Mutex::new(Vec::<[u8; 4]>::new());
            std::thread::scope(|scope| {
                for t in 0..nthreads {
                    let (alpha, probe, hits, expect) = (&alpha, &probe, &hits, &expect);
                    scope.spawn(move || {
                        let mut bf = Blowfish::attributez(); // rekeyed per candidate
                        let mut key = [0u8; 4];
                        for (i0, &a) in alpha.iter().enumerate() {
                            if i0 % nthreads != t {
                                continue;
                            }
                            key[0] = a;
                            for &b in alpha.iter() {
                                key[1] = b;
                                for &c in alpha.iter() {
                                    key[2] = c;
                                    for &d in alpha.iter() {
                                        key[3] = d;
                                        bf.rekey(&key, 4);
                                        if probe.iter().enumerate().all(|(i, blk)| {
                                            let mut blk = *blk;
                                            bf.decipher_block(&mut blk);
                                            match expect {
                                                // NUL is the wildcard: the
                                                // plaintext is ASCII text, so a
                                                // real NUL cannot occur, and the
                                                // known bytes are not contiguous
                                                // (see --known).
                                                Some(kp) => blk
                                                    .iter()
                                                    .zip(&kp[i * 8..i * 8 + 8])
                                                    .all(|(&g, &w)| w == 0 || g == w),
                                                None => blk.iter().all(|&ch| is_text(ch)),
                                            }
                                        }) {
                                            hits.lock().expect("lock").push(key);
                                        }
                                    }
                                }
                            }
                        }
                    });
                }
            });
            let hits = hits.into_inner().expect("lock");
            if hits.is_empty() {
                eprintln!("[butez] EXHAUSTED: no key in this alphabet reproduces {want} block(s)");
                return ExitCode::FAILURE;
            }
            let need = bute::decoded_len(data).expect("checked");
            let mut out = vec![0u8; need];
            for key in &hits {
                let bf = Blowfish::new(key);
                bute::decode_into(&bf, data, &mut out).expect("checked");
                println!(
                    "HIT key={:?} printable={:.3}",
                    String::from_utf8_lossy(key),
                    printable(&out)
                );
                println!("{}", String::from_utf8_lossy(&out[..out.len().min(300)]));
            }
        }
        Cmd::Crack {
            path,
            words,
            truncations,
            threshold,
        } => {
            let Some(r) = find(&rez, &path) else {
                eprintln!("butez: no such resource: {path}");
                return ExitCode::FAILURE;
            };
            let data = r.data(rez.bytes());
            let need = match bute::decoded_len(data) {
                Ok(n) => n,
                Err(e) => {
                    eprintln!("butez: {path}: {e}");
                    return ExitCode::FAILURE;
                }
            };
            let list = if words == *std::path::Path::new("-") {
                std::io::read_to_string(std::io::stdin()).unwrap_or_default()
            } else {
                match std::fs::read_to_string(&words) {
                    Ok(s) => s,
                    Err(e) => {
                        eprintln!("butez: {}: {e}", words.display());
                        return ExitCode::FAILURE;
                    }
                }
            };
            let mut buf = vec![0u8; need];
            let mut tried = 0usize;
            let mut hits = 0usize;
            let mut best = (0.0f64, String::new(), 0usize);
            for line in list.lines() {
                let cand = line.trim_end_matches(['\r', '\n']);
                if cand.is_empty() {
                    continue;
                }
                let lens: Vec<usize> = if truncations {
                    [4usize, 8, 16, cand.len()]
                        .into_iter()
                        .filter(|&n| n > 0 && n <= cand.len())
                        .collect()
                } else {
                    vec![cand.len()]
                };
                for kb in lens {
                    let bf = Blowfish::with_key_bytes(cand.as_bytes(), kb);
                    if bute::decode_into(&bf, data, &mut buf).is_err() {
                        continue;
                    }
                    tried += 1;
                    let score = printable(&buf);
                    if score > best.0 {
                        best = (score, cand.to_string(), kb);
                    }
                    if score >= threshold {
                        hits += 1;
                        println!("HIT {score:.3}  key={cand:?} key_bytes={kb}");
                        println!("{}", String::from_utf8_lossy(&buf[..buf.len().min(200)]));
                    }
                }
            }
            eprintln!(
                "[butez] {tried} candidates, {hits} hits; best {:.3} with key={:?} key_bytes={}",
                best.0, best.1, best.2
            );
            if hits == 0 {
                return ExitCode::FAILURE;
            }
        }
    }
    ExitCode::SUCCESS
}

/// The schedule for one resource. Retail keys its two encrypted resources
/// differently (`"1212C"` truncated to 4 for ATTRIBUTEZ, the typed `K3V1` for
/// CHEATZ), so `list`/`cat`/`dump` pick by path unless `--key` was given.
fn schedule_for(path: &str, cli_key: &str, key_bytes: usize, explicit: bool) -> Blowfish {
    if !explicit && path.to_ascii_uppercase().ends_with("CHEATZ") {
        return Blowfish::cheatz();
    }
    Blowfish::with_key_bytes(cli_key.as_bytes(), key_bytes)
}

/// Decrypt if the resource is a well-formed bute stream, else `None`.
fn decrypt(bf: &Blowfish, data: &[u8]) -> Option<Vec<u8>> {
    let need = bute::decoded_len(data).ok()?;
    let mut out = vec![0u8; need];
    bute::decode_into(bf, data, &mut out).ok()?;
    Some(out)
}

/// The body of `[section]` in a decrypted bute file, `//` comments included.
///
/// Deliberately a scanner and not a parser: `CButeMgr::Parse` builds a tree we
/// have no need of here, and the file is line-oriented `key<tabs>= value`.
fn bute_section<'a>(plain: &'a [u8], section: &str) -> Option<&'a [u8]> {
    let head = format!("[{section}]");
    let mut at = 0;
    while at < plain.len() {
        let end = plain[at..]
            .iter()
            .position(|&c| c == b'\n')
            .map_or(plain.len(), |n| at + n);
        let line = plain[at..end]
            .strip_suffix(b"\r")
            .unwrap_or(&plain[at..end]);
        if line == head.as_bytes() {
            let body = &plain[end.min(plain.len())..];
            // to the next section header, or end of file
            let mut scan = 0;
            while scan < body.len() {
                let e = body[scan..]
                    .iter()
                    .position(|&c| c == b'\n')
                    .map_or(body.len(), |n| scan + n);
                if body[scan..e].starts_with(b"[") {
                    return Some(&body[..scan]);
                }
                scan = e + 1;
            }
            return Some(body);
        }
        at = end + 1;
    }
    None
}

/// `key = <value>` inside `[section]`, split into the value and its trailing
/// `//` comment. A quoted value is returned unquoted and undecoded.
fn bute_str(plain: &[u8], section: &str, key: &str) -> Option<(Vec<u8>, String)> {
    let body = bute_section(plain, section)?;
    for line in body.split(|&c| c == b'\n') {
        let line = line.strip_suffix(b"\r").unwrap_or(line);
        // A blank or all-whitespace line is not an error, it is just not this
        // key -- skip it. (`?` here would abandon the whole scan on the empty
        // line that follows every section header.)
        let Some(s) = line.iter().position(|c| !c.is_ascii_whitespace()) else {
            continue;
        };
        let t = &line[s..];
        if !t.starts_with(key.as_bytes()) {
            continue;
        }
        let rest = &t[key.len()..];
        if !rest
            .iter()
            .next()
            .is_some_and(|c| c.is_ascii_whitespace() || *c == b'=')
        {
            continue; // `Value` must not match `ValueTwo`
        }
        let Some(eq) = rest.iter().position(|&c| c == b'=') else {
            continue;
        };
        let val = &rest[eq + 1..];
        let val = &val[val
            .iter()
            .position(|c| !c.is_ascii_whitespace())
            .unwrap_or(val.len())..];
        // a quoted value ends at its closing quote; the rest of the line is comment
        let (text, tail) = if val.first() == Some(&b'"') {
            let close = val[1..].iter().position(|&c| c == b'"')? + 1;
            (val[1..close].to_vec(), &val[close + 1..])
        } else {
            let end = val.windows(2).position(|w| w == b"//").unwrap_or(val.len());
            (val[..end].to_vec(), &val[end..])
        };
        let comment = String::from_utf8_lossy(tail).trim().to_string();
        return Some((text, comment));
    }
    None
}

/// The same, parsed as an integer. `(DWORD)n` and bare `n` both read as `n`;
/// the annotation only decides which accessor retail may use, not the digits.
fn bute_int(plain: &[u8], section: &str, key: &str) -> Option<i64> {
    let (v, _) = bute_str(plain, section, key)?;
    let s = String::from_utf8_lossy(&v);
    let s = s.trim().trim_start_matches("(DWORD)").trim();
    s.parse().ok()
}

/// Which byte values a brute-force position may take.
#[derive(Clone, Copy, clap::ValueEnum)]
enum Alphabet {
    /// `a-z0-9` — a lowercase typed password (36^4 = 1.7M).
    Lower,
    /// `A-Za-z0-9` (62^4 = 14.8M).
    Alnum,
    /// Every printable ASCII byte, 0x20..0x7e (95^4 = 81.5M).
    Printable,
    /// All 256 byte values (4.3G — hours, not minutes).
    Any,
}

impl Alphabet {
    fn bytes(self) -> Vec<u8> {
        match self {
            Alphabet::Lower => (b'a'..=b'z').chain(b'0'..=b'9').collect(),
            Alphabet::Alnum => (b'A'..=b'Z')
                .chain(b'a'..=b'z')
                .chain(b'0'..=b'9')
                .collect(),
            Alphabet::Printable => (0x20u8..=0x7e).collect(),
            Alphabet::Any => (0u8..=255).collect(),
        }
    }
}

fn is_text(c: u8) -> bool {
    (0x20..0x7f).contains(&c) || matches!(c, b'\t' | b'\r' | b'\n')
}

fn printable(b: &[u8]) -> f64 {
    if b.is_empty() {
        return 0.0;
    }
    let n = b
        .iter()
        .filter(|&&c| (0x20..0x7f).contains(&c) || matches!(c, b'\t' | b'\r' | b'\n'))
        .count();
    n as f64 / b.len() as f64
}

fn find<'a>(rez: &Rez<'a>, path: &str) -> Option<Resource<'a>> {
    let up = path.to_ascii_uppercase().replace('/', "\\");
    rez.resources()
        .flatten()
        .find(|r| r.path().to_string().to_ascii_uppercase() == up)
}
