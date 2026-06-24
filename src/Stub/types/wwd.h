#ifndef FORMATS_WWD_H
#define FORMATS_WWD_H

#include <Ints.h>

/*
 * WWD level container + PID image on-disk formats.
 *
 * Gruntz levels are .WWD ("Wap World Data") files; sprites/images are .PID files
 * loaded by name through the RezMgr. This header pins the FIXED on-disk layouts
 * that the Gruntz asset loaders read:
 *
 *   - WwdHeader      0x5F4 (1524) — the level file header.
 *   - WwdPlaneHeader 0xA0  (160)  — one per plane, inside the (inflated) main block.
 *   - PidHeader      0x20  (32)   — the .PID image header (raw, NOT zlib).
 *
 * The per-object record (WwdObjectRecord, 0x11C/284) lives in wwd_object.h.
 *
 * PROVENANCE (the on-disk format is FIXED, so these offsets ARE the layout):
 *   - Gruntz ground truth: the loaders in GRUNTZ.EXE read these exact offsets:
 *       CGameLevel::LoadWwd        @0x15d280 — cmp firstDword,0x5f4; rep movs 0x17d
 *         dwords (1524 B) header copy; test flags[+8],0x2 (COMPRESS); numPlanes
 *         [+0x2dc]; planesOffset [+0x2e0]; mainBlockLength [+0x2e8]; checksum
 *         [+0x2ec]; loops numPlanes with plane stride 0xA0 (160).
 *       WwdFile::InflateMainBlock  @0x160790 — planesOffset[+0x2e0] <= 0x5f4;
 *         mainBlockLength [+0x2e8]; uncompress(out+planesOffset, in+planesOffset).
 *       WwdFile::IsValidWwd        @0x160530 — read 1524 B; require read==0x5f4 &&
 *         firstDword<=0x5f4.
 *   - Cross-validated vs OpenClaw libwap WwdFile.cpp / libwap.h and libwap32
 *     misc/wwd_structs.h + src/wwd_read.cpp (clean-room WAP32 — algorithm/layout
 *     reference only, NOT a byte source), and against
 *     refs/libwap32/tests/wwd_files/RETAIL*.WWD hexdumps.
 *
 * CONVENTION: all multi-byte fields are little-endian. The first u32 of each
 * structure equals its own byte size (the "signature": 1524 for the file header,
 * 160 for a plane header, 32 for a tile-description header), which the loaders
 * validate.
 */

/* MSVC 5.0 (1997) predates <stdint.h> (C99; MSVC added it in VS2010). This is a
 * comprehension header, so just define the two fixed-width names it uses in terms
 * of the native i386 types (int == long == 4 bytes under this target). */
typedef u32 uint32_t; /* 32-bit unsigned */
typedef i32 int32_t;  /* 32-bit signed   */

/* -------------------------------------------------------------------------- */
/* WWD level header — FULLY PINNED, size 1524 (0x5F4)                          */
/* -------------------------------------------------------------------------- */
/*
 * The first u32 of the file == 1524 (== sizeof(WwdHeader); the "signature").
 * At file offset planesOffset (== 1524) the bytes 78 9C ... begin a zlib deflate
 * stream when flags bit1 (COMPRESS) is set; decompressed size == mainBlockLength.
 *
 * NOTE: WwdHeader has GRADUATED into src/Wwd/WwdFile.h (the authoritative,
 * matched 0x5F4 layout). It is intentionally NOT re-declared here to avoid a
 * second source of truth; the full field roster (levelName/author/rezFile/
 * imageSet1..4/prefix1..4/numPlanes@0x2DC/planesOffset@0x2E0/...) is recorded in
 * git history of this file. This header now carries only the structures that have
 * not graduated: WwdPlaneHeader, PidHeader, and the flag enums.
 */
enum WwdFlags {
    WWD_FLAG_USE_Z_COORDS = 1 << 0, /* bit0 */
    WWD_FLAG_COMPRESS = 1 << 1      /* bit1 — main block is zlib-deflated */
};

/* -------------------------------------------------------------------------- */
/* WWD plane header — size 160 (0xA0)                                          */
/* -------------------------------------------------------------------------- */
/*
 * Inside the (inflated) main block. The loader walks planes with stride 0xA0
 * (`add esi,0xa0` in CGameLevel::LoadWwd). First u32 == 160 (the signature).
 * tile[]/imageset-names/object[] follow at the per-plane offsets below.
 */
