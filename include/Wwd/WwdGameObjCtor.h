#ifndef GRUNTZ_WWD_WWDGAMEOBJCTOR_H
#define GRUNTZ_WWD_WWDGAMEOBJCTOR_H

// WwdGameObjCtor.h - the shared CWwdGameObject base-object ctor cluster (hoisted
// from WwdObjMgrFactories.cpp, wave4-L): the CResolveNode-shaped inline ctor base
// (WwdCtorBase), the 0x15b390 /GX base-object ctor class (CWwdGameObj15b390), and
// the +0x7c anim worker's inline-construction view (WwdAnimWorker). The factories
// (H obj, src/Wwd/WwdObjMgr.cpp) placement-construct these; the ctor body itself
// (0x15b390) lives in the I obj (src/Wwd/WwdFactoryObject.cpp).
//
// Field names are placeholders; only OFFSETS + emitted code bytes are load-bearing.

#include <Ints.h>
#include <Mfc.h> // CString (the +0xdc label member)
#include <rva.h>

// The CResolveNode base subobject: stamps 0x5efbc0 + the +0x04..+0xd8 field block.
struct WwdCtorBase {
    WwdCtorBase(int a, int b, int c) {
        m_08 = c;
        m_04 = b;
        m_0c = a;
        m_20 = (int)0x80000000;
        m_38 = -1;
        // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0) // 0x5efbc0
        m_5c = (int)0x80000000;
        m_64 = (int)0x80000000;
        m_3c = 0;
        m_40 = 0;
        m_a8 = 0;
        m_a4 = 0;
        m_b4 = 0;
        m_c0 = (int)0x80000000;
        m_d8 = -1;
    }
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    int m_04, m_08, m_0c;
    char _p10[0x20 - 0x10];
    int m_20;
    char _p24[0x38 - 0x24];
    int m_38, m_3c, m_40;
    char _p44[0x5c - 0x44];
    int m_5c;
    char _p60[0x64 - 0x60];
    int m_64;
    char _p68[0x78 - 0x68];
    int m_78;
    void* m_7c; // +0x7c worker
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
SIZE_UNKNOWN(WwdCtorBase); // CResolveNode base subobject (+0x00..+0xd8)

struct CWwdGameObj15b390 : public WwdCtorBase {
    CString m_label; // +0xdc  ??0CString (0x1b9b93)
    char _pe0[0x188 - 0xe0];
    int m_188;                              // +0x188  object id
    CWwdGameObj15b390(int a, int b, int c); // 0x15b390 (I obj)
};
SIZE_UNKNOWN(CWwdGameObj15b390); // 0x15b390 per-kind wide-object ctor (CResolveNode base)

// The +0x7c sprite/anim worker (0x17c bytes). `new`-constructed inline in the base
// ctor; self-contained inline-construction view (only the field stores matter).
struct WwdAnimWorkerInit {
    WwdAnimWorkerInit(int b, int a) {
        m_04 = b;
        m_08 = 0;
        m_0c = a;
        // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0) // 0x5efb80
        m_10 = 0;
        m_14 = 0;
        m_18 = 0;
        m_170 = 0;
        m_1c = 0;
        m_174 = 0;
        m_178 = 0;
    }
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    int m_04, m_08, m_0c, m_10, m_14, m_18, m_1c;
    char _p20[0x170 - 0x20];
    int m_170, m_174, m_178;
};
SIZE(WwdAnimWorkerInit, 0x17c); // the +0x7c anim worker

#endif // GRUNTZ_WWD_WWDGAMEOBJCTOR_H
