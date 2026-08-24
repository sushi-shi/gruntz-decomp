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

    i32 Configure(HINSTANCE resourceModule, HWND sourceWindow, i32 reserved);
    void Clear();
    void RemoveSurface(CRezImage* image);
    void RemovePalette(CImagePaletteNode* palette);
    void ClearSurfaces();
    void ClearPalettes();
    CImagePaletteNode* CreatePaletteFromEntries(PALETTEENTRY* entries, i32 flags);
    CImagePaletteNode* CreatePaletteFromRgb(u8* rgb, i32 flags);
    CImagePaletteNode* LoadPaletteFromFile(char* path, i32 flags);
    CImagePaletteNode* LoadPaletteFromData(u8* data, u32 dataSize, RezDecodeKind format, i32 flags);
    i32 ResizeSurface(CRezImage* image, i32 width, i32 height, ColorDepth bitDepth, i32 flags);
    void SetImagePalette(CRezImage* image, CImagePaletteNode* palette, i32 scalar);

    CRezImage* CreateSurface(i32 width, i32 height, ColorDepth bitDepth, i32 flags);
    CRezImage*
    CreateSurfaceFromPixels(u8* pixels, i32 width, i32 height, ColorDepth bitDepth, i32 flags);
    CRezImage* LoadSurfaceFromData(u8* data, RezDecodeKind format, i32 flags);
    CRezImage* LoadSurfaceFromResource(char* resourceName, i32 flags);
    CRezImage* ConvertSurface(CRezImage* source, CImagePaletteNode* palette);

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
