use gruntz_codec::rid::{self, RidError};

fn resource(width: i32, height: i32, pixels: &[u8]) -> Vec<u8> {
    let words = [
        3u32,
        0,
        u32::from_le_bytes(width.to_le_bytes()),
        u32::from_le_bytes(height.to_le_bytes()),
        u32::MAX,
        2,
        0,
        9,
    ];
    let mut out = Vec::new();
    for word in words {
        out.extend_from_slice(&word.to_le_bytes());
    }
    out.extend_from_slice(pixels);
    out
}

#[test]
fn raw_pixels_decode_and_roundtrip() {
    let bytes = resource(3, 2, &[1, 2, 3, 4, 5, 6]);
    let rid = rid::split(&bytes).unwrap();
    assert_eq!((rid.dims.width(), rid.dims.height()), (3, 2));
    assert_eq!(rid.header.offset_x, -1);
    let mut pixels = [0u8; 6];
    assert_eq!(rid.decode_into(&mut pixels).unwrap(), 6);
    assert_eq!(pixels, [1, 2, 3, 4, 5, 6]);
    let mut encoded = vec![0u8; rid.encoded_len()];
    let n = rid.encode_into(&mut encoded).unwrap();
    assert_eq!(&encoded[..n], bytes);
}

#[test]
fn short_payload_is_rejected() {
    let bytes = resource(3, 2, &[1, 2]);
    assert!(matches!(
        rid::split(&bytes),
        Err(RidError::TruncatedPixels { need: 6, have: 2 })
    ));
}
