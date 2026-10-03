#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/InGameText.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/ToyPeek.h>
#include <Wwd/LogicRecordEvent.h>

i32 DispatchInGameIconLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CInGameIcon)
}

i32 DispatchInGameTextLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CInGameText)
}

i32 DispatchToyPeekLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CToyPeek)
}
