#ifndef SRC_IMAGE_IMAGEPALETTENODE_H
#define SRC_IMAGE_IMAGEPALETTENODE_H

#include <rva.h>

#include <Mfc.h>

#include <Ints.h>

struct CImagePaletteNode {
    HPALETTE m_palette;
    LOGPALETTE m_pal;
    char m_padEntries[0x408 - (4 + 4 + 4)];
    i32 m_flags;
    i32 m_systemTuned;
    POSITION m_listPosition;

    CImagePaletteNode() {
        m_palette = NULL;
        m_systemTuned = false;
        m_listPosition = NULL;
    }
    i32 Build(PALETTEENTRY* entries, i32 flags);
    void Tune();
    i32 ProcessPal(void* rgb, i32 flags);
    i32 ProcessPalQuad(void* bgr, i32 flags);
    i32 ProcessPalBGR(void* bgr, i32 flags);
    i32 ParseDispatch(void* buf, u32 size, i32 type, i32 ctrl);
    i32 ParsePaletteTail(void* buf, u32 size, i32 ctrl);
    void Run();
    i32 LoadByExtension(char* path, i32 arg);
    i32 LoadPalFile(char* path, i32 arg);
    i32 LoadPcxFile(char* path, i32 arg);
    i32 LoadBmpFile(char* path, i32 arg);
    i32 Apply(char* path, i32 arg);
};
SIZE(0x414);

#endif // SRC_IMAGE_IMAGEPALETTENODE_H
