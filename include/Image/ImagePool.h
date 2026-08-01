#ifndef SRC_IMAGE_IMAGEPOOL_H
#define SRC_IMAGE_IMAGEPOOL_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>
#include <Io/SaveGame.h>

class CRezImage;
namespace ApiCallerStubs {
    struct CImagePaletteNode;
}
using ApiCallerStubs::CImagePaletteNode;

class CImagePool {
public:
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

    i32 SetHandles(HINSTANCE resModule, HWND src, i32 c);
    void Clear();
    void Free(CRezImage* node);
    void RemovePalette(CImagePaletteNode* node);
    void ClearSurfaces();
    void ClearPalettes();
    CImagePaletteNode* AddPaletteEntries(PALETTEENTRY* entries, i32 flags);
    CImagePaletteNode* AddPaletteRGB(void* rgb, i32 flags);
    CImagePaletteNode* AddImageFile(char* path, i32 arg);
    CImagePaletteNode* AddImageDispatch(void* buf, u32 size, i32 type, i32 ctrl);
    i32 EnsureSurface(CRezImage* img, i32 w, i32 h, i32 bitCount, i32 flag);
    void B(CRezImage* node, void* paletteNode, i32 b);

    CRezImage* AddSurfaceBmp(i32 width, i32 height, i32 bitCount, i32 flag);
    CRezImage* AddSurfaceBlit(void* src, i32 width, i32 height, i32 bitCount, i32 flag);
    CRezImage* AddSurfaceOp(void* buf, i32 kind, i32 ctrl);
    CRezImage* AddSurfaceRez(char* name, i32 ctrl);
    CRezImage* AddSurfaceConvert(CRezImage* src, void* pal);

    HINSTANCE m_resourceModuleHandle;
    HWND m_sourceHwnd;
    i32 m_08;
    HPALETTE m_selectedPalette;
    CPtrList m_surfaces;
    CPtrList m_palettes;
    i32 m_48;
};
SIZE_UNKNOWN();

extern "C" HINSTANCE g_hResModule;

extern "C" HINSTANCE g_hResModule;

extern char g_bmpHeaderTemplate[];

namespace ApiCallerStubs {
    i32 DisplayUsesPalette();
    void ResetSystemPalette();
} // namespace ApiCallerStubs

#endif // SRC_IMAGE_IMAGEPOOL_H
