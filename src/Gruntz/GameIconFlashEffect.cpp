#include <rva.h>

#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniAdvanceCursorInline.h>
#include <Gruntz/GameIconFlashState.h>
#include <Gruntz/UserLogic.h>
#include <Rez/FrameClock.h>

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x000ae360, 0x6f)
i32 GameIconFlashEffect(CGameObject* obj) {
    CLogicRecord* record = obj->m_logicRecord;
    GameIconFlashState state = static_cast<GameIconFlashState>(record->EventCode());
    if (state != GAME_ICON_FLASH_IDLE) {
        if (state == GAME_ICON_FLASH_ACTIVE) {
            CAniAdvanceCursor* a = &static_cast<CWwdSpriteObject*>(obj)->m_animationCursor;
            a->Advance(g_engineFrameDelta);
            if (IsAniCursorComplete(a)) {
                obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_PENDING_DELETE);
                return 1;
            }
        }
        return 1;
    }
    obj->m_flags |= IDX(WWD_GAME_OBJECT_FLAG_SKIP_COLLISION);
    static_cast<CWwdSpriteObject*>(obj)->SetAnimationByName("GAME_ICONFLASH", 0);
    record->SetEventCode(IDX(GAME_ICON_FLASH_ACTIVE));
    return 1;
}
