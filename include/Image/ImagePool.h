#ifndef SRC_IMAGE_IMAGEPOOL_H
#define SRC_IMAGE_IMAGEPOOL_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ColorDepth.h>
#include <Enums.h>
#include <Image/RezDecodeKind.h>
#include <Ints.h>
#include <Io/SaveGame.h>

class CRezImage;
struct CImagePaletteNode;

class CImagePool {
public:
    CImagePool() : m_surfaces(0xa), m_palettes(0xa) {
        m_resourceModuleHandle = NULL;
        m_sourceHwnd = NULL;
        m_reserved08 = 0;
        m_reserved48 = 0;
        m_selectedPalette = NULL;
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
    CImagePaletteNode* AddImageDispatch(void* buf, u32 size, RezDecodeKind type, i32 ctrl);
    i32 EnsureSurface(CRezImage* img, i32 w, i32 h, ColorDepth bitCount, i32 flag);
    void B(CRezImage* node, void* paletteNode, i32 b);

    CRezImage* AddSurfaceBmp(i32 width, i32 height, ColorDepth bitCount, i32 flag);
    CRezImage* AddSurfaceBlit(void* src, i32 width, i32 height, ColorDepth bitCount, i32 flag);
    CRezImage* AddSurfaceOp(void* buf, RezDecodeKind kind, i32 ctrl);
    CRezImage* AddSurfaceRez(char* name, i32 ctrl);
    CRezImage* AddSurfaceConvert(CRezImage* src, void* pal);

    HINSTANCE m_resourceModuleHandle;
    HWND m_sourceHwnd;
    i32 m_reserved08;
    HPALETTE m_selectedPalette;
    CPtrList m_surfaces;
    CPtrList m_palettes;
    i32 m_reserved48;
};

extern HINSTANCE g_hResModule;

extern char g_bmpHeaderTemplate[];

i32 DisplayUsesPalette();
void ResetSystemPalette();

#endif // SRC_IMAGE_IMAGEPOOL_H
