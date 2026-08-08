//! Writer tests: build an archive in memory, read it back with the validated
//! reader, and check the bytes the retail loader actually looks at.
//!
//! Nothing here needs a copy of the game. The one thing that does — the retail
//! corpus — is exercised by `rezpack roundtrip <archive>`.

use gruntz_rez::write::{RezBuilder, ResourceSpec, WriteError, BANNER, BANNER_LINE};
use gruntz_rez::{FourCc, Rez, HEADER_SIZE};

fn pid<'a>(name: &'a str, data: &'a [u8]) -> ResourceSpec<'a> {
    ResourceSpec::new(name, FourCc::from_tag("PID"), data)
}

#[test]
fn header_matches_retails_byte_for_byte_where_it_is_fixed() {
    // The first 127 bytes of retail Gruntz.REZ, GRUNTZ.VRZ and GRUNTDEM.REZ —
    // all three are identical here.
    const RETAIL_BANNER: &[u8; 127] = b"\x0d\x0a\x52\x65\x7a\x4d\x67\x72\x20\x56\x65\x72\x73\x69\x6f\x6e\
\x20\x31\x20\x43\x6f\x70\x79\x72\x69\x67\x68\x74\x20\x28\x43\x29\
\x20\x31\x39\x39\x35\x20\x4d\x4f\x4e\x4f\x4c\x49\x54\x48\x20\x49\
\x4e\x43\x2e\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x0d\x0a\
\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\
\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\
\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\
\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x20\x0d\x0a\x1a";
    assert_eq!(&BANNER, RETAIL_BANNER);

    let mut b = RezBuilder::new();
    b.set_time(913_067_100); // retail's, 1998-12-07
    b.root().add(pid("A", b"x")).unwrap();
    let image = b.finish().unwrap();

    assert_eq!(&image[..127], &RETAIL_BANNER[..]);
    // The three bytes retail asserts (CRezMgr::Open @0x13b004/@0x13b016/@0x13b021).
    assert_eq!(image[0x00], b'\r');
    assert_eq!(image[0x3f], b'\n');
    assert_eq!(image[0x7e], 0x1a);
    // Fields at fixed offsets, first payload at 0xa8.
    assert_eq!(HEADER_SIZE, 0xa8);
    assert_eq!(u32::from_le_bytes(image[0x7f..0x83].try_into().unwrap()), 1);
    assert_eq!(
        u32::from_le_bytes(image[0x93..0x97].try_into().unwrap()),
        913_067_100
    );
    assert_eq!(image[0xa7], 1); // is_sorted
    assert_eq!(image[0xa8], b'x');
}

#[test]
fn small_archive_round_trips_in_process() {
    let mut b = RezBuilder::new();
    b.root()
        .add(ResourceSpec {
            name: "README",
            comment: "a comment",
            kind: FourCc::from_tag("TXT"),
            id: 7,
            time: 0x3000_0001,
            keys: &[11, 22, 33],
            data: b"hello",
        })
        .unwrap()
        .add(pid("SPRITE", b"\x01\x02\x03\x04"))
        .unwrap();
    let image = b.finish().unwrap();

    let rez = Rez::new(&image).unwrap();
    assert_eq!(rez.header.version, 1);
    assert_eq!(rez.validate().unwrap(), 2);
    rez.is_contiguous().unwrap();

    let all: Vec<_> = rez.resources().map(|r| r.unwrap()).collect();
    let readme = all.iter().find(|r| r.name == "README").unwrap();
    assert_eq!(readme.kind.to_string(), "TXT");
    assert_eq!(readme.comment, "a comment");
    assert_eq!(readme.id, 7);
    assert_eq!(readme.time, 0x3000_0001);
    assert_eq!(readme.num_keys, 3);
    assert_eq!(readme.keys().collect::<Vec<_>>(), vec![11, 22, 33]);
    assert_eq!(readme.data(rez.bytes()), b"hello");

    let sprite = all.iter().find(|r| r.name == "SPRITE").unwrap();
    assert_eq!(sprite.kind.to_string(), "PID");
    assert_eq!(sprite.data(rez.bytes()), b"\x01\x02\x03\x04");
    assert_eq!(sprite.comment, "");
    assert_eq!(sprite.num_keys, 0);
}

