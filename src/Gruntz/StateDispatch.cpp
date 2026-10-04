#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/StateDispatch.h>

#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LevelTime.h>
#include <Gruntz/LogicRecordHandler.h>

i32 DispatchLevelTimeLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CLevelTime)
}

CLevelTime::CLevelTime(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
}