struct WwdPlaneHeader {
    uint32_t signature;       /* @0x00 (0)   == 160 (plane header size)        */
    uint32_t null0;           /* @0x04 (4)                                     */
    uint32_t flags;           /* @0x08 (8)   MAIN/NO_DRAW/X_WRAP/Y_WRAP/AUTO   */
    uint32_t null1;           /* @0x0C (12)                                    */
    char name[64];            /* @0x10 (16)                                    */
    uint32_t pixelWidth;      /* @0x50 (80)                                    */
    uint32_t pixelHeight;     /* @0x54 (84)                                    */
    uint32_t tilePixelWidth;  /* @0x58 (88)                                    */
    uint32_t tilePixelHeight; /* @0x5C (92)                                    */
    uint32_t tilesOnAxisX;    /* @0x60 (96)                                    */
    uint32_t tilesOnAxisY;    /* @0x64 (100)                                   */
    uint32_t null2;           /* @0x68 (104)                                   */
    uint32_t null3;           /* @0x6C (108)                                   */
    int32_t movementPercentX; /* @0x70 (112)                                   */
    int32_t movementPercentY; /* @0x74 (116)                                   */
    uint32_t fillColor;       /* @0x78 (120)                                   */
    uint32_t imageSetsCount;  /* @0x7C (124)                                   */
    uint32_t objectsCount;    /* @0x80 (128)                                   */
    uint32_t tilesOffset;     /* @0x84 (132)  offset of tile[] within block    */
    uint32_t imageSetsOffset; /* @0x88 (136)  offset of imageset name list     */
    uint32_t objectsOffset;   /* @0x8C (140)  offset of object[] within block  */
    int32_t coordZ;           /* @0x90 (144)                                   */
    uint32_t null4;           /* @0x94 (148)                                   */
    uint32_t null5;           /* @0x98 (152)                                   */
    uint32_t null6;           /* @0x9C (156)                                   */
}; /* sizeof == 0xA0 (160) — PINNED (matches loader plane stride add esi,0xa0) */

/*
 * Tile map: int32_t tile[tilesOnAxisX * tilesOnAxisY] at tilesOffset.
 * Image sets: imageSetsCount length-prefixed strings at imageSetsOffset.
 * Objects:    objectsCount WwdObjectRecord (wwd_object.h) at objectsOffset.
 */

/* -------------------------------------------------------------------------- */
/* PID image header — size 32 (0x20)                                          */
/* -------------------------------------------------------------------------- */
/*
 * Loaded by name through the RezMgr; the entry payload is a raw PID (NOT zlib).
 * Header is 8 u32; pixel data starts at offset 0x20. Pixels are 8bpp palette
 * indices, optionally RLE (flag bit5) and/or with an embedded 768-byte palette
 * (flag bit7) appended at end-of-file.
 */
enum WwdPidFlags {
    WAP_PID_FLAG_TRANSPARENCY = 1 << 0,
    WAP_PID_FLAG_VIDEO_MEMORY = 1 << 1,  /* "VID" */
    WAP_PID_FLAG_SYSTEM_MEMORY = 1 << 2, /* "SYS" */
    WAP_PID_FLAG_MIRROR = 1 << 3,
    WAP_PID_FLAG_INVERT = 1 << 4,
    WAP_PID_FLAG_COMPRESSION = 1 << 5, /* "RLE" — RLE-compressed pixels */
    WAP_PID_FLAG_LIGHTS = 1 << 6,
    WAP_PID_FLAG_EMBEDDED_PALETTE = 1 << 7 /* 768-byte palette at EOF       */
};

struct PidHeader {
    uint32_t fileDesc; /* @0x00 (0)  id / file descriptor                     */
    uint32_t flags;    /* @0x04 (4)  WwdPidFlags                              */
    uint32_t width;    /* @0x08 (8)                                           */
    uint32_t height;   /* @0x0C (12)                                          */
    int32_t offsetX;   /* @0x10 (16) draw anchor X                            */
    int32_t offsetY;   /* @0x14 (20) draw anchor Y                            */
    uint32_t unk0;     /* @0x18 (24)                                          */
    uint32_t unk1;     /* @0x1C (28)                                          */
    /* @0x20 (32): RLE/uncompressed 8bpp pixel stream (palette indices).       */
    /* if EMBEDDED_PALETTE: 768 bytes (256 * {r,g,b}) at (fileSize-768).       */
}; /* sizeof == 0x20 (32) — PINNED */

#endif /* FORMATS_WWD_H */
