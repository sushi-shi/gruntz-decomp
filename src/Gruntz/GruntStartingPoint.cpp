#include <StdAfx.h>

#include <Ints.h>

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

template<>
CActReg CActRegPool<CGruntStartingPoint>::s_table(ACT_ID_FIRST, ACT_ID_LAST);

CGruntStartingPoint::CGruntStartingPoint(CGameObject* obj)
    : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetImageSetByName("GAME_EXIT");
    SET_ANIMATION_ACT("A");
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION));
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
    Hide();
}

void CGruntStartingPoint::FireActivation(i32 coord) {
    DispatchRegisteredAct(this, coord);
}

void RegisterGruntStartingPointActions() {
    ACT_NAME_ID(id, "A")

    CActRegPool<CGruntStartingPoint>::s_table[id] =
        static_cast<CActHandler>(&CGruntStartingPoint::Idle);
}

i32 CGruntStartingPoint::Idle() {
    return 0;
}
