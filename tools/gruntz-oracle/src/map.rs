//! **WWD → PNG** — render a level exactly the way retail composites its tile
//! planes, and report every reference it could not resolve.
//!
//! One PNG per plane. The main plane goes to the caller's path; the others to
//! a sibling `<stem>-planes/NN-<name>.png`. Nothing here reads `src/` or runs
//! the game — it is an independent second opinion on the format, so that a
//! wrong idea about the layout shows up as a corrupt image or a missing-key
//! row rather than as silence.
//!
//! The field map this walks is verified in `docs/formats/wwd-v1.md`; that
//! document is the evidence, this module is one consumer of it.
//!
//! ## Pipeline, and what each step is imitating
//!
//! | Step | Here | Retail |
//! |---|---|---|
//! | inflate the main block | [`expand`] | `WwdFile_InflateMainBlock` @0x160790 |
//! | walk the plane headers | `wwd::split` | `CGameLevel::LoadWwd` @0x15d280 |
//! | read one plane | [`render_plane`] | `CDDrawWorkerHost::Read` @0x161640 |
//! | resolve an image-set name | [`load_frame_set`] | `CDDrawWorkerRegistry::m_10map.Lookup` |
//! | build the registry keys | [`installed_key`] | `CDDrawWorkerRegistry::InstallTree` @0x154f80 |
//! | pick the three registry roots | [`registry_roots`] | `CPlay::LoadActionTileSprites` @0xdb600, `LoadLevelImages` @0xdb7e0, `LoadGameImages` @0xdb8a0 |
//! | frame number from a resource name | [`first_number`] | `CDDrawWorker::BuildFramesFromSymTab` @0x1521f0 — skip to the first digit, then `atoi` |
//! | draw one cell | the `plane.tiles()` loop | `CDDrawWorkerHost::Draw` @0x162010 |
//! | complain about a bad handle | [`record_missing`] | `CDDrawWorkerHost::ValidateTiles` @0x163510 |
//!
//! ## Addressing
//!
//! [`expand`] rebuilds `header || inflated main block` and hands that whole
//! image to `wwd::split`, because **every offset in a WWD is absolute from
//! byte 0 of the image, header included** — retail relies on the same thing,
//! `memcpy`ing the header ahead of the inflate output at 0x160790. Dropping
//! the header to save 1524 bytes would shift every plane, tile and object
//! offset.
//!
//! ## Tile handles
//!
//! A cell is a `u32` split into two halves, and retail's own diagnostics name
//! them: `ValidateTiles` @0x163510 formats *"Bad map image set value (%i)"*
//! for `handle >> 16` and *"Bad map tile value (%i)"* for `handle & 0xffff`.
//! So the high word selects one of the plane's image sets and the low word
//! selects a frame inside it. Two handles are reserved, both proven from
//! `Draw`: `wwd::TILE_CLEAR` (`0xffffffff`) draws nothing, and
//! `wwd::TILE_FILL` (`0xeeeeeeee`) colour-fills the cell — retail issues
//! `BltEx(..., 0x1000400, &m_bltFx)` with `m_bltFx.dwFillColor` taken from the
//! plane header's `fill_color`, which [`rgba_for_fill`] imitates by treating
//! that value as a palette index.
//!
//! In practice neither branch fires much: across all 63 shipped WWDs the high
//! word is 0 in every one of 261 129 cells, and `TILE_FILL` never appears at
//! all. Both paths exist for correctness on a hand-authored level, not because
//! retail data needs them.
//!
//! ## Image-set registry keys
//!
//! This is the only genuinely non-obvious part. A plane names its image sets
//! with keys like `ACTION`, `LEVEL_WATER`, `GAME_CURSORZ` — but nothing in the
//! WWD says how those map onto archive directories. Retail builds them at load
//! time: `CDDrawWorkerRegistry::InstallTree` @0x154f80 walks a resource
//! subtree and joins each directory level onto a prefix with `"_"`
//! (`sprintf(buf, "%s%s%s", sub, prefix, e->m_name)`), and `CPlay` installs
//! exactly three trees:
//!
//! ```text
//! <level bank>\TILEZ    prefix ""        ->  ROCKZ_EDGE      CPlay::LoadActionTileSprites @0xdb600
//! <level bank>\IMAGEZ   prefix "LEVEL"   ->  LEVEL_WATER     CPlay::LoadLevelImages       @0xdb7e0
//! <game bank>\IMAGEZ    prefix "GAME"    ->  GAME_CURSORZ    CPlay::LoadGameImages        @0xdb8a0
//! ```
//!
//! [`installed_key`] is that join run backwards: given a resource's directory
//! path, produce the key retail would have filed it under. The header *also*
//! carries the same three `(directory, prefix)` pairs at 0x1d0/0x374/0x3f4 and
//! 0x574/0x594, so [`registry_roots`] prefers those and falls back to the
//! hardcoded triple — but retail never reads them (`docs/formats/wwd-v1.md`
//! marks them proven-unread), so the fallback is the authoritative path and
//! the header values are a cross-check that happens to agree on all 63 files.
//!
//! ## Missing references are data, not errors
//!
//! An unresolved image set or absent frame does not abort the render; it is
//! counted in [`MapReport`] and emitted as a TSV row (level, plane, image set,
//! frame, cell count, reason). A level that renders with zero rows is one
//! whose entire tile-reference graph resolved, which is the actual assertion
//! this module makes about the format.
//!
//! ## Not modelled
//!
//! Planes are rendered independently, so `z_coord` ordering, the parallax
//! `movement_x/y_percent`, `scroll_x/y` and the wrap flags — all of which
//! `CDDrawWorkerHost` applies at runtime — have no effect on the output. Plane
//! objects (`offset_objects`) are not drawn at all: this renders the tile
//! geometry, not a frame of the game.

