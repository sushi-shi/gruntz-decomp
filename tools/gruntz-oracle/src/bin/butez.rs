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
        /// Two is already ~1 false positive in 10^7.
        #[arg(long, default_value_t = 3)]
        blocks: usize,
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
    let bf = Blowfish::with_key_bytes(key, cli.key_bytes);

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
            let want = blocks.max(1).min((data.len() - 1) / 8);
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
                 on {nthreads} threads, {want} probe blocks",
                alpha.len(),
                (alpha.len() as u64).pow(4)
            );
            let hits = std::sync::Mutex::new(Vec::<[u8; 4]>::new());
            std::thread::scope(|scope| {
                for t in 0..nthreads {
                    let (alpha, probe, hits) = (&alpha, &probe, &hits);
                    scope.spawn(move || {
                        let mut bf = Blowfish::attributez();
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
                                        if probe.iter().all(|blk| {
                                            let mut blk = *blk;
                                            bf.decipher_block(&mut blk);
                                            blk.iter().all(|&ch| is_text(ch))
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
                eprintln!("[butez] no key in this alphabet decodes {want} blocks to text");
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

/// Decrypt if the resource is a well-formed bute stream, else `None`.
fn decrypt(bf: &Blowfish, data: &[u8]) -> Option<Vec<u8>> {
    let need = bute::decoded_len(data).ok()?;
    let mut out = vec![0u8; need];
    bute::decode_into(bf, data, &mut out).ok()?;
    Some(out)
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
