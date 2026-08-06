//! **WWD** — WAP World Data level geometry.
//!
//! WWD stores a fixed 0x5f4-byte file header followed by a main block. The
//! main block may be zlib-compressed; decompression deliberately stays in the
//! `std`-using caller, while this module validates and borrows the resulting
//! uncompressed image. Each plane contains a tile grid and a compact list of
//! image-set registry keys. A non-empty tile handle is split exactly as retail
//! `CDDrawWorkerHost::Draw` @0x162010 does: high 16 bits select the image set
//! and low 16 bits select its frame.

use core::{fmt, str};

use gruntz_cast::AsUsize;

pub const HEADER_SIZE: usize = 0x5f4;
pub const PLANE_HEADER_SIZE: usize = 0xa0;
pub const FLAG_COMPRESSED: u32 = 0x2;
pub const PLANE_FLAG_MAIN: u32 = 0x1;
pub const TILE_CLEAR: u32 = u32::MAX;
pub const TILE_FILL: u32 = 0xeeee_eeee;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum WwdError {
    Truncated {
        need: usize,
        have: usize,
    },
    BadHeaderSize(u32),
    BadPlaneHeaderSize {
        plane: usize,
        size: u32,
    },
    BadDimensions {
        plane: usize,
        width: i32,
        height: i32,
    },
    RangeOverflow,
    OffsetOutOfBounds {
        offset: u32,
        need: usize,
        have: usize,
    },
    InvalidText,
    MissingImageSetName {
        plane: usize,
        index: usize,
    },
}

impl fmt::Display for WwdError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            Self::Truncated { need, have } => {
                write!(f, "WWD has {have} bytes, need at least {need}")
            }
            Self::BadHeaderSize(size) => {
                write!(f, "WWD header size is {size:#x}, expected {HEADER_SIZE:#x}")
            }
            Self::BadPlaneHeaderSize { plane, size } => write!(
                f,
                "WWD plane {plane} header size is {size:#x}, expected {PLANE_HEADER_SIZE:#x}"
            ),
            Self::BadDimensions {
                plane,
                width,
                height,
            } => {
                write!(f, "WWD plane {plane} has invalid grid {width}x{height}")
            }
            Self::RangeOverflow => f.write_str("WWD byte range overflows usize"),
            Self::OffsetOutOfBounds { offset, need, have } => write!(
                f,
                "WWD offset {offset:#x} needs {need} byte(s), image has {have}"
            ),
            Self::InvalidText => f.write_str("WWD contains non-ASCII text"),
            Self::MissingImageSetName { plane, index } => {
                write!(f, "WWD plane {plane} image-set name {index} is missing")
            }
        }
    }
}

impl core::error::Error for WwdError {}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Header<'a> {
    pub header_size: u32,
    pub flags: u32,
    pub level_name: &'a str,
    pub author: &'a str,
    pub created: &'a str,
    pub rez_file: &'a str,
    pub tile_directory: &'a str,
    pub palette: &'a str,
    pub start_x: i32,
    pub start_y: i32,
    pub num_planes: u32,
    pub planes_offset: u32,
    pub tile_descriptions_offset: u32,
    pub main_block_length: u32,
    pub checksum: u32,
    pub launch_app: &'a str,
    pub image_directories: [&'a str; 4],
    pub image_prefixes: [&'a str; 4],
}

impl Header<'_> {
    pub fn compressed(self) -> bool {
        self.flags & FLAG_COMPRESSED != 0
    }

    pub fn uncompressed_len(self) -> Result<usize, WwdError> {
        HEADER_SIZE
            .checked_add(self.main_block_length.as_usize())
            .ok_or(WwdError::RangeOverflow)
    }
}

pub fn header(bytes: &[u8]) -> Result<Header<'_>, WwdError> {
    let fixed = bytes.get(..HEADER_SIZE).ok_or(WwdError::Truncated {
        need: HEADER_SIZE,
        have: bytes.len(),
    })?;
    let result = Header {
        header_size: word(fixed, 0x00),
        flags: word(fixed, 0x08),
        level_name: cstr(&fixed[0x10..0x50])?,
        author: cstr(&fixed[0x50..0x90])?,
        created: cstr(&fixed[0x90..0xd0])?,
        rez_file: cstr(&fixed[0xd0..0x1d0])?,
        tile_directory: cstr(&fixed[0x1d0..0x250])?,
        palette: cstr(&fixed[0x250..0x2d0])?,
        start_x: signed(fixed, 0x2d0),
        start_y: signed(fixed, 0x2d4),
        num_planes: word(fixed, 0x2dc),
        planes_offset: word(fixed, 0x2e0),
        tile_descriptions_offset: word(fixed, 0x2e4),
        main_block_length: word(fixed, 0x2e8),
        checksum: word(fixed, 0x2ec),
        launch_app: cstr(&fixed[0x2f4..0x374])?,
        image_directories: [
            cstr(&fixed[0x374..0x3f4])?,
            cstr(&fixed[0x3f4..0x474])?,
            cstr(&fixed[0x474..0x4f4])?,
            cstr(&fixed[0x4f4..0x574])?,
        ],
        image_prefixes: [
            cstr(&fixed[0x574..0x594])?,
            cstr(&fixed[0x594..0x5b4])?,
            cstr(&fixed[0x5b4..0x5d4])?,
            cstr(&fixed[0x5d4..0x5f4])?,
        ],
    };
    if result.header_size.as_usize() != HEADER_SIZE {
        return Err(WwdError::BadHeaderSize(result.header_size));
    }
    Ok(result)
}

