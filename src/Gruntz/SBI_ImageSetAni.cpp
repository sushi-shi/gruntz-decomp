// SBI_ImageSetAni.cpp - CSBI_ImageSetAni::Serialize (0xe7cd0), the frameless slot-1
// serialize the ANI conveyor SBI leaf shares with CSBI_StatzTabArrow (both vtables'
// slot 1 = thunk 0x2829 -> 0xe7cd0). RE-ATTRIBUTED here from SBI_WarlordHead.cpp,
// where it was mis-named CSBI_WarlordHead::Serialize: the vtable proof (gruntz sema
// class) shows CSBI_ImageSetAni/CSBI_StatzTabArrow slot 1 = 0x2829 -> 0xe7cd0, while
// CSBI_WarlordHead slot 1 = thunk 0x3cd8 -> 0xeb970 (that real one now lives in
// SBI_WarlordHead.cpp). The six persistent ints m_3c..m_50 belong to this class
// (size 0x54), not warlord (which serializes only its single m_3c direction).
#define SBI_DTOR_CHAIN // enable the inline base-dtor bodies (see StatusBarItem.h)
#include <rva.h>
#include <Mfc.h>
#include <Ints.h>
#include <Gruntz/SBI_ImageSetAni.h>
#include <Gruntz/GameRegistry.h> // canonical g_gameReg singleton (m_world liveness gate)

// The g_gameReg singleton (?g_gameReg@@3PAUWwdGameReg@@A @ VA 0x64556c).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

// vtable slot 1 (0xe7cd0): save/load the six persistent ints (m_3c..m_50) through the
// stream's Read/WriteBytes, then chain the CSBI_ImageSet base serialize and normalize
// its result to a bool. mode 7 = load, mode 4 = save; any other mode just chains.
// Bails early when the stream is null or the active game manager (g_gameReg->m_world)
// is gone.
RVA(0x000e7cd0, 0xf8)
i32 CSBI_ImageSetAni::Serialize(CImageSetStream* s, i32 mode, i32 a3, i32 a4) {
    if (s == 0) {
        return 0;
    }
    if (g_gameReg->m_world == 0) {
        return 0;
    }
    switch (mode) {
        case 7:
            s->ReadBytes(&m_3c, 4);
            s->ReadBytes(&m_40, 4);
            s->ReadBytes(&m_44, 4);
            s->ReadBytes(&m_48, 4);
            s->ReadBytes(&m_4c, 4);
            s->ReadBytes(&m_50, 4);
            break;
        case 4:
            s->WriteBytes(&m_3c, 4);
            s->WriteBytes(&m_40, 4);
            s->WriteBytes(&m_44, 4);
            s->WriteBytes(&m_48, 4);
            s->WriteBytes(&m_4c, 4);
            s->WriteBytes(&m_50, 4);
            break;
    }
    return CSBI_ImageSet::Serialize(s, mode, a3, a4) != 0; // qualified = direct base call
}

// ---------------------------------------------------------------------------
// ~CSBI_ImageSetAni (0x1047f0): the /GX chain destructor - stamp
// ??_7CSBI_ImageSetAni, run DtorImageSetAni (reloc-masked), then MSVC folds the
// four inline base dtors in (ImageSet/Image/RectOnly/StatusBarItem - the
// SBI_DTOR_CHAIN device) behind the /GX SEH frame. Collapsed from
// SBI_ImageSetAniEh.cpp (5-level case of
// docs/patterns/eh-dtor-multilevel-polymorphic-chain.md).
RVA(0x001047f0, 0x94)
CSBI_ImageSetAni::~CSBI_ImageSetAni() {
    DtorImageSetAni();
}