#[test]
fn nests_three_deep_like_retail() {
    // Retail's deepest path shape: AREA2\IMAGEZ\TREE2\FRAME001.
    let frames: [(&str, [u8; 3]); 3] = [
        ("FRAME001", [1, 1, 1]),
        ("FRAME002", [2, 2, 2]),
        ("FRAME003", [3, 3, 3]),
    ];
    let mut b = RezBuilder::new();
    for (name, bytes) in &frames {
        b.root()
            .dir_path("AREA2\\IMAGEZ\\TREE2")
            .unwrap()
            .add(pid(name, bytes))
            .unwrap();
    }
    b.root()
        .dir_path("AREA2/ANIZ")
        .unwrap()
        .add(ResourceSpec::new(
            "FORTSPLASH",
            FourCc::from_tag("ANI"),
            b"ani",
        ))
        .unwrap();
    let image = b.finish().unwrap();

    let rez = Rez::new(&image).unwrap();
    assert_eq!(rez.validate().unwrap(), 4);
    rez.is_contiguous().unwrap();

    let mut paths: Vec<String> = rez
        .resources()
        .map(|r| r.unwrap().path().to_string())
        .collect();
    paths.sort();
    assert_eq!(
        paths,
        vec![
            "AREA2\\ANIZ\\FORTSPLASH",
            "AREA2\\IMAGEZ\\TREE2\\FRAME001",
            "AREA2\\IMAGEZ\\TREE2\\FRAME002",
            "AREA2\\IMAGEZ\\TREE2\\FRAME003",
        ]
    );
    let mut dirs: Vec<String> = rez
        .directories()
        .map(|d| d.unwrap().path().to_string())
        .collect();
    dirs.sort();
    assert_eq!(
        dirs,
        vec!["AREA2", "AREA2\\ANIZ", "AREA2\\IMAGEZ", "AREA2\\IMAGEZ\\TREE2"]
    );
    // `/` and `\` are both accepted as separators and normalise to one tree.
    assert_eq!(dirs.iter().filter(|d| d.as_str() == "AREA2").count(), 1);
}

#[test]
fn an_empty_directory_survives_and_carries_its_time() {
    let mut b = RezBuilder::new();
    b.root().dir("EMPTY").unwrap().set_time(0x1234_5678);
    b.root()
        .dir("FULL")
        .unwrap()
        .add(pid("A", b"a"))
        .unwrap();
    let image = b.finish().unwrap();

    let rez = Rez::new(&image).unwrap();
    assert_eq!(rez.validate().unwrap(), 1);
    let dirs: Vec<_> = rez.directories().map(|d| d.unwrap()).collect();
    assert_eq!(dirs.len(), 2);
    let empty = dirs.iter().find(|d| d.name == "EMPTY").unwrap();
    assert_eq!(empty.size, 0);
    assert_eq!(empty.time, 0x1234_5678);
}

#[test]
fn every_directorys_payloads_are_contiguous_even_when_interleaved() {
    // Add to A, then B, then back to A: `place` groups by directory regardless
    // of insertion order, which is what makes is_sorted = 1 honest.
    let mut b = RezBuilder::new();
    b.root().dir("A").unwrap().add(pid("A1", b"aaaa")).unwrap();
    b.root().dir("B").unwrap().add(pid("B1", b"bb")).unwrap();
    b.root()
        .dir("A")
        .unwrap()
        .add(pid("A2", b"aaaaaaaa"))
        .unwrap();
    b.root().add(pid("TOP", b"t")).unwrap();
    let image = b.finish().unwrap();

    let rez = Rez::new(&image).unwrap();
    assert_eq!(rez.validate().unwrap(), 4);
    rez.is_contiguous().unwrap();
    assert_eq!(rez.header.is_sorted, 1);

    // A1 and A2 really are adjacent.
    let all: Vec<_> = rez.resources().map(|r| r.unwrap()).collect();
    let a1 = all.iter().find(|r| r.name == "A1").unwrap();
    let a2 = all.iter().find(|r| r.name == "A2").unwrap();
    assert_eq!(a1.pos + a1.size, a2.pos);
}