#[derive(Debug, Clone, Copy)]
pub struct Wwd<'a> {
    bytes: &'a [u8],
    pub header: Header<'a>,
}

pub fn split(image: &[u8]) -> Result<Wwd<'_>, WwdError> {
    let parsed = header(image)?;
    let need = parsed.uncompressed_len()?;
    if image.len() < need {
        return Err(WwdError::Truncated {
            need,
            have: image.len(),
        });
    }
    let world = Wwd {
        bytes: &image[..need],
        header: parsed,
    };
    for plane in world.planes() {
        plane?;
    }
    Ok(world)
}

impl<'a> Wwd<'a> {
    pub fn planes(&self) -> Planes<'a> {
        Planes {
            bytes: self.bytes,
            offset: self.header.planes_offset.as_usize(),
            count: self.header.num_planes.as_usize(),
            index: 0,
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub struct Plane<'a> {
    pub index: usize,
    pub flags: u32,
    pub name: &'a str,
    pub pixel_width: i32,
    pub pixel_height: i32,
    pub tile_width: usize,
    pub tile_height: usize,
    pub tiles_wide: usize,
    pub tiles_high: usize,
    pub scroll_x: i32,
    pub scroll_y: i32,
    pub movement_x_percent: i32,
    pub movement_y_percent: i32,
    pub fill_color: u32,
    pub z_coord: i32,
    tiles: &'a [u8],
    image_sets: ImageSets<'a>,
}

impl<'a> Plane<'a> {
    pub fn is_main(self) -> bool {
        self.flags & PLANE_FLAG_MAIN != 0
    }

    pub fn pixel_size(self) -> Result<(usize, usize), WwdError> {
        let width = self
            .tiles_wide
            .checked_mul(self.tile_width)
            .ok_or(WwdError::RangeOverflow)?;
        let height = self
            .tiles_high
            .checked_mul(self.tile_height)
            .ok_or(WwdError::RangeOverflow)?;
        Ok((width, height))
    }

    pub fn tiles(self) -> Tiles<'a> {
        Tiles { bytes: self.tiles }
    }

    pub fn image_sets(self) -> ImageSets<'a> {
        self.image_sets
    }
}

pub struct Planes<'a> {
    bytes: &'a [u8],
    offset: usize,
    count: usize,
    index: usize,
}

impl<'a> Iterator for Planes<'a> {
    type Item = Result<Plane<'a>, WwdError>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.index == self.count {
            return None;
        }
        let index = self.index;
        self.index += 1;
        let at = match index
            .checked_mul(PLANE_HEADER_SIZE)
            .and_then(|n| self.offset.checked_add(n))
        {
            Some(at) => at,
            None => return Some(Err(WwdError::RangeOverflow)),
        };
        Some(parse_plane(self.bytes, at, index))
    }
}

