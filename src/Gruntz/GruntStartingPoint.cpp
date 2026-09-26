#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/GruntStartingPoint.h>

#include <Bute/ButeMgr.h>
#include <Gruntz/ActNameRegistry.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/ActRegistry.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/TypeColl.h>
#include <Gruntz/TypeKeyColl.h>
#include <ZTools/BitVec.h>
#include <ZTools/ZDArray.h>

RVA_DYNINIT(0x0003e100, 0xa, CActRegPool<CGruntStartingPoint>::s_table)
RVA_DYNINIT(0x0003e120, 0x15, CActRegPool<CGruntStartingPoint>::s_table)
RVA_DYNINIT(0x0003e150, 0xe, CActRegPool<CGruntStartingPoint>::s_table)
RVA_DYNINIT(0x0003e170, 0x1f, CActRegPool<CGruntStartingPoint>::s_table)
template<> DATA(0x002446d8)
CActReg CActRegPool<CGruntStartingPoint>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

RVA_COMPGEN(0x00010640, 0x1e, ??_GCGruntStartingPoint@@UAEPAXI@Z)
RVA_COMPGEN(0x00010670, 0x44, ??1CGruntStartingPoint@@UAE@XZ)

RVA(0x0003df30, 0x161)
CGruntStartingPoint::CGruntStartingPoint(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_EXIT");
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    Hide();
}

RVA(0x0003e1a0, 0x102)
void CGruntStartingPoint::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

RVA(0x0003e300, 0x18d)
void RegisterGruntStartingPointActions() {
    ACT_NAME_ID(id, "A")

    CActRegPool<CGruntStartingPoint>::s_table[id] =
        static_cast<CActHandler>(&CGruntStartingPoint::Idle);
}

RVA(0x0003e500, 0x3)
i32 CGruntStartingPoint::Idle() {
    return 0;
}
