#include <rva.h>

#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/GameIconFlashState.h>
#include <Gruntz/UserLogic.h>
#include <Rez/FrameClock.h>

RVA(0x000ae360, 0x6f)
i32 GameIconFlashEffect(CGameObject* obj) {
    AnimWorkerObj* w = obj->m_animWorker;
    GameIconFlashState state = static_cast<GameIconFlashState>(w->ActKey());
    if (state != GAME_ICON_FLASH_IDLE) {
        if (state == GAME_ICON_FLASH_ACTIVE) {
            CAniAdvanceCursor* a = &static_cast<CWwdGameObjectA*>(obj)->m_animCursor;
            a->Advance(g_engineFrameDelta);
            if (a->m_finished != 0 && a->m_frameTicksLeft == 0) {
                obj->m_flags |= 0x10000;
                return 1;
            }
        }
        return 1;
    }
    obj->m_flags |= 1;
    static_cast<CWwdGameObjectA*>(obj)->ApplyLookupGeometry("GAME_ICONFLASH", 0);
    w->SetActKey(IDX(GAME_ICON_FLASH_ACTIVE));
    return 1;
}
