use gruntz_codec::ani::{self, AniError, FLAG_FRAME_COUNT, FLAG_HAS_CUES, HEADER_SIZE};

fn resource(records: &[[i16; 10]], cues: &[Option<&[u8]>]) -> Vec<u8> {
    let mut out = vec![0u8; HEADER_SIZE];
    out[0..8].copy_from_slice(b"ANItest!");
    out[8..12].copy_from_slice(&7i32.to_le_bytes());
    out[12..16].copy_from_slice(&i32::try_from(records.len()).unwrap().to_le_bytes());
    out[16..20].copy_from_slice(&4u32.to_le_bytes());
    out.extend_from_slice(b"WALK");
    for (record, cue) in records.iter().zip(cues) {
        for field in record {
            out.extend_from_slice(&field.to_le_bytes());
        }
        if let Some(text) = cue {
            out.extend_from_slice(text);
            out.push(0);
        }
    }
    out
}

#[test]
fn parses_records_cues_and_roundtrips() {
    let mut a = [0i16; 10];
    a[0] = i16::from_le_bytes(FLAG_HAS_CUES.to_le_bytes());
    a[1] = 3;
    a[2] = 8;
    a[4] = 12;
    a[5] = 44;
    let mut b = [0i16; 10];
    b[0] = i16::from_le_bytes(FLAG_FRAME_COUNT.to_le_bytes());
    b[1] = 1;
    b[5] = 2;
    let bytes = resource(&[a, b], &[Some(b"STEP1  STEP2\tSTEP3"), None]);
    let parsed = ani::split(&bytes).unwrap();
    assert_eq!(parsed.name, b"WALK");
    assert_eq!(parsed.header.count, 2);
    let records: Vec<_> = parsed.records().collect();
    assert_eq!(records[0].param, 12);
    assert_eq!(records[0].duration_ms(), 44);
    assert_eq!(
        records[0].cues().collect::<Vec<_>>(),
        [b"STEP1", b"STEP2", b"STEP3"]
    );
    assert!(records[1].cue_text.is_none());
    assert_eq!(records[1].duration_ms(), 44);
    let mut encoded = vec![0u8; parsed.encoded_len()];
    let n = parsed.encode_into(&mut encoded).unwrap();
    assert_eq!(&encoded[..n], bytes);
}

#[test]
fn rejects_unterminated_cue_string() {
    let mut a = [0i16; 10];
    a[0] = i16::from_le_bytes(FLAG_HAS_CUES.to_le_bytes());
    let mut bytes = resource(&[a], &[Some(b"SOUND")]);
    bytes.pop();
    assert!(matches!(
        ani::split(&bytes),
        Err(AniError::UnterminatedCues { record: 0, .. })
    ));
}

#[test]
fn rejects_negative_record_count() {
    let mut bytes = vec![0u8; HEADER_SIZE];
    bytes[12..16].copy_from_slice(&(-1i32).to_le_bytes());
    assert_eq!(ani::split(&bytes).unwrap_err(), AniError::NegativeCount(-1));
}
