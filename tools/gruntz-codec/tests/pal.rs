use gruntz_codec::pal::{self, PalError};

#[test]
fn parses_rgb_entries_without_copying() {
    let mut bytes = [0u8; pal::BYTE_LEN];
    bytes[3..6].copy_from_slice(&[17, 34, 51]);
    bytes[765..768].copy_from_slice(&[253, 254, 255]);

    let palette = pal::split(&bytes).unwrap();
    assert_eq!(palette.as_bytes().as_ptr(), bytes.as_ptr());
    assert_eq!(palette.entry(0), Some([0, 0, 0]));
    assert_eq!(palette.entry(1), Some([17, 34, 51]));
    assert_eq!(palette.entry(255), Some([253, 254, 255]));
    assert_eq!(palette.entry(256), None);
}

#[test]
fn rejects_non_pal_lengths() {
    assert_eq!(
        pal::split(&[0; pal::BYTE_LEN - 1]),
        Err(PalError {
            have: pal::BYTE_LEN - 1
        })
    );
    assert_eq!(
        pal::split(&[0; pal::BYTE_LEN + 1]),
        Err(PalError {
            have: pal::BYTE_LEN + 1
        })
    );
}
