use gruntz_codec::fnt::{self, FntError};

fn font(glyphs: &[(i32, i32, &[u8])]) -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(&i32::try_from(glyphs.len()).unwrap().to_le_bytes());
    for (width, height, pixels) in glyphs {
        bytes.extend_from_slice(&width.to_le_bytes());
        bytes.extend_from_slice(&height.to_le_bytes());
        bytes.extend_from_slice(pixels);
    }
    bytes
}

#[test]
fn parses_variable_sized_glyphs_exactly() {
    let bytes = font(&[(0, 0, &[]), (2, 2, &[1, 2, 3, 4]), (1, 3, &[5, 6, 7])]);
    let parsed = fnt::split(&bytes).unwrap();
    assert_eq!(parsed.count(), 3);
    let glyphs: Vec<_> = parsed.glyphs().collect();
    assert_eq!(
        (glyphs[1].index, glyphs[1].width, glyphs[1].height),
        (1, 2, 2)
    );
    assert_eq!(glyphs[1].pixels, [1, 2, 3, 4]);
    assert_eq!(glyphs[2].pixels, [5, 6, 7]);
}

#[test]
fn rejects_negative_dimensions() {
    let bytes = font(&[(-1, 4, &[])]);
    assert_eq!(
        fnt::split(&bytes).unwrap_err(),
        FntError::NegativeDimensions {
            glyph: 0,
            width: -1,
            height: 4,
        }
    );
}

#[test]
fn rejects_truncated_pixels_and_trailing_data() {
    let bytes = font(&[(2, 2, &[1, 2, 3])]);
    assert!(matches!(
        fnt::split(&bytes),
        Err(FntError::Truncated { glyph: 0, .. })
    ));

    let mut bytes = font(&[(1, 1, &[9])]);
    bytes.push(0);
    assert_eq!(fnt::split(&bytes).unwrap_err(), FntError::TrailingBytes(1));
}