use std::collections::BTreeMap;
use std::io::Read;
use std::path::{Path, PathBuf};

use flate2::read::ZlibDecoder;
use gruntz_codec::{pcx, pid, rid, wwd};
use gruntz_rez::{Resource, Rez};

#[derive(Debug)]
struct TileImage {
    width: usize,
    height: usize,
    rgba: Vec<u8>,
    palette: Option<[u8; pid::PALETTE_SIZE]>,
}

type FrameSet = BTreeMap<u16, TileImage>;

#[derive(Debug)]
struct RegistryRoot {
    dirs: Vec<String>,
    prefix: String,
}

#[derive(Debug, Clone, PartialEq, Eq, PartialOrd, Ord)]
struct MissingKey {
    plane: usize,
    image_set: String,
    frame: Option<u16>,
    reason: String,
}

#[derive(Debug, Default)]
pub struct MapReport {
    pub planes: usize,
    pub missing_references: usize,
    missing: BTreeMap<MissingKey, usize>,
}

impl MapReport {
    pub fn write_manifest(
        &self,
        level_path: &str,
        out: &Path,
    ) -> Result<(), Box<dyn std::error::Error>> {
        if self.missing.is_empty() {
            return Ok(());
        }
        if let Some(parent) = out.parent() {
            std::fs::create_dir_all(parent)?;
        }
        let mut text = String::from("level\tplane\timage_set\tframe\tcells\treason\n");
        for (key, cells) in &self.missing {
            let frame = key
                .frame
                .map_or_else(String::new, |value| value.to_string());
            text.push_str(&format!(
                "{level_path}\t{}\t{}\t{frame}\t{cells}\t{}\n",
                key.plane, key.image_set, key.reason
            ));
        }
        std::fs::write(out, text)?;
        Ok(())
    }

    pub fn append_manifest_rows(&self, level_path: &str, text: &mut String) {
        for (key, cells) in &self.missing {
            let frame = key
                .frame
                .map_or_else(String::new, |value| value.to_string());
            text.push_str(&format!(
                "{level_path}\t{}\t{}\t{frame}\t{cells}\t{}\n",
                key.plane, key.image_set, key.reason
            ));
        }
    }
}

