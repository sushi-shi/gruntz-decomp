use gruntz_rez::fec::{Fec, FecError, ENTRY_HEADER_SIZE, NAME_CAPACITY, SCRAMBLE_BASE};

fn archive(entries: &[(&str, &[u8], usize)]) -> Vec<u8> {
    let mut bytes = Vec::new();
    bytes.extend_from_slice(b"FEC");
    bytes.extend_from_slice(&1i32.to_le_bytes());
    bytes.extend_from_slice(&1i32.to_le_bytes());
    bytes.extend_from_slice(&i32::try_from(entries.len()).unwrap().to_le_bytes());
    for (slot, (name, payload, padding)) in entries.iter().enumerate() {
        let mut header = [0u8; ENTRY_HEADER_SIZE];
        header[0..4].copy_from_slice(&i32::try_from(slot + 1).unwrap().to_le_bytes());
        header[4..6].copy_from_slice(&u16::try_from(name.len()).unwrap().to_le_bytes());
        for (index, byte) in name.bytes().enumerate() {
            let delta = if index % 2 == 0 { 0x4f } else { 0x53 };
            header[6 + index] = byte.wrapping_add(delta);
        }
        let scramble = SCRAMBLE_BASE + u16::try_from(*padding).unwrap();
        header[0x106..0x108].copy_from_slice(&scramble.to_le_bytes());
        header[0x108..0x10c].copy_from_slice(&i32::try_from(payload.len()).unwrap().to_le_bytes());
        bytes.extend_from_slice(&header);
        bytes.resize(bytes.len() + padding, 0xa5);
        bytes.extend_from_slice(payload);
    }
    bytes
}

#[test]
fn walks_decodes_and_borrows_payloads() {
    let bytes = archive(&[("INTRO.SMK", &[1, 2, 3], 0), ("FINAL.SMK", &[4, 5], 7)]);
    let fec = Fec::new(&bytes).unwrap();
    assert_eq!(fec.header.file_count, 2);
    assert_eq!(fec.validate().unwrap(), 2);
    let entries: Vec<_> = fec.entries().map(Result::unwrap).collect();
    let mut name = [0u8; NAME_CAPACITY];
    assert_eq!(entries[0].decoded_name(&mut name).unwrap(), "INTRO.SMK");
    assert_eq!(entries[0].payload, [1, 2, 3]);
    assert_eq!(entries[1].decoded_name(&mut name).unwrap(), "FINAL.SMK");
    assert_eq!(entries[1].payload, [4, 5]);
}

#[test]
fn rejects_bad_scramble_and_trailing_bytes() {
    let mut bytes = archive(&[("A.SMK", &[1], 0)]);
    bytes[15 + 0x106..15 + 0x108].copy_from_slice(&(SCRAMBLE_BASE - 1).to_le_bytes());
    assert_eq!(
        Fec::new(&bytes).unwrap().validate().unwrap_err(),
        FecError::BadScramble(SCRAMBLE_BASE - 1)
    );

    let mut bytes = archive(&[("A.SMK", &[1], 0)]);
    bytes.push(0);
    assert_eq!(
        Fec::new(&bytes).unwrap().validate().unwrap_err(),
        FecError::TrailingBytes(1)
    );
}
