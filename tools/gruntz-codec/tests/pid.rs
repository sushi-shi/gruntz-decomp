//! Grammar tests for the PID codec.
//!
//! These are deliberately *small* and hand-built: the large-corpus evidence
//! lives in `gruntz-oracle`, which runs 29 798 real sprites through decode,
//! re-encode and retail's own machine code. What is pinned here is the set of
//! edge cases the corpus happens not to contain, plus the two retail
//! behaviours that a future refactor could plausibly "tidy away".

use gruntz_codec::pid::{
    self, decode_rle_into, decode_skiprun_into, encode_rle_into, encode_skiprun_into,
    encoded_rle_len, encoded_skiprun_len, Dims, FillRunCap, Grammar, LiteralRule, PidError,
    RowOverrun, HEADER_SIZE, PALETTE_SIZE,
};

fn dims(w: i32, h: i32) -> Dims {
    Dims::new(w, h).expect("test dims are valid")
}

fn decode_rle(stream: &[u8], w: i32, h: i32, o: RowOverrun) -> Result<(Vec<u8>, usize), PidError> {
    let d = dims(w, h);
    let mut out = vec![0u8; d.pixel_len()];
    let used = decode_rle_into(stream, &mut out, d, o)?;
    Ok((out, used))
}

fn encode_rle(pixels: &[u8], w: i32, h: i32, rule: LiteralRule) -> Vec<u8> {
    let d = dims(w, h);
    let mut out = vec![0u8; encoded_rle_len(pixels, d, rule)];
    let n = encode_rle_into(pixels, d, rule, &mut out).unwrap();
    out.truncate(n);
    out
}

// ---------------------------------------------------------------------------
// the 0xC0 grammar
// ---------------------------------------------------------------------------

#[test]
fn rle_run_and_literal() {
    // C3 AA  ->  AA AA AA ;  05 -> literal 0x05
    let (px, used) = decode_rle(&[0xc3, 0xaa, 0x05], 4, 1, RowOverrun::Carry).unwrap();
    assert_eq!(px, [0xaa, 0xaa, 0xaa, 0x05]);
    assert_eq!(used, 3);
}

#[test]
fn rle_high_bytes_are_legal_literals() {
    // 0x80..0xBF do not look like a run tag, so the decoder must take them as
    // literals. (The IMAGEZ exporter chose not to *emit* them that way - see
    // LiteralRule - but the decoder still has to accept them.)
    let (px, _) = decode_rle(&[0x80, 0xbf, 0x00, 0x7f], 4, 1, RowOverrun::Carry).unwrap();
    assert_eq!(px, [0x80, 0xbf, 0x00, 0x7f]);
}

#[test]
fn rle_zero_run_is_refused_not_hung() {
    // Retail's RunDecode1 does `remaining -= 0` and loops forever on `C0 xx`.
    // The corpus contains none, so this is the one place we deliberately
    // diverge from retail rather than reproduce it.
    let err = decode_rle(&[0xc0, 0xaa], 4, 1, RowOverrun::Carry).unwrap_err();
    assert!(matches!(err, PidError::ZeroRun { at: 0 }));
}

#[test]
fn rle_short_stream_reports_the_row_that_wanted_more() {
    let err = decode_rle(&[0xc2, 0xaa], 4, 1, RowOverrun::Carry).unwrap_err();
    match err {
        PidError::StreamExhausted { row, remaining, .. } => {
            assert_eq!((row, remaining), (0, 2));
        }
        other => panic!("expected StreamExhausted, got {other:?}"),
    }
}

#[test]
fn rle_destination_must_be_exact() {
    let d = dims(4, 1);
    let mut too_small = [0u8; 3];
    assert!(matches!(
        decode_rle_into(&[0xc4, 0xaa], &mut too_small, d, RowOverrun::Carry),
        Err(PidError::BadDestination { need: 4, have: 3 })
    ));
}

// ---------------------------------------------------------------------------
// the two retail decoders, on the case that separates them
// ---------------------------------------------------------------------------

#[test]
fn carry_and_spill_differ_on_a_row_crossing_run() {
    // A 6-long run on a 4-wide image: 4 pixels fit, 2 would cross into row 1.
    //
    //   Carry (RunDecode1 @0x145270)     - clamp to the row, prepend the
    //                                      remainder to the next row, and give
    //                                      row 1 only `width - carry` to fill.
    //   Spill (DecodePidData @0x176440)  - write all 6 from the row start and
    //                                      let `remaining` go negative, then
    //                                      start row 1 with a full budget.
    //
    // They do not merely paint different pixels: they consume a DIFFERENT
    // NUMBER OF TOKENS, so after one row-crossing run the two decoders are
    // reading the stream at different offsets and everything after diverges.
    // That is why it matters that the shipped encoder never emits one.
    let stream = [0xc6, 0xaa, 0xc2, 0xbb, 0xc2, 0xcc];

    let (carry, carry_used) = decode_rle(&stream, 4, 2, RowOverrun::Carry).unwrap();
    assert_eq!(carry, [0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xbb, 0xbb]);
    assert_eq!(carry_used, 4);

    let (spill, spill_used) = decode_rle(&stream, 4, 2, RowOverrun::Spill).unwrap();
    assert_eq!(spill, [0xaa, 0xaa, 0xaa, 0xaa, 0xbb, 0xbb, 0xcc, 0xcc]);
    assert_eq!(spill_used, 6);

    assert_ne!(carry, spill);
}

