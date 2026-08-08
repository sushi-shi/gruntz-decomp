use gruntz_codec::bute::{self, Blowfish, ButeError};

/// Schneier's published all-zero vector — the one check that the cipher is
/// Blowfish and not a lookalike. Key and plaintext are eight zero bytes;
/// ciphertext is `4EF99745 6198DD78`.
///
/// Retail hands the halves back swapped relative to the reference
/// implementation's internal `Xl`/`Xr` (`*xr = l; *xl = r;`), which is exactly
/// what makes the returned pair read as (`xL`, `xR`) in the published order.
#[test]
fn matches_the_published_blowfish_test_vector() {
    let bf = Blowfish::new(&[0u8; 8]);
    assert_eq!(bf.encipher(0, 0), (0x4ef9_9745, 0x6198_dd78));
    assert_eq!(bf.decipher(0x4ef9_9745, 0x6198_dd78), (0, 0));
}

#[test]
fn decipher_inverts_encipher_on_the_retail_key() {
    let bf = Blowfish::attributez();
    for &(l, r) in &[
        (0u32, 0u32),
        (1, 0),
        (0, 1),
        (0xdead_beef, 0x0bad_f00d),
        (u32::MAX, u32::MAX),
    ] {
        let (cl, cr) = bf.encipher(l, r);
        assert_eq!(bf.decipher(cl, cr), (l, r), "half pair {l:#x}/{r:#x}");
    }
}

/// `"1212C"` truncated to four bytes must be the same schedule as `"1212"` —
/// this is the claim that `Blowfish_InitKey`'s hard-coded `keybytes = 4` drops
/// the literal's trailing `C`, and it is what makes the archive decode.
#[test]
fn the_fifth_key_byte_is_dead() {
    let five = Blowfish::with_key_bytes(b"1212C", 4);
    let four = Blowfish::new(b"1212");
    assert_eq!(five.encipher(0, 0), four.encipher(0, 0));
    // ... and using all five bytes is a genuinely different schedule, so the
    // truncation is not a distinction without a difference.
    let whole = Blowfish::new(b"1212C");
    assert_ne!(whole.encipher(0, 0), four.encipher(0, 0));
}

fn roundtrip(plain: &[u8]) {
    let bf = Blowfish::attributez();
    let mut stream = vec![0u8; bute::encoded_len(plain)];
    let n = bute::encode_into(&bf, plain, &mut stream).unwrap();
    assert_eq!(n, stream.len());
    assert_eq!(bute::decoded_len(&stream), Ok(plain.len()));
    let mut back = vec![0u8; plain.len()];
    assert_eq!(bute::decode_into(&bf, &stream, &mut back), Ok(plain.len()));
    assert_eq!(back, plain);
}

#[test]
fn round_trips_every_tail_length() {
    // 0..=16 covers an empty stream, both sides of one block, and the exact
    // multiple that costs an extra pad block.
    for n in 0..=16usize {
        let plain: Vec<u8> = (0..n).map(|i| (i as u8).wrapping_mul(37)).collect();
        roundtrip(&plain);
    }
}

#[test]
fn round_trips_a_realistic_bute_body() {
    let plain = b"[General]\r\n\tRezSync\t= (DWORD)3\r\n\tRezVersion\t= 3\r\n";
    roundtrip(plain);
}

#[test]
fn rejects_a_stream_that_is_not_eight_n_plus_one() {
    assert_eq!(
        bute::decoded_len(&[]),
        Err(ButeError::BadStreamLength { have: 0 })
    );
    for have in [2usize, 8, 10, 16] {
        assert_eq!(
            bute::decoded_len(&vec![0u8; have]),
            Err(ButeError::BadStreamLength { have })
        );
    }
    assert_eq!(bute::decoded_len(&[0u8; 1]), Ok(0));
    assert_eq!(bute::decoded_len(&[0u8; 9]), Ok(0));
    assert_eq!(bute::decoded_len(&[0u8; 17]), Ok(8));
}

#[test]
fn rejects_an_impossible_length_trailer() {
    let mut stream = [0u8; 17];
    stream[16] = 9;
    assert_eq!(
        bute::decoded_len(&stream),
        Err(ButeError::BadTailLength { tail: 9 })
    );
}

#[test]
fn refuses_to_overrun_a_short_output_buffer() {
    let bf = Blowfish::attributez();
    let mut stream = [0u8; 17];
    stream[16] = 8;
    let mut out = [0u8; 15];
    assert_eq!(
        bute::decode_into(&bf, &stream, &mut out),
        Err(ButeError::ShortOutput { need: 16, have: 15 })
    );
}

