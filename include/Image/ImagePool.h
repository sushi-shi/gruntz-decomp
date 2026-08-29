#ifndef SRC_IMAGE_IMAGEPOOL_H
#define SRC_IMAGE_IMAGEPOOL_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/ColorDepth.h>
#include <Enums.h>
#include <Image/ImagePaletteNode.h>
#include <Image/RezDecodeKind.h>
#include <Ints.h>

class CDib;

GZ_ENUM_FLAGS_BEGIN(DibMgrFlags, u32)
    DMPF_NOIDENTITY = 0x00000001,
GZ_ENUM_FLAGS_END(DibMgrFlags, u32)
GZ_ENUM_FLAGS_OPS(DibMgrFlags)

class CDibMgr {
public:
    CDibMgr();
    ~CDibMgr() {
        Term();
    }

    i32 Init(HINSTANCE instance, HWND window, u32 flags = 0);
    void Term();

    b32 IsValid() {
        return m_hInst != NULL && m_hWnd != NULL;
    }
    i32 GetNumDibs() {
        return m_collDibs.GetCount();
    }
    i32 GetNumPals() {
        return m_collPals.GetCount();
    }
    u32 GetFlags() {
        return m_dwFlags;
    }
    CDibPal* GetCurPal() {
        return m_pCurPal;
    }

    void SetCurPal(CDibPal* palette) {
        m_pCurPal = palette;
    }
    void SetWindowHandle(HWND window) {
        ASSERT(window != NULL);
        ASSERT(::IsWindow(window));
        m_hWnd = window;
    }
    void SetPalette(CDib* dib, CDibPal* palette, b32 owner = false);

    CDib* AddDib(i32 width, i32 height, ColorDepth depth = BPP_PALETTED_8, u32 flags = 0);
    CDib*
    AddDib(u8* bytes, i32 width, i32 height, ColorDepth depth = BPP_PALETTED_8, u32 flags = 0);
    CDib* AddDib(u8* bytes, RezDecodeKind type, u32 flags = 0);
    CDib* AddDib(const char* file, u32 flags = 0);
    CDib* AddDib(CDib* original, CDibPal* palette);

    CDibPal* AddPal(PALETTEENTRY* entries, u32 flags = 0);
    CDibPal* AddPal(u8* rgb, u32 flags = 0);
    CDibPal* AddPal(const char* file, u32 flags = 0);
    CDibPal* AddPal(u8* data, u32 dataSize, RezDecodeKind type, u32 flags = 0);

    void RemoveDib(CDib* dib);
    void RemovePal(CDibPal* palette);
    i32
    ResizeDib(CDib* dib, i32 width, i32 height, ColorDepth depth = BPP_PALETTED_8, u32 flags = 0);
    void RemoveAllDibs();
    void RemoveAllPals();

    HDC GetDC(b32 prepare = true, b32 backgroundPalette = false) {
        HDC dc = ::GetDC(m_hWnd);
        if (prepare && dc != NULL && m_pCurPal != NULL) {
            m_hOldPal = SelectPalette(dc, m_pCurPal->GetHandle(), backgroundPalette);
            RealizePalette(dc);
        }
        return dc;
    }
    b32 PrepDC(HDC dc) {
        if (m_pCurPal != NULL) {
            m_hOldPal = SelectPalette(dc, m_pCurPal->GetHandle(), false);
            RealizePalette(dc);
            return true;
        }
        return false;
    }
    void ReleaseDC(HDC dc) {
        if (m_hOldPal != NULL) {
            SelectPalette(dc, m_hOldPal, false);
            m_hOldPal = NULL;
        }
        ::ReleaseDC(m_hWnd, dc);
    }

    static HINSTANCE GetGlobalInstanceHandle() {
        return s_hInst;
    }

private:
    HINSTANCE m_hInst;
    HWND m_hWnd;
    u32 m_dwFlags;
    HPALETTE m_hOldPal;

    CPtrList m_collDibs;
    CPtrList m_collPals;

    CDibPal* m_pCurPal;

    static HINSTANCE s_hInst;
};

inline CDibMgr::CDibMgr() {
    m_hInst = NULL;
    m_hWnd = NULL;
    m_dwFlags = 0;
    m_pCurPal = NULL;
    m_hOldPal = NULL;
}

extern char g_bmpHeaderTemplate[];

i32 DisplayUsesPalette();
void ResetSystemPalette();

#endif // SRC_IMAGE_IMAGEPOOL_H