pub fn render_resource(
    rez: &Rez,
    resource: &Resource,
    main_out: &Path,
) -> Result<MapReport, Box<dyn std::error::Error>> {
    if resource.kind.to_string() != "WWD" {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidInput,
            format!("{} is {}, not WWD", resource.path(), resource.kind),
        )
        .into());
    }
    let image = expand(resource.data(rez.bytes()))?;
    let world = wwd::split(&image)?;
    let registry_roots = registry_roots(world.header, resource)?;
    let planes = world.planes().collect::<Result<Vec<_>, _>>()?;
    let main_count = planes.iter().filter(|plane| plane.is_main()).count();
    if main_count != 1 {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            format!("WWD has {main_count} main planes; expected exactly one"),
        )
        .into());
    }
    let plane_root = plane_output_root(main_out);
    let mut report = MapReport {
        planes: planes.len(),
        ..MapReport::default()
    };
    for plane in planes {
        let destination = if plane.is_main() {
            main_out.to_path_buf()
        } else {
            let name = safe_component(plane.name);
            plane_root.join(format!("{:02}-{name}.png", plane.index))
        };
        render_plane(rez, &registry_roots, plane, &destination, &mut report)?;
    }
    report.missing_references = report.missing.values().sum();
    Ok(report)
}

fn expand(resource: &[u8]) -> Result<Vec<u8>, Box<dyn std::error::Error>> {
    let header = wwd::header(resource)?;
    if !header.compressed() {
        return Ok(resource.to_vec());
    }
    let compressed = resource.get(wwd::HEADER_SIZE..).ok_or_else(|| {
        std::io::Error::new(
            std::io::ErrorKind::UnexpectedEof,
            "WWD has no compressed main block",
        )
    })?;
    let mut main = Vec::with_capacity(
        usize::try_from(header.main_block_length)
            .map_err(|_| std::io::Error::other("WWD main block is too large"))?,
    );
    ZlibDecoder::new(compressed).read_to_end(&mut main)?;
    if main.len()
        != usize::try_from(header.main_block_length)
            .map_err(|_| std::io::Error::other("WWD main block is too large"))?
    {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            format!(
                "WWD inflated to {} bytes; header declares {}",
                main.len(),
                header.main_block_length
            ),
        )
        .into());
    }
    let mut image = Vec::with_capacity(wwd::HEADER_SIZE + main.len());
    image.extend_from_slice(&resource[..wwd::HEADER_SIZE]);
    image.extend_from_slice(&main);
    Ok(image)
}

fn render_plane(
    rez: &Rez,
    registry_roots: &[RegistryRoot],
    plane: wwd::Plane<'_>,
    out: &Path,
    report: &mut MapReport,
) -> Result<(), Box<dyn std::error::Error>> {
    let names = plane.image_sets().collect::<Result<Vec<_>, _>>()?;
    let mut sets: Vec<Result<FrameSet, String>> = Vec::with_capacity(names.len());
    for name in &names {
        sets.push(load_frame_set(rez, registry_roots, name));
    }
    let (width, height) = plane.pixel_size()?;
    let pixel_bytes = width
        .checked_mul(height)
        .and_then(|pixels| pixels.checked_mul(4))
        .ok_or_else(|| std::io::Error::other("rendered plane is too large"))?;
    let mut canvas = vec![0u8; pixel_bytes];
    let fallback_palette = sets
        .iter()
        .filter_map(|set| set.as_ref().ok())
        .flat_map(BTreeMap::values)
        .find_map(|image| image.palette.as_ref());
    let fill = rgba_for_fill(plane.fill_color, fallback_palette);
    for (cell, handle) in plane.tiles().enumerate() {
        let column = cell % plane.tiles_wide;
        let row = cell / plane.tiles_wide;
        let x = column * plane.tile_width;
        let y = row * plane.tile_height;
        if handle == wwd::TILE_CLEAR {
            continue;
        }
        if handle == wwd::TILE_FILL {
            fill_rect(
                &mut canvas,
                width,
                x,
                y,
                plane.tile_width,
                plane.tile_height,
                fill,
            );
            continue;
        }
        let set_index = usize::try_from(handle >> 16)
            .map_err(|_| std::io::Error::other("tile image-set index does not fit usize"))?;
        let frame_index = u16::try_from(handle & 0xffff)
            .map_err(|_| std::io::Error::other("tile frame index does not fit u16"))?;
        let Some(name) = names.get(set_index) else {
            record_missing(
                report,
                plane.index,
                format!("#{set_index}"),
                Some(frame_index),
                "image-set index is outside plane table".to_string(),
            );
            continue;
        };
        let Some(set) = sets.get(set_index) else {
            continue;
        };
        let set = match set {
            Ok(set) => set,
            Err(reason) => {
                record_missing(
                    report,
                    plane.index,
                    (*name).to_string(),
                    None,
                    reason.clone(),
                );
                continue;
            }
        };
        let Some(tile) = set.get(&frame_index) else {
            record_missing(
                report,
                plane.index,
                (*name).to_string(),
                Some(frame_index),
                "frame is absent".to_string(),
            );
            continue;
        };
        blit(
            &mut canvas,
            width,
            height,
            x,
            y,
            plane.tile_width,
            plane.tile_height,
            tile,
        );
    }
    write_png(out, width, height, &canvas)?;
    eprintln!(
        "[wwd] plane {} {:?}: {}x{} -> {}",
        plane.index,
        plane.name,
        width,
        height,
        out.display()
    );
    Ok(())
}