fn parse_plane(bytes: &[u8], at: usize, index: usize) -> Result<Plane<'_>, WwdError> {
    let h = bytes
        .get(
            at..at
                .checked_add(PLANE_HEADER_SIZE)
                .ok_or(WwdError::RangeOverflow)?,
        )
        .ok_or(WwdError::OffsetOutOfBounds {
            offset: u32::try_from(at).unwrap_or(u32::MAX),
            need: PLANE_HEADER_SIZE,
            have: bytes.len(),
        })?;
    let header_size = word(h, 0x00);
    if header_size.as_usize() != PLANE_HEADER_SIZE {
        return Err(WwdError::BadPlaneHeaderSize {
            plane: index,
            size: header_size,
        });
    }
    let raw_width = signed(h, 0x60);
    let raw_height = signed(h, 0x64);
    let raw_tile_width = signed(h, 0x58);
    let raw_tile_height = signed(h, 0x5c);
    if raw_width <= 0 || raw_height <= 0 || raw_tile_width <= 0 || raw_tile_height <= 0 {
        return Err(WwdError::BadDimensions {
            plane: index,
            width: raw_width,
            height: raw_height,
        });
    }
    let tiles_wide = usize::try_from(raw_width).map_err(|_| WwdError::RangeOverflow)?;
    let tiles_high = usize::try_from(raw_height).map_err(|_| WwdError::RangeOverflow)?;
    let tile_count = tiles_wide
        .checked_mul(tiles_high)
        .ok_or(WwdError::RangeOverflow)?;
    let tile_bytes = tile_count.checked_mul(4).ok_or(WwdError::RangeOverflow)?;
    let tiles_offset = word(h, 0x84);
    let tiles = range(bytes, tiles_offset, tile_bytes)?;
    let image_sets_offset = word(h, 0x88);
    let names = range(bytes, image_sets_offset, 0)?;
    let image_set_count = word(h, 0x7c).as_usize();
    let plane = Plane {
        index,
        flags: word(h, 0x08),
        name: cstr(&h[0x10..0x50])?,
        pixel_width: signed(h, 0x50),
        pixel_height: signed(h, 0x54),
        tile_width: usize::try_from(raw_tile_width).map_err(|_| WwdError::RangeOverflow)?,
        tile_height: usize::try_from(raw_tile_height).map_err(|_| WwdError::RangeOverflow)?,
        tiles_wide,
        tiles_high,
        scroll_x: signed(h, 0x68),
        scroll_y: signed(h, 0x6c),
        movement_x_percent: signed(h, 0x70),
        movement_y_percent: signed(h, 0x74),
        fill_color: word(h, 0x78),
        z_coord: signed(h, 0x90),
        tiles,
        image_sets: ImageSets {
            bytes: names,
            plane: index,
            count: image_set_count,
            index: 0,
            at: 0,
        },
    };
    // Walking once here catches truncated name tables during `split`; callers
    // receive a fresh copy of the iterator from `Plane::image_sets`.
    for name in plane.image_sets() {
        name?;
    }
    Ok(plane)
}

#[derive(Debug, Clone, Copy)]
pub struct ImageSets<'a> {
    bytes: &'a [u8],
    plane: usize,
    count: usize,
    index: usize,
    at: usize,
}

impl<'a> Iterator for ImageSets<'a> {
    type Item = Result<&'a str, WwdError>;

    fn next(&mut self) -> Option<Self::Item> {
        if self.index == self.count {
            return None;
        }
        let wanted = self.index;
        self.index += 1;
        while self.at < self.bytes.len()
            && (self.bytes[self.at] < b'0' || self.bytes[self.at] >= 0x80)
        {
            self.at += 1;
        }
        let start = self.at;
        while self.at < self.bytes.len()
            && self.bytes[self.at] >= b'0'
            && self.bytes[self.at] < 0x80
        {
            self.at += 1;
        }
        if start == self.at {
            return Some(Err(WwdError::MissingImageSetName {
                plane: self.plane,
                index: wanted,
            }));
        }
        Some(str::from_utf8(&self.bytes[start..self.at]).map_err(|_| WwdError::InvalidText))
    }
}

pub struct Tiles<'a> {
    bytes: &'a [u8],
}

impl Iterator for Tiles<'_> {
    type Item = u32;

    fn next(&mut self) -> Option<Self::Item> {
        let word: [u8; 4] = self.bytes.get(..4)?.try_into().ok()?;
        self.bytes = &self.bytes[4..];
        Some(u32::from_le_bytes(word))
    }

    fn size_hint(&self) -> (usize, Option<usize>) {
        let count = self.bytes.len() / 4;
        (count, Some(count))
    }
}

impl ExactSizeIterator for Tiles<'_> {}

fn range(bytes: &[u8], offset: u32, len: usize) -> Result<&[u8], WwdError> {
    let start = offset.as_usize();
    let end = start.checked_add(len).ok_or(WwdError::RangeOverflow)?;
    if len == 0 {
        return bytes.get(start..).ok_or(WwdError::OffsetOutOfBounds {
            offset,
            need: 0,
            have: bytes.len(),
        });
    }
    bytes.get(start..end).ok_or(WwdError::OffsetOutOfBounds {
        offset,
        need: len,
        have: bytes.len(),
    })
}

fn word(bytes: &[u8], at: usize) -> u32 {
    u32::from_le_bytes(bytes[at..at + 4].try_into().unwrap_or([0; 4]))
}

fn signed(bytes: &[u8], at: usize) -> i32 {
    i32::from_le_bytes(bytes[at..at + 4].try_into().unwrap_or([0; 4]))
}

fn cstr(bytes: &[u8]) -> Result<&str, WwdError> {
    let end = bytes
        .iter()
        .position(|&byte| byte == 0)
        .unwrap_or(bytes.len());
    str::from_utf8(&bytes[..end]).map_err(|_| WwdError::InvalidText)
}
