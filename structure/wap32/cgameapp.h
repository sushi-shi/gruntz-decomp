#ifndef WAP32_CGAMEAPP_H
#define WAP32_CGAMEAPP_H

/*
 * WAP32::CGameApp — engine application base (CGruntzApp derives from this).
 * .?AVCGameApp@@
 *
 * LAYOUT PORTED FROM tomalla (refs/tomalla-gruntz/wap32/cgameapp.h), attributed.
 * @address / @vftable / @offset values are for the PATCHED build 1.0.1.77, NOT our
 * v1.0.0.76 target — treat addresses as approximate, re-verify before matching.
 * Field+vtable layout = high confidence (tomalla reconstruction); method NAMES that
 * are tomalla guesses keep their "UnknownVirtualMethodNN" form.
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>

namespace WAP32
{
    class CGameWnd;
    class CGameMgr;

    /* GameInfo — window/launch descriptor passed around during init. */
    struct GameInfo
    {
        //@size: 0x1d4   (ported from tomalla)

        //@offset: 0
        int size;
        //@offset: 4
        char windowClassFlags;
        //@offset: 8
        HINSTANCE hInstance;
        //@offset: c
        char szCmdLine[128];
        //@offset: 8c
        // used for: window-class name "<fragment>Class"; loading cursor/icon/menu
        // of the same name.
        char szGameIdentifier[64];
        //@offset: cc
        char szWindowName[64];

        //@offset: 10c
        char _padding2[0x40];

        //@offset: 14c
        char szWindowClassName[128];

        //@offset: 1cc
        int windowWidth;
        //@offset: 1d0
        int windowHeight;
    };

    class CGameApp
    {
    public:
        CGameApp();
        virtual ~CGameApp();

        struct WindowClassFlags
        {
            // if Windowed set: default cursor used; window created with "Gruntz"
            // menu; window uses width/height instead of fullscreen.
            static const int Windowed = 0x1;
            static const int DialogFrame = 0x2;
        };

        //@vftable: 0  vector deleting destructor (-> @address: 00480cf0)
        //@vftable: 4
        //@todo
        virtual bool VirtualUnknownMethod02(GameInfo* pGameInfo, WNDCLASSA* pWndClass, CREATESTRUCTA* pCreateStruct);
        //@vftable: 8
        //@todo
        virtual bool VirtualUnknownMethod03(HINSTANCE hInstance, char* szWindowName, char* szGameIdentifier, char* szCmdLine, char windowClassFlags, int windowWidth, int windowHeight);
        //@vftable: C
        //@todo
        virtual void VirtualUnknownMethod04() {}
        //@vftable: 10
        virtual void CloseResources();
        //@vftable: 14
        //@todo
        virtual void VirtualUnknownMethod06() {}
        //@vftable: 18
        //@todo
        virtual void VirtualUnknownMethod07() {}
        //@vftable: 1C
        virtual void ReportError(int errorMessageId, int errorCode);
        //@vftable: 20
        //@todo
        virtual void VirtualUnknownMethod09() {}
        //@vftable: 24
        virtual void FreeGameManager();
        //@vftable: 28
        //@todo
        virtual void VirtualUnknownMethod11() {}
        //@vftable: 2C
        virtual bool InitializeAccelerators(char* szGameIdentifier);
        //@vftable: 30
        virtual void ShowError();
        //@vftable: 34
        virtual WAP32::CGameWnd* InitializeGameWindow();
        //@vftable: 38
        virtual WAP32::CGameMgr* InitializeGameManager();
        //@vftable: 3C
        virtual void InitializeDefaultWindowClass();
        //@vftable: 40
        virtual void InitializeDefaultCreateStruct();

        //@offset: 4
        WAP32::CGameWnd* m_pGameWnd;
        //@offset: 8
        WAP32::CGameMgr* m_pGameMgr;
        //@offset: c
        HINSTANCE m_hInstance;
        //@offset: 10
        HACCEL m_hAccelerators;
        //@offset: 14 (size: 1d4)
        GameInfo m_gameInfo;
        //@offset: 1e8 (size: 28)
        WNDCLASSA m_wndClass;
        //@offset: 210
        CREATESTRUCTA m_createStruct;
        //@offset: 240
        unsigned int fieldUnknown240;
        //@offset: 244
        bool fieldUnknown244_errorRelatedFlag;
        //@offset: 248
        bool m_isError;
        //@offset: 24c
        unsigned int m_errorMessageId;
        //@offset: 250
        unsigned int m_errorCode;

    private:
        //@address: 00654bc4
        static int m_referenceCounter;

        static LRESULT CALLBACK GameWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    };
}

#endif /* WAP32_CGAMEAPP_H */
