#include <rva.h>

#include <Gruntz/StateDispatch.h>

#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/LevelTime.h>
#include <Gruntz/LogicRecordHandler.h>
#include <Gruntz/LogicTypeTableInline.h>

RVA_COMPGEN(0x00011a20, 0x1e, ??_GCLevelTime@@UAEPAXI@Z)
RVA_COMPGEN(0x00011a50, 0x44, ??1CLevelTime@@UAE@XZ)

RVA(0x0009b770, 0xf1)
i32 DispatchLevelTimeLogic(CGameObject* obj){TILE_LOGIC_RECORD_DISPATCH(CLevelTime)}

RVA(0x0009b8b0, 0x18f)
CLevelTime::CLevelTime(CGameObject* obj) : CUserLogic(obj, CUserLogic::INLINE_BASE), CWapX(obj) {
    SetObjectFlags(IDX(WWD_GAME_OBJECT_FLAG_KEEP_ACTIVE));
}
