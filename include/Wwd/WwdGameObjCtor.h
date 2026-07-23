#ifndef GRUNTZ_WWD_WWDGAMEOBJCTOR_H
#define GRUNTZ_WWD_WWDGAMEOBJCTOR_H

#include <Ints.h>
#include <Mfc.h> // CString (the +0xdc label member)
#include <rva.h>

struct AnimWorkerObj; // <DDrawMgr/AnimWorkerObj.h> - the owned +0x7c worker (canonical

struct WwdCtorBase {
    WwdCtorBase(int a, int b, int c) {
        m_08 = c;
        m_04 = b;
        m_0c = a;
        m_20 = static_cast<int>(0x80000000);
        m_38 = -1;
        // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0) // 0x5efbc0
        m_screenX = static_cast<int>(0x80000000);
        m_64 = static_cast<int>(0x80000000);
        m_3c = 0;
        m_40 = 0;
        m_a8 = 0;
        m_a4 = 0;
        m_b4 = 0;
        m_c0 = static_cast<int>(0x80000000);
        m_d8 = -1;
    }
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    int m_04, m_08, m_0c;
    char _p10[0x20 - 0x10];
    int m_20;
    char _p24[0x38 - 0x24];
    int m_38, m_3c, m_40;
    char _p44[0x5c - 0x44];
    int m_screenX; // +0x5c (node m_screenX)
    char _p60[0x64 - 0x60];
    int m_64;
    char _p68[0x78 - 0x68];
    int m_78;
    AnimWorkerObj* m_7c; // +0x7c  owned worker (canonical WwdGameObjectFamily.h m_7c)
    int m_80;
    char _p84[0x88 - 0x84];
    int m_88;
    char _p8c[0x90 - 0x8c];
    int m_90;
    char _p94[0x98 - 0x94];
    int m_98;
    char _p9c[0xa4 - 0x9c];
    int m_a4, m_a8;
    char _pac[0xb4 - 0xac];
    int m_b4;
    char _pb8[0xc0 - 0xb8];
    int m_c0;
    char _pc4[0xd8 - 0xc4];
    int m_d8;
};
SIZE_UNKNOWN(); // CResolveNode base subobject (+0x00..+0xd8)

struct CWwdGameObjBaseCtor : public WwdCtorBase {
    CString m_label; // +0xdc  ??0CString (0x1b9b93)
    char _pe0[0x188 - 0xe0];
    int m_188;                                // +0x188  object id
    CWwdGameObjBaseCtor(int a, int b, int c); // 0x15b390 (I obj)
};
SIZE_UNKNOWN(); // 0x15b390 per-kind wide-object ctor (CResolveNode base)

#include <DDrawMgr/AnimWorkerObj.h>

#endif // GRUNTZ_WWD_WWDGAMEOBJCTOR_H
