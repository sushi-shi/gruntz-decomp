#ifndef SRC_IMAGE_IMAGEPALETTENODE_H
#define SRC_IMAGE_IMAGEPALETTENODE_H

#include <Mfc.h> // POSITION + <windows.h> (HPALETTE / LOGPALETTE / PALETTEENTRY)
#include <Ints.h>
#include <rva.h>

namespace ApiCallerStubs {
    // The palette list node (+0x2c list). Build realizes the HPALETTE from a 256-entry
    // LOGPALETTE it assembles in-place; the front-ends fill that array. Run deletes the
    // realized HPALETTE before the node is freed. Only offsets the pool touches are pinned.
    struct CImagePaletteNode {
        HPALETTE m_palette;                     // +0x000  realized HPALETTE (CreatePalette)
        LOGPALETTE m_pal;                       // +0x004  header + entry[0]
        char m_padEntries[0x408 - (4 + 4 + 4)]; // entry[1..255] -> +0x408
        i32 m_flags;                            // +0x408  Build's stored flags (Run zeroes it)
        i32 m_systemTuned;                      // +0x40c  1 when reserved system range snapshotted
        POSITION m_listPosition;                // +0x410  cached AddTail POSITION
        i32 Build(PALETTEENTRY* entries, i32 flags);                // 0x176df0 (imagepool)
        void Tune();                                          // 0x1770e0 (imagepool)
        i32 ProcessPal(void* rgb, i32 flags);                       // 0x176e70 (imagepool)
        i32 ProcessPalQuad(void* bgr, i32 flags);                   // 0x176ec0 (imagepool)
        i32 ProcessPalBGR(void* bgr, i32 flags);                    // 0x176f30 (imagepool)
        i32 ParseDispatch(void* buf, u32 size, i32 type, i32 ctrl); // 0x177040 (imagepool)
        i32 ParsePaletteTail(void* buf, u32 size, i32 ctrl);        // 0x177400 (imagepool)
        void Run();                                                 // 0x177070 (imagepool)
        i32 LoadByExtension(char* path, i32 arg);                   // 0x176f90 (imagepool)
        i32 LoadPalFile(char* path, i32 arg);                       // 0x1771f0 (imagepool)
        i32 LoadPcxFile(char* path, i32 arg);                       // 0x1772e0 (imagepool)
        i32 LoadBmpFile(char* path, i32 arg);                       // 0x177480 (palettebmp)
        i32 Apply(char* path, i32 arg);                             // 0x1775f0 (palettebmp)
    };
} // namespace ApiCallerStubs

#endif // SRC_IMAGE_IMAGEPALETTENODE_H
