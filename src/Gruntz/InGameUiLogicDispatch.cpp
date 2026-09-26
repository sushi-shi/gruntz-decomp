#include <StdAfx.h>

#include <rva.h>

#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/InGameIcon.h>
#include <Gruntz/InGameText.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/ToyPeek.h>
#include <Wwd/LogicRecordEvent.h>

RVA(0x00095750, 0xf4)
i32 DispatchInGameIconLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CInGameIcon)
}

RVA(0x00095890, 0xf1)
i32 DispatchInGameTextLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CInGameText)
}

RVA(0x000959d0, 0xf1)
i32 DispatchToyPeekLogic(CGameObject* owner) {
    LOGIC_RECORD_DISPATCH(CToyPeek)
}