fn load_frame_set(
    rez: &Rez,
    registry_roots: &[RegistryRoot],
    key: &str,
) -> Result<FrameSet, String> {
    let mut frames = FrameSet::new();
    let mut found_directory = false;
    for resource in rez.resources().flatten() {
        let Some(installed) = installed_key(resource.dirs.as_slice(), registry_roots) else {
            continue;
        };
        if !installed.eq_ignore_ascii_case(key) {
            continue;
        }
        found_directory = true;
        let kind = resource.kind.to_string();
        if kind != "PID" && kind != "RID" && kind != "PCX" {
            continue;
        }
        let Some(index) = first_number(resource.name) else {
            continue;
        };
        let index = u16::try_from(index)
            .map_err(|_| format!("{} frame number does not fit u16", resource.path()))?;
        let image = decode_image(rez, &resource).map_err(|error| error.to_string())?;
        if frames.insert(index, image).is_some() {
            return Err(format!("duplicate frame {index}"));
        }
    }
    if frames.is_empty() {
        if found_directory {
            Err("image-set directory has no supported numbered images".to_string())
        } else {
            Err("image-set registry key is unresolved".to_string())
        }
    } else {
        Ok(frames)
    }
}

fn installed_key(dirs: &[&str], roots: &[RegistryRoot]) -> Option<String> {
    for root in roots {
        if dirs.len() <= root.dirs.len()
            || !dirs
                .iter()
                .zip(&root.dirs)
                .all(|(actual, expected)| actual.eq_ignore_ascii_case(expected))
        {
            continue;
        }
        let relative = dirs[root.dirs.len()..].join("_");
        if root.prefix.is_empty() {
            return Some(relative);
        }
        return Some(format!("{}_{}", root.prefix, relative));
    }
    None
}

fn registry_roots(
    header: wwd::Header<'_>,
    resource: &Resource,
) -> Result<Vec<RegistryRoot>, Box<dyn std::error::Error>> {
    let mut roots = Vec::new();
    push_registry_root(&mut roots, header.tile_directory, "");
    for (directory, prefix) in header
        .image_directories
        .into_iter()
        .zip(header.image_prefixes)
    {
        push_registry_root(&mut roots, directory, prefix);
    }
    if roots.is_empty() {
        let level_root = resource.dirs.as_slice().first().copied().ok_or_else(|| {
            std::io::Error::new(std::io::ErrorKind::InvalidData, "WWD has no resource root")
        })?;
        push_registry_root(&mut roots, &format!("{level_root}\\TILEZ"), "");
        push_registry_root(&mut roots, &format!("{level_root}\\IMAGEZ"), "LEVEL");
        push_registry_root(&mut roots, "GAME\\IMAGEZ", "GAME");
    }
    Ok(roots)
}

fn push_registry_root(roots: &mut Vec<RegistryRoot>, path: &str, prefix: &str) {
    let dirs: Vec<_> = path
        .split(['\\', '/'])
        .filter(|component| !component.is_empty() && !component.contains(':'))
        .map(str::to_string)
        .collect();
    if dirs.is_empty() {
        return;
    }
    roots.push(RegistryRoot {
        dirs,
        prefix: prefix.to_string(),
    });
}

