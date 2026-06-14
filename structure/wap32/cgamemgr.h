#ifndef WAP32_CGAMEMGR_H
#define WAP32_CGAMEMGR_H

/*
 * WAP32::CGameMgr — engine game-manager base (CGruntzMgr derives from this).
 * .?AVCGameMgr@@  (size 0x2c)
 *
 * Layout ported from tomalla (@approx tomalla 1.0.1.77; offsets version-independent).
 * NOT yet graduated to src/ (src only forward-declares WAP32::CGameMgr) — this is
 * the comprehension layout. Polymorphic: the vptr lands at +0x00 and the fields
 * follow at +0x04.
 */

namespace WAP32
{
    class CGameApp;
    class CGameWnd;

    class CGameMgr
    {
    public:
        CGameMgr();
        virtual ~CGameMgr();                 // vtbl +0x00 (vector deleting dtor)
        virtual int  UnknownVirtualMethod1(CGameWnd *pGameWnd, char *szCmdLine); // +0x04
        virtual void UnknownClose();          // +0x08
        virtual void UnknownVirtualMethod3(); // +0x0c
        virtual void UnknownVirtualMethod4(); // +0x10
        virtual void UnknownVirtualMethod5(); // +0x14

        CGameWnd *m_pGameWnd;        // +0x04
        CGameApp *m_pGameApp;        // +0x08
        int       fieldUnknown00C;   // +0x0c
        int       m_isSoundEnabled;  // +0x10
        int       m_isMusicEnabled;  // +0x14
        int       fieldUnknown018;   // +0x18
        int       fieldUnknown01C;   // +0x1c
        int       fieldUnknown020;   // +0x20
        int       fieldUnknown024;   // +0x24
        char      _pad28[0x2c - 0x28]; // +0x28  (tail pad to 0x2c)
    };
}

#endif /* WAP32_CGAMEMGR_H */
