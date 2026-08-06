//! Tiny indexed GIF89a writer for ANI previews.
//!
//! This deliberately emits literal-only LZW streams. They are larger than a
//! compressor's output, but keeping the writer local avoids adding a runtime
//! dependency to the asset oracle and makes every emitted byte auditable.

use std::io;

pub struct Frame<'a> {
    pub pixels: &'a [u8],
    /// 256 RGB triples. A short/missing palette becomes a grey ramp.
    pub palette: Option<&'a [u8]>,
    pub transparent: Option<u8>,
    pub delay_ms: u32,
}

pub fn encode(width: usize, height: usize, frames: &[Frame<'_>]) -> io::Result<Vec<u8>> {
    if width == 0 || height == 0 || frames.is_empty() {
        return Err(io::Error::new(
            io::ErrorKind::InvalidInput,
            "GIF needs non-zero dimensions and at least one frame",
        ));
    }
    let w = u16::try_from(width)
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidInput, "GIF width exceeds 65535"))?;
    let h = u16::try_from(height)
        .map_err(|_| io::Error::new(io::ErrorKind::InvalidInput, "GIF height exceeds 65535"))?;
    let pixels = width
        .checked_mul(height)
        .ok_or_else(|| io::Error::new(io::ErrorKind::InvalidInput, "GIF dimensions overflow"))?;
    if frames.iter().any(|frame| frame.pixels.len() != pixels) {
        return Err(io::Error::new(
            io::ErrorKind::InvalidInput,
            "GIF frame size does not match the canvas",
        ));
    }

    let mut out = Vec::new();
    out.extend_from_slice(b"GIF89a");
    out.extend_from_slice(&w.to_le_bytes());
    out.extend_from_slice(&h.to_le_bytes());
    out.extend_from_slice(&[0x70, 0, 0]); // no global table, 8-bit colour resolution
                                          // Repeat forever. A finite ANI still benefits from looping as a preview.
    out.extend_from_slice(b"\x21\xff\x0bNETSCAPE2.0\x03\x01\x00\x00\x00");

    for frame in frames {
        let delay = frame.delay_ms.saturating_add(5) / 10;
        let delay = u16::try_from(delay.max(1)).unwrap_or(u16::MAX);
        let transparency = u8::from(frame.transparent.is_some());
        out.extend_from_slice(&[0x21, 0xf9, 4, transparency]);
        out.extend_from_slice(&delay.to_le_bytes());
        out.push(frame.transparent.unwrap_or(0));
        out.push(0);

        out.push(0x2c);
        out.extend_from_slice(&0u16.to_le_bytes());
        out.extend_from_slice(&0u16.to_le_bytes());
        out.extend_from_slice(&w.to_le_bytes());
        out.extend_from_slice(&h.to_le_bytes());
        out.push(0x87); // local 256-entry colour table
        write_palette(&mut out, frame.palette);

        out.push(8); // LZW minimum code size
        let compressed = literal_lzw(frame.pixels);
        for block in compressed.chunks(255) {
            out.push(u8::try_from(block.len()).unwrap_or(255));
            out.extend_from_slice(block);
        }
        out.push(0);
    }
    out.push(0x3b);
    Ok(out)
}

fn write_palette(out: &mut Vec<u8>, palette: Option<&[u8]>) {
    for i in 0..256 {
        if let Some(rgb) = palette.and_then(|p| p.get(i * 3..i * 3 + 3)) {
            out.extend_from_slice(rgb);
        } else {
            let grey = u8::try_from(i).unwrap_or(u8::MAX);
            out.extend_from_slice(&[grey, grey, grey]);
        }
    }
}

/// Emit clear + literal codes in short groups. Resetting before the 9-bit
/// dictionary fills means the code width never changes, which keeps this
/// intentionally simple encoder valid without maintaining a dictionary.
fn literal_lzw(pixels: &[u8]) -> Vec<u8> {
    const CLEAR: u16 = 256;
    const END: u16 = 257;
    let mut packed = Vec::new();
    let (mut bits, mut count) = (0u32, 0u8);
    let push = |code: u16, packed: &mut Vec<u8>, bits: &mut u32, count: &mut u8| {
        *bits |= u32::from(code) << *count;
        *count += 9;
        while *count >= 8 {
            packed.push(bits.to_le_bytes()[0]);
            *bits >>= 8;
            *count -= 8;
        }
    };

    push(CLEAR, &mut packed, &mut bits, &mut count);
    for (chunk_index, chunk) in pixels.chunks(254).enumerate() {
        if chunk_index != 0 {
            push(CLEAR, &mut packed, &mut bits, &mut count);
        }
        for &pixel in chunk {
            push(u16::from(pixel), &mut packed, &mut bits, &mut count);
        }
    }
    push(END, &mut packed, &mut bits, &mut count);
    if count != 0 {
        packed.push(bits.to_le_bytes()[0]);
    }
    packed
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn writes_a_complete_multiframe_gif() {
        let a = [0, 1, 2, 3];
        let b = [3, 2, 1, 0];
        let frames = [
            Frame {
                pixels: &a,
                palette: None,
                transparent: Some(0),
                delay_ms: 22,
            },
            Frame {
                pixels: &b,
                palette: None,
                transparent: None,
                delay_ms: 100,
            },
        ];
        let gif = encode(2, 2, &frames).unwrap();
        assert_eq!(&gif[..6], b"GIF89a");
        assert_eq!(gif.last(), Some(&0x3b));
        assert_eq!(
            gif.windows(3)
                .filter(|bytes| *bytes == [0x21, 0xf9, 0x04])
                .count(),
            2
        );
    }
}