#[test]
fn carry_refuses_a_run_longer_than_a_scanline() {
    // Retail's carry loop is not clamped to the new row, so a run this long
    // would walk off it. Refuse rather than reproduce an out-of-bounds write.
    let err = decode_rle(&[0xc0 | 10, 0xaa], 4, 3, RowOverrun::Carry).unwrap_err();
    assert!(matches!(err, PidError::Overrun { .. }));
}

// ---------------------------------------------------------------------------
// encoder spellings
// ---------------------------------------------------------------------------

#[test]
fn encoder_never_crosses_a_scanline() {
    // Eight identical pixels on a 4-wide image must become two runs of 4, not
    // one run of 8 - the decoders disagree about the latter.
    let px = [0xaa; 8];
    assert_eq!(
        encode_rle(&px, 4, 2, LiteralRule::Decodable),
        [0xc4, 0xaa, 0xc4, 0xaa]
    );
}

#[test]
fn encoder_prefers_a_run_at_the_size_tie() {
    // Two identical pixels cost 2 bytes either way; retail spells the run.
    assert_eq!(
        encode_rle(&[0xaa, 0xaa, 0x01, 0x02], 4, 1, LiteralRule::Decodable),
        [0xc2, 0xaa, 0x01, 0x02]
    );
}

#[test]
fn literal_rules_differ_exactly_where_the_two_exporters_do() {
    // 0x50 is in 0x40..0x7f: legal as a literal, but the IMAGEZ/BOOTY exporter
    // spells it C1 50. 0x10 is below 0x40 and is a literal under both.
    let px = [0x10, 0x50, 0x01, 0x02];
    assert_eq!(
        encode_rle(&px, 4, 1, LiteralRule::Decodable),
        [0x10, 0x50, 0x01, 0x02]
    );
    assert_eq!(
        encode_rle(&px, 4, 1, LiteralRule::LowSixBits),
        [0x10, 0xc1, 0x50, 0x01, 0x02]
    );
    assert_eq!(
        encode_rle(&px, 4, 1, LiteralRule::Never),
        [0xc1, 0x10, 0xc1, 0x50, 0xc1, 0x01, 0xc1, 0x02]
    );
}

#[test]
fn every_literal_rule_round_trips_through_the_decoder() {
    let px: Vec<u8> = (0..=255u8).chain(0..=255u8).collect();
    for rule in [
        LiteralRule::Decodable,
        LiteralRule::LowSixBits,
        LiteralRule::HighBitClear,
        LiteralRule::Never,
    ] {
        let enc = encode_rle(&px, 64, 8, rule);
        let (dec, used) = decode_rle(&enc, 64, 8, RowOverrun::Carry).unwrap();
        assert_eq!(dec, px, "rule {rule:?} did not round-trip");
        assert_eq!(used, enc.len(), "rule {rule:?} left bytes unconsumed");
    }
}

#[test]
fn runs_are_capped_at_63() {
    let px = [0xaa; 128];
    // 128 = 63 + 63 + 2
    assert_eq!(
        encode_rle(&px, 128, 1, LiteralRule::Decodable),
        [0xff, 0xaa, 0xff, 0xaa, 0xc2, 0xaa]
    );
}

// ---------------------------------------------------------------------------
// the skip/fill grammar
// ---------------------------------------------------------------------------

#[test]
fn skiprun_fill_and_literal() {
    // 0x82 -> two fill pixels;  0x02 AA BB -> two literals.
    let d = dims(4, 1);
    let mut out = vec![0u8; d.pixel_len()];
    let used = decode_skiprun_into(&[0x82, 0x02, 0xaa, 0xbb], &mut out, d, 0x7f).unwrap();
    assert_eq!(out, [0x7f, 0x7f, 0xaa, 0xbb]);
    assert_eq!(used, 4);
}

