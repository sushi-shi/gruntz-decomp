#include <rva.h>

#include <Gruntz/GruntStartingPoint.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/ButeTree.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TypeColl.h>
#include <Gruntz/TypeKeyColl.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

RVA_DYNINIT(0x0003e020, 0xa, CActRegPool<CGruntStartingPoint>::s_table)
RVA_DYNINIT(0x0003e040, 0x15, CActRegPool<CGruntStartingPoint>::s_table)
RVA_DYNINIT(0x0003e070, 0xe, CActRegPool<CGruntStartingPoint>::s_table)
RVA_DYNINIT(0x0003e090, 0x1f, CActRegPool<CGruntStartingPoint>::s_table)
template<> DATA(0x00245630)
CActReg CActRegPool<CGruntStartingPoint>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00010650, 0x1e, ??_GCGruntStartingPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x00010680, 0x44, ??1CGruntStartingPoint@@UAE@XZ)

RVA(0x0003de50, 0x161)
CGruntStartingPoint::CGruntStartingPoint(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_EXIT");
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    Hide();
}

static inline CActHandler* R4Lookup(i32 coord) {
    return (CActRegPool<CGruntStartingPoint>::s_table.ResolveEntry(coord));
}

RVA(0x0003e0c0, 0x102)
void CGruntStartingPoint::FireActivation(i32 coord) {
    CActHandler* e = R4Lookup(coord);
    if ((*e) != NULL) {
        CActHandler* e2 = R4Lookup(coord);
        (this->*((*e2)))();
    }
}

RVA(0x0003e220, 0x18d)
void ActReg4RegisterType() {
    ACT_NAME_ID(id, "A")

    *R4Lookup(id) = static_cast<CActHandler>(&CGruntStartingPoint::Idle);
}

RVA(0x0003e420, 0x3)
i32 CGruntStartingPoint::Idle() {
    return 0;
}
