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
    u32 m_headerSize;
    u32 m_reserved04;
    u32 m_flags;
    u32 m_reserved0c;
    char m_levelName[0x40];
    char m_author[0x40];
    char m_created[0x40];
    char m_rezFile[0x100];
    char m_tileDirectory[0x80];
    char m_palette[0x80];
    i32 m_startX;
    i32 m_startY;
    u32 m_reserved2d8;
    u32 m_numPlanes;
    u32 m_planesOffset;
    u32 m_tileDescriptionsOffset;
    u32 m_mainBlockLength;
    u32 m_checksum;
    u32 m_reserved2f0;
    char m_launchApp[0x80];
    char m_imageDirectory[4][0x80];
    char m_imagePrefix[4][0x20];
};

struct WwdPlaneHeader {
    u32 m_headerSize;
    u32 m_reserved04;
    GZ_ENUM_STORAGE(WwdPlaneFlags, u32) m_flags;

    u32 m_reserved0c;
    char m_name[0x50 - 0x10];
    i32 m_pixelWidth;
    i32 m_pixelHeight;
    i32 m_tilePixelWidth;
    i32 m_tilePixelHeight;
    i32 m_tilesWide;
    i32 m_tilesHigh;
    i32 m_scrollX;
    i32 m_scrollY;
    i32 m_movementXPercent;
    i32 m_movementYPercent;
    u32 m_fillColor;
    u32 m_imageSetsCount;
    i32 m_objectsCount;
    u32 m_tilesOffset;
    u32 m_imageSetsOffset;
    u32 m_objectsOffset;
    i32 m_zCoord;
    u32 m_reserved94[3];
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