fn first_number(name: &str) -> Option<u32> {
    let start = name
        .as_bytes()
        .iter()
        .position(|byte| byte.is_ascii_digit())?;
    let end = name.as_bytes()[start..]
        .iter()
        .position(|byte| !byte.is_ascii_digit())
        .map_or(name.len(), |relative| start + relative);
    name[start..end].parse().ok()
}

fn decode_image(rez: &Rez, resource: &Resource) -> Result<TileImage, Box<dyn std::error::Error>> {
    let data = resource.data(rez.bytes());
    match resource.kind.to_string().as_str() {
        "PID" => {
            let image = pid::split(data)?;
            let dims = image.header.dims()?;
            let mut pixels = vec![0u8; dims.pixel_len()];
            image.decode_into(&mut pixels, pid::RowOverrun::Carry)?;
            Ok(indexed_image(
                dims.width(),
                dims.height(),
                &pixels,
                image.palette,
                transparent_index(image.header, image.palette),
            ))
        }
        "RID" => {
            let image = rid::split(data)?;
            Ok(indexed_image(
                image.dims.width(),
                image.dims.height(),
                image.pixels,
                None,
                None,
            ))
        }
        "PCX" => {
            let image = pcx::split(data)?;
            let mut pixels = vec![0u8; image.pixel_len()];
            let mut scratch = vec![0u8; image.scratch_len()];
            image.decode_into(&mut pixels, &mut scratch)?;
            let width = image.dims.width() / image.planes;
            if image.planes == 1 {
                Ok(indexed_image(
                    width,
                    image.dims.height(),
                    &pixels,
                    image.palette,
                    None,
                ))
            } else {
                let mut rgba = Vec::with_capacity(width * image.dims.height() * 4);
                for bgr in pixels.chunks_exact(3) {
                    rgba.extend_from_slice(&[bgr[2], bgr[1], bgr[0], 0xff]);
                }
                Ok(TileImage {
                    width,
                    height: image.dims.height(),
                    rgba,
                    palette: None,
                })
            }
        }
        kind => Err(std::io::Error::new(
            std::io::ErrorKind::InvalidInput,
            format!("unsupported tile image type {kind}"),
        )
        .into()),
    }
}

fn transparent_index(header: pid::PidHeader, palette: Option<&[u8]>) -> Option<u8> {
    if header.flags & pid::flags::TRANSPARENCY == 0 {
        return None;
    }
    palette
        .and_then(|entries| {
            entries
                .chunks_exact(3)
                .position(|rgb| rgb == [0xff, 0x00, 0x84])
        })
        .and_then(|index| u8::try_from(index).ok())
        .or_else(|| Some(header.fill_byte()))
}

fn indexed_image(
    width: usize,
    height: usize,
    pixels: &[u8],
    palette: Option<&[u8]>,
    transparent: Option<u8>,
) -> TileImage {
    let owned_palette = palette.and_then(|source| {
        if source.len() != pid::PALETTE_SIZE {
            return None;
        }
        let mut entries = [0u8; pid::PALETTE_SIZE];
        entries.copy_from_slice(source);
        Some(entries)
    });
    let mut rgba = Vec::with_capacity(width * height * 4);
    for &pixel in pixels {
        let index = usize::from(pixel);
        let grey = [pixel, pixel, pixel];
        let rgb = palette
            .and_then(|entries| entries.get(index * 3..index * 3 + 3))
            .unwrap_or(&grey);
        rgba.extend_from_slice(&[
            rgb[0],
            rgb[1],
            rgb[2],
            if transparent == Some(pixel) { 0 } else { 0xff },
        ]);
    }
    TileImage {
        width,
        height,
        rgba,
        palette: owned_palette,
    }
}

fn rgba_for_fill(fill: u32, palette: Option<&[u8; pid::PALETTE_SIZE]>) -> [u8; 4] {
    let index = u8::try_from(fill & 0xff).unwrap_or(0);
    if let Some(rgb) = palette.and_then(|entries| {
        let at = usize::from(index) * 3;
        entries.get(at..at + 3)
    }) {
        return [rgb[0], rgb[1], rgb[2], 0xff];
    }
    [index, index, index, 0xff]
}

