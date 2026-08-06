use gruntz_codec::wwd::{self, HEADER_SIZE, PLANE_FLAG_MAIN, PLANE_HEADER_SIZE};

#[test]
fn parses_plane_names_and_tile_handles() {
    let mut image = vec![0u8; HEADER_SIZE + PLANE_HEADER_SIZE + 32];
    put(&mut image, 0x00, u32::try_from(HEADER_SIZE).unwrap());
    image[0x10..0x14].copy_from_slice(b"TEST");
    put(&mut image, 0x2dc, 1);
    put(&mut image, 0x2e0, u32::try_from(HEADER_SIZE).unwrap());
    let main_len = u32::try_from(image.len() - HEADER_SIZE).unwrap();
    put(&mut image, 0x2e8, main_len);
    let plane = HEADER_SIZE;
    put(&mut image, plane, u32::try_from(PLANE_HEADER_SIZE).unwrap());
    put(&mut image, plane + 0x08, PLANE_FLAG_MAIN);
    image[plane + 0x10..plane + 0x14].copy_from_slice(b"MAIN");
    put(&mut image, plane + 0x58, 2);
    put(&mut image, plane + 0x5c, 2);
    put(&mut image, plane + 0x60, 2);
    put(&mut image, plane + 0x64, 1);
    put(&mut image, plane + 0x7c, 2);
    let tiles = plane + PLANE_HEADER_SIZE;
    put(&mut image, plane + 0x84, u32::try_from(tiles).unwrap());
    put(&mut image, tiles, 0x0000_0003);
    put(&mut image, tiles + 4, 0x0001_0007);
    let names = tiles + 8;
    put(&mut image, plane + 0x88, u32::try_from(names).unwrap());
    image[names..names + 5].copy_from_slice(&[4, 0, 0, 0, b'A']);
    image[names + 5..names + 10].copy_from_slice(&[4, 0, 0, 0, b'B']);

    let world = wwd::split(&image).unwrap();
    let parsed = world.planes().next().unwrap().unwrap();
    assert!(parsed.is_main());
    assert_eq!(parsed.pixel_size().unwrap(), (4, 2));
    assert_eq!(parsed.tiles().collect::<Vec<_>>(), [3, 0x0001_0007]);
    assert_eq!(
        parsed.image_sets().collect::<Result<Vec<_>, _>>().unwrap(),
        ["A", "B"]
    );
}

fn put(bytes: &mut [u8], at: usize, value: u32) {
    bytes[at..at + 4].copy_from_slice(&value.to_le_bytes());
}
