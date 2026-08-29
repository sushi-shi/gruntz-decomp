#ifndef SRC_IMAGE_IMAGEPALETTENODE_H
#define SRC_IMAGE_IMAGEPALETTENODE_H

#include <rva.h>

#include <Mfc.h>

#include <Image/RezDecodeKind.h>
#include <Ints.h>

struct DIB_LOGPAL256 {
    u16 version;
    u16 numEntries;
    PALETTEENTRY entries[256];
};

class CDibPal {
public:
    CDibPal();
    ~CDibPal() {
        Term();
    }

    i32 Init(PALETTEENTRY* entries, u32 flags = 0);
    i32 Init(u8* rgb, u32 flags = 0);
    i32 Init(RGBQUAD* quads, u32 flags = 0);
    i32 Init(RGBTRIPLE* triples, u32 flags = 0);
    i32 Init(const char* file, u32 flags = 0);
    i32 Init(u8* data, u32 dataSize, RezDecodeKind type, u32 flags = 0);
    void Term();

    b32 IsValid() {
        return m_hPal != NULL;
    }

    i32 InitPal(const char* file, u32 flags = 0);
    i32 InitPcx(const char* file, u32 flags = 0);
    i32 InitBmp(const char* file, u32 flags = 0);
    i32 InitRes(const char* file, u32 flags = 0);
    i32 InitPcx(u8* data, u32 dataSize, u32 flags = 0);

    HPALETTE GetHandle() {
        return m_hPal;
    }
    PALETTEENTRY* GetPes() {
        return m_logPal.entries;
    }
    u32 GetFlags() {
        return m_dwFlags;
    }
    POSITION GetPos() {
        return m_pos;
    }
    void SetPos(POSITION pos) {
        m_pos = pos;
    }
    b32 IsIdentity() {
        return m_bIdentity;
    }

    static i32 IsPaletteDevice();

private:
    void MakeIdentity();
    static void ClearSystemPalette();

    HPALETTE m_hPal;
    DIB_LOGPAL256 m_logPal;
    u32 m_dwFlags;
    b32 m_bIdentity;
    POSITION m_pos;
};

inline CDibPal::CDibPal() {
    m_hPal = NULL;
    m_bIdentity = false;
    m_pos = NULL;
}

#endif // SRC_IMAGE_IMAGEPALETTENODE_H