#[test]
fn largest_fields_are_max_strlen_plus_one_over_present_strings() {
    let mut b = RezBuilder::new();
    b.root()
        .dir("DIRECTORY_NAME_X") // 16
        .unwrap()
        .add(ResourceSpec {
            name: "RESOURCE_NAME_LONG", // 18
            comment: "cmt",             // 3
            kind: FourCc::from_tag("TXT"),
            id: 0,
            time: 0,
            keys: &[1, 2],
            data: b"d",
        })
        .unwrap()
        .add(pid("SHORT", b"d"))
        .unwrap();
    let rez_image = b.finish().unwrap();
    let rez = Rez::new(&rez_image).unwrap();
    assert_eq!(rez.header.largest_dir_name_size, 17);
    assert_eq!(rez.header.largest_rez_name_size, 19);
    assert_eq!(rez.header.largest_comment_size, 4);
    assert_eq!(rez.header.largest_key_ary, 2);
}

#[test]
fn an_all_empty_comment_archive_reports_largest_comment_zero_like_retail() {
    // Retail carries largest_comment_size == 0 while every comment is "", which
    // is what fixes the rule: an empty string does not participate.
    let mut b = RezBuilder::new();
    b.root().add(pid("A", b"a")).unwrap();
    let image = b.finish().unwrap();
    assert_eq!(Rez::new(&image).unwrap().header.largest_comment_size, 0);
}

#[test]
fn refuses_a_duplicate_name_and_type_but_allows_a_type_change() {
    let mut b = RezBuilder::new();
    let d = b.root().dir("D").unwrap();
    d.add(pid("A", b"1")).unwrap();
    // Same name, different type: retail keys a directory by type then name, so
    // this is not a collision.
    d.add(ResourceSpec::new("A", FourCc::from_tag("TXT"), b"2"))
        .unwrap();
    // Same name and type, differing only in case: it is.
    let err = d.add(pid("a", b"3")).unwrap_err();
    assert!(matches!(err, WriteError::Duplicate { .. }), "{err:?}");
    assert_eq!(Rez::new(&b.finish().unwrap()).unwrap().validate().unwrap(), 2);
}

#[test]
fn refuses_names_the_container_or_the_game_cannot_address() {
    let mut b = RezBuilder::new();
    for bad in ["", "HAS\\SEP", "HAS/SEP", "HAS\tTAB"] {
        assert!(
            matches!(b.root().add(pid(bad, b"x")), Err(WriteError::BadName(_))),
            "accepted {bad:?}"
        );
        assert!(
            matches!(b.root().dir(bad), Err(WriteError::BadName(_))),
            "accepted directory {bad:?}"
        );
    }
}

#[test]
fn a_custom_banner_still_parses_and_is_bounded() {
    let mut b = RezBuilder::new();
    b.set_banner("gruntz-rez", "clean-room").unwrap();
    b.root().add(pid("A", b"a")).unwrap();
    let image = b.finish().unwrap();
    assert_eq!(&image[2..12], b"gruntz-rez");
    assert_eq!(&image[0x40..0x4a], b"clean-room");
    assert_eq!(image[0x7e], 0x1a);
    let rez = Rez::new(&image).unwrap();
    assert_eq!(rez.validate().unwrap(), 1);
    assert!(rez.header.comment.starts_with("gruntz-rez"));

    let too_long = "x".repeat(BANNER_LINE + 1);
    assert!(matches!(
        RezBuilder::new().set_banner(&too_long, ""),
        Err(WriteError::BadBanner)
    ));
}

#[test]
fn fourcc_tag_round_trips() {
    for tag in ["PID", "ANI", "WAV", "WWD", "XMI", "PAL", "PCX", "TXT", "BAT"] {
        assert_eq!(FourCc::from_tag(tag).to_string(), tag);
    }
    // Retail's stored dword for PID, straight out of the archive.
    assert_eq!(FourCc::from_tag("PID").0, 0x0050_4944);
}

#[test]
fn an_empty_archive_is_still_a_valid_archive() {
    let image = RezBuilder::new().finish().unwrap();
    let rez = Rez::new(&image).unwrap();
    assert_eq!(rez.validate().unwrap(), 0);
    assert_eq!(rez.header.root_dir_size, 0);
    assert_eq!(rez.header.next_write_pos, 0xa8);
    rez.is_contiguous().unwrap();
}
