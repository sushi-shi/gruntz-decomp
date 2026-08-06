use core::fmt;

use gruntz_codec::xmi::{Event, Sequence, XmiError};

/// 60 pulses/quarter at 120 BPM = Miles' 120 ticks/second clock. This metrical
/// form is more widely supported than MIDI's otherwise equivalent SMPTE form.
const DIVISION: u16 = 60;
const FIXED_TEMPO: [u8; 6] = [0xff, 0x51, 3, 0x07, 0xa1, 0x20];

#[derive(Debug)]
pub enum MidiError {
    Xmi(XmiError),
    TimestampOverflow,
    TrackTooLarge(usize),
}

impl fmt::Display for MidiError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Xmi(error) => write!(f, "{error}"),
            Self::TimestampOverflow => write!(f, "MIDI timestamp overflow"),
            Self::TrackTooLarge(size) => write!(f, "MIDI track is {size} bytes; exceeds u32"),
        }
    }
}

impl std::error::Error for MidiError {}

impl From<XmiError> for MidiError {
    fn from(value: XmiError) -> Self {
        Self::Xmi(value)
    }
}

#[derive(Debug)]
struct TimedEvent {
    time: u32,
    phase: u8,
    order: usize,
    bytes: Vec<u8>,
}

pub fn render(sequence: Sequence<'_>) -> Result<Vec<u8>, MidiError> {
    let mut timed = Vec::new();
    let mut end_time = 0u32;
    for (order, event) in sequence.events().enumerate() {
        let event = event?;
        end_time = end_time.max(event.time());
        match event {
            Event::Channel {
                time,
                status,
                data,
                data_len,
                duration,
            } => {
                let mut bytes = vec![status];
                bytes.extend_from_slice(&data[..usize::from(data_len)]);
                timed.push(TimedEvent {
                    time,
                    phase: 1,
                    order,
                    bytes,
                });
                if let Some(duration) = duration {
                    let off_time = time
                        .checked_add(duration)
                        .ok_or(MidiError::TimestampOverflow)?;
                    end_time = end_time.max(off_time);
                    timed.push(TimedEvent {
                        time: off_time,
                        // End an older note before another source event at the
                        // same tick. A zero-length note must first be started.
                        phase: if duration == 0 { 2 } else { 0 },
                        order,
                        bytes: vec![0x80 | (status & 0x0f), data[0], 0],
                    });
                }
            }
            Event::Meta { time, kind, data } => {
                // XMIDI delays are absolute 120 Hz intervals. Its tempo event
                // describes musical beat accounting, whereas retaining it in
                // metrical MIDI would incorrectly change real event timing.
                if kind == 0x2f || kind == 0x51 {
                    continue;
                }
                let mut bytes = vec![0xff, kind];
                push_vlq(
                    &mut bytes,
                    u32::try_from(data.len()).map_err(|_| MidiError::TrackTooLarge(data.len()))?,
                );
                bytes.extend_from_slice(data);
                timed.push(TimedEvent {
                    time,
                    phase: 1,
                    order,
                    bytes,
                });
            }
            Event::SysEx { time, status, data } => {
                let mut bytes = vec![status];
                push_vlq(
                    &mut bytes,
                    u32::try_from(data.len()).map_err(|_| MidiError::TrackTooLarge(data.len()))?,
                );
                bytes.extend_from_slice(data);
                timed.push(TimedEvent {
                    time,
                    phase: 1,
                    order,
                    bytes,
                });
            }
        }
    }
    timed.sort_by_key(|event| (event.time, event.phase, event.order));

    let mut track = Vec::new();
    track.push(0);
    track.extend_from_slice(&FIXED_TEMPO);
    let mut previous = 0u32;
    for event in timed {
        push_vlq(&mut track, event.time - previous);
        track.extend_from_slice(&event.bytes);
        previous = event.time;
    }
    push_vlq(&mut track, end_time - previous);
    track.extend_from_slice(&[0xff, 0x2f, 0]);

    let track_len =
        u32::try_from(track.len()).map_err(|_| MidiError::TrackTooLarge(track.len()))?;
    let mut out = Vec::with_capacity(22 + track.len());
    out.extend_from_slice(b"MThd");
    out.extend_from_slice(&6u32.to_be_bytes());
    out.extend_from_slice(&0u16.to_be_bytes());
    out.extend_from_slice(&1u16.to_be_bytes());
    out.extend_from_slice(&DIVISION.to_be_bytes());
    out.extend_from_slice(b"MTrk");
    out.extend_from_slice(&track_len.to_be_bytes());
    out.extend_from_slice(&track);
    Ok(out)
}

fn push_vlq(out: &mut Vec<u8>, value: u32) {
    let mut bytes = [0u8; 5];
    let mut at = bytes.len() - 1;
    bytes[at] = u8::try_from(value & 0x7f).unwrap_or_default();
    let mut rest = value >> 7;
    while rest != 0 {
        at -= 1;
        bytes[at] = u8::try_from(rest & 0x7f).unwrap_or_default() | 0x80;
        rest >>= 7;
    }
    out.extend_from_slice(&bytes[at..]);
}

#[cfg(test)]
mod tests {
    use gruntz_codec::xmi::Sequence;

    use super::{push_vlq, render};

    #[test]
    fn writes_midi_variable_length_quantities() {
        for (value, expected) in [
            (0, &[0][..]),
            (127, &[0x7f]),
            (128, &[0x81, 0]),
            (0x3fff, &[0xff, 0x7f]),
            (0x4000, &[0x81, 0x80, 0]),
        ] {
            let mut actual = Vec::new();
            push_vlq(&mut actual, value);
            assert_eq!(actual, expected);
        }
    }

    #[test]
    fn expands_note_durations_on_the_120_hz_timeline() {
        let events = [
            0x90, 60, 100, 10, // t=0, off at 10
            5, 0x90, 62, 100, 5, // t=5, off at 10
            0xff, 0x2f, 0, // EOT at t=5
        ];
        let midi = render(Sequence {
            timbres: None,
            branches: None,
            events: &events,
        })
        .unwrap();
        assert_eq!(&midi[..14], b"MThd\0\0\0\x06\0\0\0\x01\0\x3c");
        assert_eq!(&midi[14..18], b"MTrk");
        assert_eq!(
            &midi[22..],
            &[
                0, 0xff, 0x51, 3, 0x07, 0xa1, 0x20, // 120 BPM at PPQN 60
                0, 0x90, 60, 100, // first note
                5, 0x90, 62, 100, // second note
                5, 0x80, 60, 0, // both expire at t=10
                0, 0x80, 62, 0, 0, 0xff, 0x2f, 0,
            ]
        );
    }
}
