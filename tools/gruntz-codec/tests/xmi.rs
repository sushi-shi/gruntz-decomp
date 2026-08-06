use gruntz_codec::xmi::{self, Event, XmiError};

fn chunk(id: &[u8; 4], data: &[u8]) -> Vec<u8> {
    let mut out = Vec::from(id.as_slice());
    out.extend_from_slice(&u32::try_from(data.len()).unwrap().to_be_bytes());
    out.extend_from_slice(data);
    if data.len() & 1 != 0 {
        out.push(0);
    }
    out
}

fn form(kind: &[u8; 4], body: &[u8]) -> Vec<u8> {
    let mut data = Vec::from(kind.as_slice());
    data.extend_from_slice(body);
    chunk(b"FORM", &data)
}

fn resource(events: &[u8]) -> Vec<u8> {
    let directory = form(b"XDIR", &chunk(b"INFO", &[1, 0]));
    let mut sequence_body = chunk(b"TIMB", &[1, 0, 5, 2]);
    sequence_body.extend_from_slice(&chunk(b"EVNT", events));
    let sequence = form(b"XMID", &sequence_body);
    let mut catalog_data = Vec::from(b"XMID".as_slice());
    catalog_data.extend_from_slice(&sequence);
    let mut out = directory;
    out.extend_from_slice(&chunk(b"CAT ", &catalog_data));
    out
}

#[test]
fn parses_sequence_events_and_note_duration() {
    let bytes = resource(&[
        0xc0, 5, // program at t=0
        10, 0x90, 60, 100, 60, // note at t=10, duration=60
        1, 0xff, 0x51, 3, 7, 0xa1, 0x20, // tempo at t=11
        0xff, 0x2f, 0, // end at the same time
    ]);
    let xmi = xmi::split(&bytes).unwrap();
    assert_eq!(xmi.declared_sequences, 1);
    let sequence = xmi.sequences().next().unwrap().unwrap();
    assert_eq!(sequence.timbre_count().unwrap(), 1);
    let events = sequence.events().collect::<Result<Vec<_>, _>>().unwrap();
    assert_eq!(
        events[0],
        Event::Channel {
            time: 0,
            status: 0xc0,
            data: [5, 0],
            data_len: 1,
            duration: None,
        }
    );
    assert_eq!(
        events[1],
        Event::Channel {
            time: 10,
            status: 0x90,
            data: [60, 100],
            data_len: 2,
            duration: Some(60),
        }
    );
    assert!(matches!(
        events[2],
        Event::Meta {
            time: 11,
            kind: 0x51,
            data: [7, 0xa1, 0x20]
        }
    ));
}

#[test]
fn rejects_sequence_count_disagreement() {
    let mut bytes = resource(&[0xff, 0x2f, 0]);
    bytes[20] = 2;
    assert!(matches!(
        xmi::split(&bytes),
        Err(XmiError::SequenceCount {
            declared: 2,
            found: 1
        })
    ));
}

#[test]
fn rejects_status_bit_in_channel_data() {
    let bytes = resource(&[0x90, 60, 0x80, 1, 0xff, 0x2f, 0]);
    assert!(matches!(xmi::split(&bytes), Err(XmiError::BadData { .. })));
}
