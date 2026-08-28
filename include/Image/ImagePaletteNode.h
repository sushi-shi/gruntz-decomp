#ifndef SRC_IMAGE_IMAGEPALETTENODE_H
#define SRC_IMAGE_IMAGEPALETTENODE_H

#include <rva.h>

#include <Mfc.h>

#include <Image/RezDecodeKind.h>
#include <Ints.h>

struct CImagePaletteNode {
    HPALETTE m_palette;
    LOGPALETTE m_logicalPalette;
    char m_padEntries[0x408 - (4 + 4 + 4)];
    i32 m_flags;
    b32 m_reservedSystemColors;
    POSITION m_listPosition;

    CImagePaletteNode() {
        m_palette = NULL;
        m_reservedSystemColors = false;
        m_listPosition = NULL;
    }
    PALETTEENTRY* Entries() {
        return m_logicalPalette.palPalEntry;
    }
    i32 CreateFromEntries(PALETTEENTRY* entries, i32 flags);
    void ReserveSystemColors();
    i32 CreateFromRgb(u8* rgb, i32 flags);
    i32 CreateFromBgrx(u8* bgrx, i32 flags);
    i32 CreateFromBgr(u8* bgr, i32 flags);
    i32 LoadFromData(u8* data, u32 dataSize, RezDecodeKind format, i32 flags);
    i32 CreateFromTrailingRgb(u8* data, u32 dataSize, i32 flags);
    void Destroy();
    i32 LoadFromFile(char* path, i32 flags);
    i32 LoadPalFile(char* path, i32 flags);
    i32 LoadPcxFile(char* path, i32 flags);
    i32 LoadBmpFile(char* path, i32 flags);
    i32 LoadFromResource(char* resourceName, i32 flags);
};

#endif // SRC_IMAGE_IMAGEPALETTENODE_H
