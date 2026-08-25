#ifndef SRC_WWD_WWDFILE_H
#define SRC_WWD_WWDFILE_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>

#if GZ_STRICT_ENUMS && !defined(GRUNTZ_WWD_WWDPLANEFLAGS_H)
GZ_ENUM_FORWARD_SPLIT(WwdPlaneFlags, u32);
#endif

class CDDrawWorker;

typedef u8 Bytef;
typedef u32 uLong;
typedef u32 uLongf;

GZ_ENUM_CONST_BEGIN(WwdFormatConstants)
    WWD_PLANE_HEADER_SIZE = 0xa0
GZ_ENUM_CONST_END(WwdFormatConstants)

// The 1524-byte WWD file header, verified field by field against retail's four
// readers and all 63 shipped levels - docs/formats/wwd-v1.md. The six string
// slots each hold a NUL-terminated name zero-padded to the end of its slot in
// every shipped file, which is what fixes their boundaries independently of the
// disassembly; the author/created splits are additionally forced by
// FillLevelInfoDialog and CustomWorldInfoDlgProc, which set dialog controls
// 0x428 and 0x429 from them.
struct WwdHeader {
    // NOT a signature: LoadWwd @0x15d29b rejects only values GREATER than
    // sizeof(WwdHeader) (`cmp eax,0x5f4; jbe`), then WwdFile_InflateMainBlock
    // uses it as both the memcpy length and the offset of the compressed bytes.
    u32 headerSize;
    u32 reserved04;
    u32 flags;
    u32 reserved0c;
    char levelName[0x40];
    char author[0x40];
    char created[0x40];
    // Proven unread: "C:\PROJ\GRUNTZ\GRUNTZ.REZ" in all 63.
    char rezFile[0x100];
    // Proven unread - CPlay hardcodes the three image-set roots instead; the
    // editor recorded what it used and the game re-derives the same thing.
    char tileDirectory[0x80];
    // Proven unread; the empty string in all 63.
    char palette[0x80];
    i32 startX;
    i32 startY;
    u32 reserved2d8;
    u32 numPlanes;
    u32 planesOffset;
    u32 tileDescriptionsOffset;
    u32 mainBlockLength;
    // Stored and never verified: retail returns it from
    // CGruntzMgr::ResolveLevelChecksum as the level's multiplayer identity token.
    u32 checksum;
    u32 reserved2f0;
    // The remaining four slots are all proven unread, like tileDirectory.
    char launchApp[0x80];
    char imageDirectory[4][0x80];
    char imagePrefix[4][0x20];
};

struct WwdPlaneHeader {
    u32 headerSize;
    u32 reserved04;
    GZ_ENUM_STORAGE(WwdPlaneFlags, u32) flags;

    u32 reserved0c;
    char name[0x50 - 0x10];
    i32 pixelWidth;
    i32 pixelHeight;
    i32 tilePixelWidth;
    i32 tilePixelHeight;
    i32 tilesWide;
    i32 tilesHigh;
    i32 scrollX;
    i32 scrollY;
    i32 movementXPercent;
    i32 movementYPercent;
    u32 fillColor;
    u32 imageSetsCount;
    i32 objectsCount;
    u32 tilesOffset;
    u32 imageSetsOffset;
    u32 objectsOffset;
    i32 zCoord;
    u32 reserved94[3];
};

class CDDSurface;

struct PlaneObjectRecord {
    i32 m_id;

    union {
        struct {
            i32 m_nameLen;
            i32 m_logicLen;
            i32 m_imageSetLen;
            i32 m_soundLen;
            i32 m_x;
            i32 m_y;
            i32 m_z;
            i32 m_gridIndex;
            i32 m_addFlags;
            i32 m_dynamicFlags;
            i32 m_stateFlags;
            i32 m_userFlags;
            i32 m_score;
            i32 m_points;
            i32 m_powerup;
            i32 m_damage;
            i32 m_smarts;
            i32 m_health;
            RECT m_extent;
            RECT m_area;
            RECT m_switchRect;
            RECT m_clip;
            RECT m_userRect1;
            RECT m_userRect2;
            i32 m_user[8];
            i32 m_minX;
            i32 m_minY;
            i32 m_maxX;
            i32 m_maxY;
            i32 m_speedX;
            i32 m_speedY;
            i32 m_tweakX;
            i32 m_tweakY;
            i32 m_counter;
            i32 m_speed;
            i32 m_width;
            i32 m_height;
            i32 m_direction;
            i32 m_faceDirection;
            i32 m_timeDelay;
            i32 m_frameDelay;
            i32 m_objectType;
            i32 m_hitTypeFlags;
            i32 m_strideX;
            i32 m_strideY;
        };
        i32 m_fields[(0x11c - 0x4) / 4];
    };
    char m_strings[1];
};

// The tile-attribute table at WwdHeader::tileDescriptionsOffset, running to EOF:
// this header then a packed array of variable-stride records indexed by a tile
// handle's low word. Every shipped record is tag 1 (uniform, 32x32) - 63 files x
// 910 records, no exceptions - and the cursor lands exactly on EOF in all of
// them, which is what validates the strides. See docs/formats/wwd-v1.md.
struct WwdTileDescTable {
    // Proven unread. It is 0x20 in all 63 shipped levels and IS this header's
    // size, but LoadWwd hardcodes that: `lea ebx,[eax+0x20]` @0x15d3ba walks to
    // the first record without ever loading the field.
    u32 m_headerSize;
    u32 m_reserved04;
    u32 m_count;
    u32 m_reserved0c[5];
    char m_descriptors[1];
};

class CFileMemBase;

extern "C" i32 uncompress(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen);

class WwdFile {
public:
    static i32 ValidateMainBlock(CString name);

    static CString GetMapBaseName(CString path);
};

#endif // SRC_WWD_WWDFILE_H
