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

struct WwdHeader {
    u32 headerSize;
    u32 reserved04;
    u32 flags;
    u32 reserved0c;
    char levelName[0x40];
    char author[0x40];
    char created[0x40];
    char rezFile[0x100];
    char tileDirectory[0x80];
    char palette[0x80];
    i32 startX;
    i32 startY;
    u32 reserved2d8;
    u32 numPlanes;
    u32 planesOffset;
    u32 tileDescriptionsOffset;
    u32 mainBlockLength;
    u32 checksum;
    u32 reserved2f0;
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

struct WwdTileDescTable {
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
