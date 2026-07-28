//! PCX grammar tests.
//!
//! The interesting one is [`a_24bpp_image_has_no_trailing_palette`]: the
//! trailing-palette heuristic looked reasonable and was wrong, and the
//! corpus caught it. It is pinned here so it stays caught.

use gruntz_codec::pcx::{self, PcxError, HEADER_SIZE, PALETTE_SIZE};

/// Build a PCX with `planes` planes, `w * planes` bytes per scanline, whose
/// pixel data is `body` and which optionally has a `0x0C` + 768-byte tail.
fn build(w: i16, h: i16, planes: u8, body: &[u8], palette_tail: bool) -> Vec<u8> {
    let mut v = vec![0u8; HEADER_SIZE];
    v[0] = 0x0a; // manufacturer
    v[1] = 5; // version
    v[2] = 1; // encoding: RLE
    v[3] = 8; // bits per pixel
    v[4..6].copy_from_slice(&0i16.to_le_bytes()); // xmin
    v[6..8].copy_from_slice(&0i16.to_le_bytes()); // ymin
    v[8..10].copy_from_slice(&(w - 1).to_le_bytes());
    v[10..12].copy_from_slice(&(h - 1).to_le_bytes());
    v[0x41] = planes;
    let bpl = u16::try_from(w).unwrap() * u16::from(planes);
    v[0x42..0x44].copy_from_slice(&bpl.to_le_bytes());
    v.extend_from_slice(body);
    if palette_tail {
        v.push(0x0c);
        v.extend(core::iter::repeat_n(0x77u8, PALETTE_SIZE));
    }
    v
}

#[test]
fn paletted_decode_and_trailing_palette() {
    // one 4-byte scanline: C3 AA 05
    let res = build(4, 1, 1, &[0xc3, 0xaa, 0x05], true);
    let p = pcx::split(&res).unwrap();
    assert_eq!(p.planes, 1);
    assert_eq!(p.palette.unwrap().len(), PALETTE_SIZE);

    let mut dst = vec![0u8; p.pixel_len()];
    let mut scratch = vec![0u8; p.scratch_len()];
    let used = p.decode_into(&mut dst, &mut scratch).unwrap();
    assert_eq!(dst, [0xaa, 0xaa, 0xaa, 0x05]);
    assert_eq!(used, p.stream.len());
}

#[test]
fn a_24bpp_image_has_no_trailing_palette() {
    // Two pixels, three planes: R = 11 12, G = 21 22, B = 31 32.
    // Retail de-interleaves to BGR, so the output is 31 21 11, 32 22 12.
    //
    // The body is crafted so the byte 769 from the end is 0x0c - exactly the
    // trap that `STATEZ\ATTRACT\SCREENZ\TITLE3` sprang. With a plane-blind
    // heuristic 769 bytes get stripped as "the palette" and the decode runs
    // short; the marker is only meaningful for 1-plane images.
    let mut body = vec![0x11, 0x12, 0x21, 0x22, 0x31, 0x32];
    body.push(0x0c);
    body.extend(core::iter::repeat_n(0x99u8, PALETTE_SIZE));

    // Pad the scanline out so the extra bytes are legitimately consumed:
    // width 2 * 3 planes = 6 bytes per row, and the tail supplies 769 more,
    // so make it 775 bytes wide-equivalent by using enough rows.
    let res = build(2, 1, 3, &body[..6], false);
    let p = pcx::split(&res).unwrap();
    assert_eq!(p.planes, 3);
    assert!(
        p.palette.is_none(),
        "a 3-plane PCX must not claim a trailing palette"
    );

    let mut dst = vec![0u8; p.pixel_len()];
    let mut scratch = vec![0u8; p.scratch_len()];
    let used = p.decode_into(&mut dst, &mut scratch).unwrap();
    assert_eq!(dst, [0x31, 0x21, 0x11, 0x32, 0x22, 0x12]);
    assert_eq!(used, 6);

    // ...and with the trap bytes actually present, the marker is still ignored.
    let res = build(2, 130, 3, &body, false);
    let p = pcx::split(&res).unwrap();
    assert!(p.palette.is_none());
    assert_eq!(p.stream.len(), body.len());
}

#[test]
fn a_zero_count_run_is_tolerated_not_fatal() {
    // Retail's PCX loop (0x1760fc) skips a zero run without decrementing the
    // counter, where the PID decoder would spin. Two zero runs then real data.
    let res = build(4, 1, 1, &[0xc0, 0x00, 0xc0, 0x00, 0xc4, 0xbb], false);
    let p = pcx::split(&res).unwrap();
    let mut dst = vec![0u8; p.pixel_len()];
    let mut scratch = vec![0u8; p.scratch_len()];
    p.decode_into(&mut dst, &mut scratch).unwrap();
    assert_eq!(dst, [0xbb; 4]);
}

#[test]
fn retail_only_accepts_8_bits_per_pixel() {
    let mut res = build(4, 1, 1, &[0xc4, 0xaa], false);
    res[3] = 4;
    assert!(matches!(
        pcx::split(&res),
        Err(PcxError::UnsupportedDepth(4))
    ));
}

#[test]
fn only_1_and_3_planes_are_handled() {
    let res = build(4, 1, 4, &[0xc4, 0xaa], false);
    assert!(matches!(
        pcx::split(&res),
        Err(PcxError::UnsupportedPlanes(4))
    ));
}

#[test]
fn a_short_stream_names_the_row() {
    let res = build(4, 2, 1, &[0xc4, 0xaa], false);
    let p = pcx::split(&res).unwrap();
    let mut dst = vec![0u8; p.pixel_len()];
    let mut scratch = vec![0u8; p.scratch_len()];
    assert!(matches!(
        p.decode_into(&mut dst, &mut scratch),
        Err(PcxError::StreamExhausted { row: 1, .. })
    ));
}
