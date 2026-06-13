#ifndef WAP32_CGAMEMGR_H
#define WAP32_CGAMEMGR_H

/*
 * WAP32::CGameMgr — engine game-manager base (CGruntzMgr derives from this).
 * .?AVCGameMgr@@
 *
 * LAYOUT PORTED FROM tomalla (refs/tomalla-gruntz/wap32/cgamemgr.h), attributed.
 * @address values are for the PATCHED build 1.0.1.77 — approximate for v1.0.0.76.
 */

#include "cgamewnd.h"

namespace WAP32
{
    class CGameApp;

    class CGameMgr
    {
    public:
        //@size: 2c   (ported from tomalla)

        //@todo
        CGameMgr();
        virtual ~CGameMgr();

        //@vftable: 0  vector deleting destructor (-> @address: 004854c0)
        //@vftable: 4
        virtual bool UnknownVirtualMethod1(WAP32::CGameWnd* pGameWnd, char* szCmdLine);
        //@vftable: 8
        virtual void UnknownClose();
        //@vftable: C
        virtual void UnknownVirtualMethod3() {}
        //@vftable: 10
        virtual void UnknownVirtualMethod4() {}
        //@vftable: 14
        virtual void UnknownVirtualMethod5() {}

        //@offset: 4
        WAP32::CGameWnd* m_pGameWnd;
        //@offset: 8
        WAP32::CGameApp* m_pGameApp;
        //@offset: c
        int fieldUnknown00C;
        //@offset: 10
        int m_isSoundEnabled;
        //@offset: 14
        int m_isMusicEnabled;
        //@offset: 18
        int fieldUnknown018;
        //@offset: 1C
        int fieldUnknown01C;
        //@offset: 20
        int fieldUnknown020;
        //@offset: 24
        int fieldUnknown024;
        //@offset: 28
        char _padding[4];

        //@address: 00654bc8
        static int unknown_cgamemgr_global_time;
        //@address: 00654bcc
        static int unknown_cgamemgr_global_time_related1;
        //@address: 00654bd0
        static int unknown_cgamemgr_global_time_related2;
        //@address: 00654bd4
        static int unknown_cgamemgr_global_time_related3;
        //@address: 00654bd8
        static int unknown_cgamemgr_global_time_related4;

    private:
        void UnknownMethodInitializeTime(bool param_1);
        void UnknownMethodInitializeTimeGlobal();
    };
}

#endif /* WAP32_CGAMEMGR_H */
