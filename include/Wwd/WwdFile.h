#include <Mfc.h>
#include <Ints.h>
#include <rva.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#ifndef SRC_WWD_WWDFILE_H
#define SRC_WWD_WWDFILE_H

class CDDrawWorker;

typedef u8 Bytef;
typedef u32 uLong;
typedef u32 uLongf;

struct WwdHeader {
    u32 wwdSignature;
    u32 field_4;
    u32 flags;
    u8 pad_c[0x10 - 0x0c];
    char levelName[0x2d0 - 0x10];
    i32 startX;
    i32 startY;
    u32 pad_2d8;
    u32 numPlanes;
    u32 planesOffset;
    u32 tileDescriptionsOffset;
    u32 mainBlockLength;
    u32 checksum;
    u8 pad_2f0[0x5f4 - 0x2f0];
};
SIZE(0x5f4);

struct WwdPlaneHeader {
    u32 headerSize;
    u32 field_04;
    u32 flags;

    u32 field_0c;
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
    u8 pad_94[0xa0 - 0x94];
};
SIZE(0xa0);

class WwdInputStream {
public:
    WwdInputStream();
    ~WwdInputStream();
    i32 Open(const char* name, i32 mode, void* errSink);
    i32 Read(void* buf, i32 len);

private:
    char _vft0[4];
    HANDLE m_handle;
    i32 m_open;
    char* m_name;
};
SIZE(0x10);

class CDDSurface;

struct PlaneObjectRecord {
    i32 m_id;

    i32 m_nameLen;
    i32 m_logicLen;
    i32 m_imageSetLen;
    i32 m_soundLen;
    i32 m_x;
    i32 m_y;
    i32 m_z;
    i32 m_gridIndex;
    i32 m_addFlags;

    i32 m_tail[(0x11c - 0x28) / 4];
    char m_strings[1];
};
SIZE_UNKNOWN();

struct WwdTileDescTable {
    char m_00[0x8];
    u32 m_count;
    char m_0c[0x20 - 0xc];
    char m_descriptors[1];
};
SIZE_UNKNOWN();

class CFileMemBase;

extern void* operator new(u32 size);
extern void operator delete(void* p);

extern "C" i32 uncompress(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen);

extern "C" Bytef* __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, u32 destLen);

class WwdFile {
public:
    static i32 ValidateMainBlock(CString name);

    static CString GetMapBaseName(CString path);
};
SIZE_UNKNOWN();

#endif // SRC_WWD_WWDFILE_H