fn fill_rect(
    canvas: &mut [u8],
    canvas_width: usize,
    x: usize,
    y: usize,
    width: usize,
    height: usize,
    rgba: [u8; 4],
) {
    for row in y..y + height {
        for column in x..x + width {
            let at = (row * canvas_width + column) * 4;
            canvas[at..at + 4].copy_from_slice(&rgba);
        }
    }
}

fn blit(
    canvas: &mut [u8],
    canvas_width: usize,
    canvas_height: usize,
    x: usize,
    y: usize,
    cell_width: usize,
    cell_height: usize,
    tile: &TileImage,
) {
    let copy_width = cell_width
        .min(tile.width)
        .min(canvas_width.saturating_sub(x));
    let copy_height = cell_height
        .min(tile.height)
        .min(canvas_height.saturating_sub(y));
    for row in 0..copy_height {
        for column in 0..copy_width {
            let source = (row * tile.width + column) * 4;
            if tile.rgba[source + 3] == 0 {
                continue;
            }
            let destination = ((y + row) * canvas_width + x + column) * 4;
            canvas[destination..destination + 4].copy_from_slice(&tile.rgba[source..source + 4]);
        }
    }
}

fn write_png(
    out: &Path,
    width: usize,
    height: usize,
    rgba: &[u8],
) -> Result<(), Box<dyn std::error::Error>> {
    if let Some(parent) = out.parent() {
        std::fs::create_dir_all(parent)?;
    }
    let file = std::fs::File::create(out)?;
    let mut encoder = png::Encoder::new(
        std::io::BufWriter::new(file),
        u32::try_from(width).map_err(|_| std::io::Error::other("PNG width exceeds u32"))?,
        u32::try_from(height).map_err(|_| std::io::Error::other("PNG height exceeds u32"))?,
    );
    encoder.set_color(png::ColorType::Rgba);
    encoder.set_depth(png::BitDepth::Eight);
    encoder.set_compression(png::Compression::Default);
    encoder.set_adaptive_filter(png::AdaptiveFilterType::Adaptive);
    encoder.write_header()?.write_image_data(rgba)?;
    Ok(())
}

fn record_missing(
    report: &mut MapReport,
    plane: usize,
    image_set: String,
    frame: Option<u16>,
    reason: String,
) {
    *report
        .missing
        .entry(MissingKey {
            plane,
            image_set,
            frame,
            reason,
        })
        .or_default() += 1;
}

fn plane_output_root(main: &Path) -> PathBuf {
    let parent = main.parent().unwrap_or_else(|| Path::new("."));
    let stem = main
        .file_stem()
        .and_then(|value| value.to_str())
        .unwrap_or("map");
    parent.join(format!("{stem}-planes"))
}

fn safe_component(name: &str) -> String {
    let cleaned: String = name
        .chars()
        .map(|character| {
            if character.is_ascii_alphanumeric() || character == '-' || character == '_' {
                character
            } else {
                '_'
            }
        })
        .collect();
    if cleaned.is_empty() {
        "plane".to_string()
    } else {
        cleaned
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn derives_the_same_registry_keys_as_install_tree() {
        let roots = [
            RegistryRoot {
                dirs: vec!["AREA1".to_string(), "TILEZ".to_string()],
                prefix: String::new(),
            },
            RegistryRoot {
                dirs: vec!["AREA1".to_string(), "IMAGEZ".to_string()],
                prefix: "LEVEL".to_string(),
            },
            RegistryRoot {
                dirs: vec!["GAME".to_string(), "IMAGEZ".to_string()],
                prefix: "GAME".to_string(),
            },
        ];
        assert_eq!(
            installed_key(&["AREA1", "TILEZ", "ROCKZ", "EDGE"], &roots).as_deref(),
            Some("ROCKZ_EDGE")
        );
        assert_eq!(
            installed_key(&["AREA1", "IMAGEZ", "WATER"], &roots).as_deref(),
            Some("LEVEL_WATER")
        );
        assert_eq!(
            installed_key(&["GAME", "IMAGEZ", "CURSORZ"], &roots).as_deref(),
            Some("GAME_CURSORZ")
        );
    }

    #[test]
    fn uses_the_first_digit_run_like_retail() {
        assert_eq!(first_number("FRAME001"), Some(1));
        assert_eq!(first_number("A12B34"), Some(12));
        assert_eq!(first_number("FRAME"), None);
    }
}
