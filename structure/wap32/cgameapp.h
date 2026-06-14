#ifndef WAP32_CGAMEAPP_H
#define WAP32_CGAMEAPP_H

/*
 * WAP32::CGameApp — engine application base (CGruntzApp derives from this).
 * .?AVCGameApp@@  (size 0x254)
 *
 * Layout ported from tomalla (@approx tomalla 1.0.1.77; offsets version-independent).
 * The class has graduated into src/Wap32/Wap32.h (matched 0x254 layout); this
 * comprehension restatement exists only so the unmatched subclass CGruntzApp lays
 * out standalone under gen_structs' per-header wrapping (gen_structs prefers the
 * src/ CGameApp on overlap). Self-contained (no <afxwin.h>): the few Win32 handle
 * types the layout needs are typedef'd here so the header parses standalone.
 *
 * Polymorphic: the vptr lands at +0x00 and the fields follow at +0x04. The
 * 0x1e8..0x240 gap holds the embedded WNDCLASSA (0x28) + CREATESTRUCTA (0x30)
 * structs (see src/Wap32/Wap32.h); modeled here as padding since only the named
 * field offsets are load-bearing.
 */

namespace WAP32
{
    class CGameWnd;
    class CGameMgr;

    typedef void *AppHINSTANCE;  // HINSTANCE
    typedef void *AppHACCEL;     // HACCEL

    /*
     * GameInfo — the 0x1d4-byte window/launch descriptor. Embedded in CGameApp at
     * +0x14 (m_gameInfo). Field set + offsets ported from tomalla.
     */
    struct GameInfo
    {
        int           size;                  // +0x000  == sizeof(GameInfo) == 0x1d4
        char          windowClassFlags;      // +0x004  bit1=Windowed, bit2=DialogFrame
        char          _pad005[0x8 - 0x5];    // +0x005
        AppHINSTANCE  hInstance;             // +0x008
        char          szCmdLine[128];        // +0x00c
        char          szGameIdentifier[64];  // +0x08c  (cursor/icon/menu resource name)
        char          szWindowName[64];      // +0x0cc
        char          _pad10c[0x14c - 0x10c];// +0x10c
        char          szWindowClassName[128];// +0x14c
        int           windowWidth;           // +0x1cc
        int           windowHeight;          // +0x1d0
    };                                       // 0x1d4

    class CGameApp
    {
    public:
        CGameApp();
        virtual ~CGameApp();                 // vtbl +0x00 (vector deleting dtor)
        virtual int  VirtualUnknownMethod02();  // +0x04
        virtual int  VirtualUnknownMethod03();  // +0x08
        virtual void VirtualUnknownMethod04();  // +0x0c
        virtual void CloseResources();          // +0x10
        virtual void VirtualUnknownMethod06();  // +0x14
        virtual void VirtualUnknownMethod07();  // +0x18
        virtual void ReportError(int errorMessageId, int errorCode); // +0x1c
        virtual void VirtualUnknownMethod09();  // +0x20
        virtual void FreeGameManager();         // +0x24
        virtual void VirtualUnknownMethod11();  // +0x28
        virtual int  InitializeAccelerators(char *szGameIdentifier); // +0x2c
        virtual void ShowError();               // +0x30
        virtual CGameWnd *InitializeGameWindow();   // +0x34
        virtual CGameMgr *InitializeGameManager();  // +0x38
        virtual void InitializeDefaultWindowClass();   // +0x3c
        virtual void InitializeDefaultCreateStruct();  // +0x40

        CGameWnd    *m_pGameWnd;      // +0x004
        CGameMgr    *m_pGameMgr;      // +0x008
        AppHINSTANCE m_hInstance;     // +0x00c
        AppHACCEL    m_hAccelerators; // +0x010
        GameInfo     m_gameInfo;      // +0x014  (0x1d4)
        char         _pad1E8[0x240 - 0x1e8]; // +0x1e8  WNDCLASSA + CREATESTRUCTA
        unsigned int fieldUnknown240;                 // +0x240
        char         fieldUnknown244_errorRelatedFlag;// +0x244 (bool)
        char         _pad245[0x248 - 0x245];          // +0x245
        char         m_isError;       // +0x248 (bool)
        char         _pad249[0x24c - 0x249];          // +0x249
        unsigned int m_errorMessageId;// +0x24c
        unsigned int m_errorCode;     // +0x250
    };                                // 0x254
}

#endif /* WAP32_CGAMEAPP_H */