/// Known-plaintext: the retail archive itself.
///
/// `GAME\ATTRIBUTEZ` must come out as the bute text the engine parses, and the
/// plaintext `STATEZ\CREDITZ\CREDITZ` next to it must NOT be a bute stream —
/// together those pin both the key and the framing. Skipped when the archive
/// is not present, so the suite still runs without the retail data.
#[test]
fn decodes_the_shipped_attributez() {
    let Some(rez) = retail_rez() else {
        eprintln!("skipping: no retail Gruntz.REZ (set GRUNTZ_REZ)");
        return;
    };
    let attributez = find(&rez, "GAME\\ATTRIBUTEZ").expect("GAME\\ATTRIBUTEZ");
    assert_eq!(attributez.len(), 150_897);

    let bf = Blowfish::attributez();
    let need = bute::decoded_len(attributez).unwrap();
    assert_eq!(need, 150_893);
    let mut plain = vec![0u8; need];
    bute::decode_into(&bf, attributez, &mut plain).unwrap();

    let head = &plain[..64];
    assert!(
        head.starts_with(b"/****"),
        "ATTRIBUTEZ should open on a comment banner, got {:?}",
        core::str::from_utf8(head)
    );
    let text = String::from_utf8_lossy(&plain);
    for section in [
        "[General]",
        "[Optionz]",
        "[Cheatz]",
        "[NORMALGRUNT]",
        "[Warlordz]",
        "[Powerupz]",
        "[SG12]",
    ] {
        assert!(text.contains(section), "missing section {section}");
    }
    assert!(text.contains("RezSync"));
    assert!(text.contains("MinScrollSpeed"));
    // 509 sections is the whole file, not a lucky prefix.
    assert_eq!(
        text.lines().filter(|l| l.starts_with('[')).count(),
        509,
        "section count"
    );

    // Re-encoding must reproduce the archived bytes exactly. This is the check
    // that the framing is retail's and not merely a framing that happens to
    // decode.
    let mut again = vec![0u8; bute::encoded_len(&plain)];
    bute::encode_into(&bf, &plain, &mut again).unwrap();
    assert_eq!(again, attributez, "re-encode is not byte-identical");
}

/// The plaintext neighbour: `CREDITZ.TXT` is stored unencrypted, so treating it
/// as a bute stream must fail on the length rule rather than yield noise.
#[test]
fn creditz_is_not_a_bute_stream() {
    let Some(rez) = retail_rez() else { return };
    let creditz = find(&rez, "STATEZ\\CREDITZ\\CREDITZ").expect("CREDITZ");
    assert!(String::from_utf8_lossy(creditz).contains("Monolith"));
    assert_eq!(
        bute::decoded_len(creditz),
        Err(ButeError::BadStreamLength { have: 9639 })
    );
}

/// The still-unsolved one. Recorded as a test so the day someone finds the key
/// this fails loudly instead of going unnoticed.
#[test]
fn cheatz_does_not_open_under_the_attributez_key() {
    let Some(rez) = retail_rez() else { return };
    let cheatz = find(&rez, "STATEZ\\CREDITZ\\PALETTEZ\\CHEATZ").expect("CHEATZ");
    assert_eq!(cheatz.len(), 865);
    // It IS framed as a bute stream - 8*108 + 1, trailer 5.
    assert_eq!(bute::decoded_len(cheatz), Ok(861));

    let bf = Blowfish::attributez();
    let mut plain = vec![0u8; 861];
    bute::decode_into(&bf, cheatz, &mut plain).unwrap();
    let printable = plain
        .iter()
        .filter(|&&c| (0x20..0x7f).contains(&c) || matches!(c, b'\t' | b'\r' | b'\n'))
        .count();
    assert!(
        printable * 2 < plain.len(),
        "CHEATZ decoded under the ATTRIBUTEZ key ({printable}/861 printable) - \
         the key question is SOLVED, update tools/gruntz-codec/src/bute.rs"
    );
}

// -- retail corpus helpers -------------------------------------------------

fn retail_rez() -> Option<Vec<u8>> {
    let path = std::env::var("GRUNTZ_REZ").unwrap_or_else(|_| {
        let home = std::env::var("HOME").unwrap_or_default();
        format!("{home}/gruntz-wine/game/Gruntz.REZ")
    });
    std::fs::read(path).ok()
}

/// Minimal REZ lookup. `gruntz-codec` deliberately has no dependency on
/// `gruntz-rez`, so the test walks the directory itself rather than inverting
/// the crate layering for a fixture.
fn find<'a>(rez: &'a [u8], want: &str) -> Option<&'a [u8]> {
    let term = rez.iter().take(512).position(|&c| c == 0x1a)?;
    let u32_at =
        |at: usize| -> u32 { u32::from_le_bytes(rez[at..at + 4].try_into().expect("in bounds")) };
    let root_pos = u32_at(term + 5) as usize;
    let root_size = u32_at(term + 9) as usize;
    let mut hit = None;
    walk(rez, root_pos, root_size, &mut String::new(), want, &mut hit);
    hit
}

fn walk<'a>(
    rez: &'a [u8],
    pos: usize,
    size: usize,
    prefix: &mut String,
    want: &str,
    hit: &mut Option<&'a [u8]>,
) {
    let body = &rez[pos..pos + size];
    let mut at = 0;
    while at + 16 <= body.len() && hit.is_none() {
        let rd = |o: usize| u32::from_le_bytes(body[o..o + 4].try_into().expect("in bounds"));
        let kind = rd(at);
        let epos = rd(at + 4) as usize;
        let esize = rd(at + 8) as usize;
        at += 16;
        // resources carry id/type/num_keys before the name
        if kind == 0 {
            at += 12;
        }
        let name_end = at + body[at..].iter().position(|&c| c == 0).expect("cstr");
        let name = String::from_utf8_lossy(&body[at..name_end]).into_owned();
        at = name_end + 1;
        if kind == 0 {
            // skip the comment cstring
            at += body[at..].iter().position(|&c| c == 0).expect("cstr") + 1;
            let full = if prefix.is_empty() {
                name
            } else {
                format!("{prefix}\\{name}")
            };
            if full.eq_ignore_ascii_case(want) {
                *hit = Some(&rez[epos..epos + esize]);
            }
        } else {
            let saved = prefix.len();
            if !prefix.is_empty() {
                prefix.push('\\');
            }
            prefix.push_str(&name);
            walk(rez, epos, esize, prefix, want, hit);
            prefix.truncate(saved);
        }
    }
}
