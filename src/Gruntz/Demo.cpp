// Demo.cpp - CDemo (the CPlay-derived demo/attract-playback state) own methods.
// CDemo's identity + layout live in the canonical <Gruntz/Demo.h>; its /GX dtor
// is in PlayDtor.cpp and its slot-21 re-post in GruntzMgrTransition.cpp.
#include <Gruntz/Demo.h>
#include <rva.h>

// The first arg is the game-manager/entry context whose CString at +0xc8 (a
// pending-name latch) is cleared before delegating to the CPlay base entry.
SIZE_UNKNOWN(CDemoEnterCtx);
struct CDemoEnterCtx {
    char p0[0xc8];
    CString m_c8; // +0xc8
};

// 0x3bfa0 - CDemo::Vfunc1 (slot 1): clear the entry context's pending name, run
// the CPlay base slot-1 (CPlay::Vfunc1 == the mode/object initializer at 0xc7ec0);
// on failure return 0, else latch m_520 and return 1.
RVA(0x0003bfa0, 0x42)
i32 CDemo::Vfunc1(i32 ctx, i32 a1, i32 a2) {
    ((CDemoEnterCtx*)ctx)->m_c8.Empty();
    if (CPlay::Vfunc1(ctx, a1, a2) == 0) {
        return 0;
    }
    m_520 = 0x124f80;
    return 1;
}
