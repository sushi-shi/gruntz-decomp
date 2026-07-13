// ImagePool.h - the shared, single definition of CImagePool, the GDI surface /
// palette node pool (RTTI CImagePool). Formerly a .cpp-local class in ImagePool.cpp
// (+ a placeholder CPreviewMgr view in SaveGame.cpp for g_previewMgr, whose
// LoadImage IS CImagePool::AddSurfaceOp @0x1751f0). Extracted to a header (wave 3)
// so g_previewMgr is typed cast-free. The bodies live in ImagePool.cpp; every call
// into the CRezImage decoders / CImagePaletteNode builders reloc-masks.
#ifndef SRC_IMAGE_IMAGEPOOL_H
#define SRC_IMAGE_IMAGEPOOL_H

#include <Mfc.h> // CPtrList + <windows.h> (HINSTANCE/HWND/HPALETTE/PALETTEENTRY)
#include <Ints.h>
#include <rva.h>

class CRezImage; // <Image/Image.h> - the pool's DIB-surface node (pointer-only here)
// The palette list node (full def in ImagePool.cpp). Forward-declared in its
// placeholder namespace to keep that TU's method mangling unchanged.
namespace ApiCallerStubs {
    struct CImagePaletteNode;
}
using ApiCallerStubs::CImagePaletteNode;

SIZE_UNKNOWN(CImagePool);
class CImagePool {
public:
    // Inline ctor/dtor: the preview dialog is the only site that `new`s/`delete`s the
    // pool (g_previewMgr), and retail inlines both there - the two CPtrList(10) member
    // ctors + the five zeroed scalar fields, and a dtor that runs Clear() before the
    // member CPtrList teardowns. Modeled inline so MSVC folds them into that dialog proc.
    CImagePool() : m_surfaces(0xa), m_palettes(0xa) {
        m_resourceModuleHandle = 0;
        m_sourceHwnd = 0;
        m_08 = 0;
        m_48 = 0;
        m_selectedPalette = 0;
    }
    ~CImagePool() {
        Clear();
    }

    i32 SetHandles(i32 a, i32 b, i32 c);                                          // 0x174e90
    void Clear();                                                                 // 0x174eb0
    void Free(CRezImage* node);                                                   // 0x174ed0
    void RemovePalette(CImagePaletteNode* node);                                  // 0x174f30
    void ClearSurfaces();                                                         // 0x174f60
    void ClearPalettes();                                                         // 0x174fa0
    CImagePaletteNode* AddPaletteEntries(PALETTEENTRY* entries, i32 flags);       // 0x1754f0
    CImagePaletteNode* AddPaletteRGB(void* rgb, i32 flags);                       // 0x175570
    CImagePaletteNode* AddImageFile(char* path, i32 arg);                         // 0x1755f0
    CImagePaletteNode* AddImageDispatch(void* buf, u32 size, i32 type, i32 ctrl); // 0x175680
    i32 EnsureSurface(CRezImage* img, i32 w, i32 h, i32 bitCount, void* flag);    // 0x175710
    void B(CRezImage* node, i32 a, i32 b);                                        // 0x175780

    CRezImage* AddSurfaceBmp(i32 width, i32 height, i32 bitCount, i32 flag);           // 0x174fe0
    CRezImage* AddSurfaceBlit(i32 src, i32 width, i32 height, i32 bitCount, i32 flag); // 0x1750e0
    CRezImage* AddSurfaceOp(void* buf, i32 kind, i32 ctrl);                            // 0x1751f0
    CRezImage* AddSurfaceRez(i32 name, i32 ctrl);                                      // 0x1752f0
    CRezImage* AddSurfaceConvert(i32 src, i32 pal);                                    // 0x1753f0

    HINSTANCE m_resourceModuleHandle; // +0x00  resource module handle (-> g_hResModule)
    HWND m_sourceHwnd;                // +0x04  source HWND (GetDC/ReleaseDC)
    i32 m_08;                         // +0x08
    HPALETTE m_selectedPalette;       // +0x0c  selected HPALETTE to restore
    CPtrList m_surfaces;              // +0x10  GDI surface nodes (m_pHead @+0x14)
    CPtrList m_palettes;              // +0x2c  palette nodes (m_pHead @+0x30)
    i32 m_48;                         // +0x48
};

#endif // SRC_IMAGE_IMAGEPOOL_H