#[test]
fn skiprun_encoder_stops_a_fill_at_126() {
    // The token can express 127, but the shipping exporter never emits 0xFF.
    // Reproducer in the archive:
    // GRUNTZ\IMAGEZ\NERFGUNGRUNT\PROJECTILE\SHADOW\FRAME001 (132 wide).
    let px = [0u8; 132];
    let d = dims(132, 1);

    let mut avoid = vec![0u8; encoded_skiprun_len(&px, d, 0, FillRunCap::AvoidFf)];
    let n = encode_skiprun_into(&px, d, 0, FillRunCap::AvoidFf, &mut avoid).unwrap();
    avoid.truncate(n);
    assert_eq!(avoid, [0x80 | 126, 0x80 | 6]);

    let mut full = vec![0u8; encoded_skiprun_len(&px, d, 0, FillRunCap::Full)];
    let n = encode_skiprun_into(&px, d, 0, FillRunCap::Full, &mut full).unwrap();
    full.truncate(n);
    assert_eq!(full, [0xff, 0x80 | 5]);
}

#[test]
fn skiprun_round_trips() {
    let px: Vec<u8> = (0..64u8).map(|i| if i % 3 == 0 { 0 } else { i }).collect();
    let d = dims(16, 4);
    let mut enc = vec![0u8; encoded_skiprun_len(&px, d, 0, FillRunCap::default())];
    let n = encode_skiprun_into(&px, d, 0, FillRunCap::default(), &mut enc).unwrap();
    enc.truncate(n);
    let mut dec = vec![0u8; d.pixel_len()];
    let used = decode_skiprun_into(&enc, &mut dec, d, 0).unwrap();
    assert_eq!(dec, px);
    assert_eq!(used, enc.len());
}

// ---------------------------------------------------------------------------
// container
// ---------------------------------------------------------------------------

fn build_resource(flags: u32, w: i32, h: i32, stream: &[u8], palette: bool) -> Vec<u8> {
    let mut v = Vec::new();
    v.extend_from_slice(&10u32.to_le_bytes()); // file_desc: 10 in every shipped sprite
    v.extend_from_slice(&flags.to_le_bytes());
    v.extend_from_slice(&w.to_le_bytes());
    v.extend_from_slice(&h.to_le_bytes());
    v.extend_from_slice(&(-3i32).to_le_bytes());
    v.extend_from_slice(&0i32.to_le_bytes());
    v.extend_from_slice(&0u32.to_le_bytes());
    v.extend_from_slice(&0u32.to_le_bytes());
    assert_eq!(v.len(), HEADER_SIZE);
    v.extend_from_slice(stream);
    if palette {
        v.extend(core::iter::repeat_n(0x55u8, PALETTE_SIZE));
    }
    v
}

#[test]
fn split_separates_the_trailing_palette() {
    let res = build_resource(pid::flags::EMBEDDED_PALETTE, 4, 1, &[0xc4, 0xaa], true);
    let p = pid::split(&res).unwrap();
    assert!(p.header.has_palette());
    assert_eq!(p.stream, [0xc4, 0xaa]);
    assert_eq!(p.palette.unwrap().len(), PALETTE_SIZE);
    assert_eq!(p.header.grammar(), Grammar::Rle);
}

#[test]
fn split_rejects_a_palette_that_cannot_fit() {
    let res = build_resource(pid::flags::EMBEDDED_PALETTE, 4, 1, &[0xc4, 0xaa], false);
    assert!(matches!(
        pid::split(&res),
        Err(PidError::MissingPalette { .. })
    ));
}

#[test]
fn compression_bit_selects_the_grammar() {
    let rle = build_resource(0, 4, 1, &[], false);
    let skip = build_resource(pid::flags::COMPRESSION, 4, 1, &[], false);
    assert_eq!(pid::split(&rle).unwrap().header.grammar(), Grammar::Rle);
    assert_eq!(
        pid::split(&skip).unwrap().header.grammar(),
        Grammar::SkipRun
    );
}

#[test]
fn fill_byte_follows_the_0x100_flag() {
    let mut res = build_resource(pid::flags::FILL_IS_WORD, 4, 1, &[], false);
    res[0x18..0x1c].copy_from_slice(&0x0000_12abu32.to_le_bytes());
    assert_eq!(pid::split(&res).unwrap().header.fill_byte(), 0xab);

    // Without the flag retail stamps a hard zero, whatever `fill` holds.
    let mut res = build_resource(0, 4, 1, &[], false);
    res[0x18..0x1c].copy_from_slice(&0x0000_12abu32.to_le_bytes());
    assert_eq!(pid::split(&res).unwrap().header.fill_byte(), 0x00);
}

#[test]
fn bad_dimensions_are_rejected() {
    for (w, h) in [(0, 4), (4, 0), (-4, 4), (1 << 20, 1 << 20)] {
        assert!(Dims::new(w, h).is_err(), "{w}x{h} should be rejected");
    }
}

#[test]
fn decodepid_would_reject_a_width_not_a_multiple_of_four() {
    let (ok, bad) = (
        build_resource(0, 4, 1, &[], false),
        build_resource(0, 5, 1, &[], false),
    );
    assert!(pid::split(&ok).unwrap().header.decodepid_would_accept());
    assert!(!pid::split(&bad).unwrap().header.decodepid_would_accept());
}
