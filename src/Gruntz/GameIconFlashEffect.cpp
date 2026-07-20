#include <Gruntz/UserLogic.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <rva.h>

extern "C" u32 g_engineFrameDelta;

RVA(0x000ae360, 0x6f)
i32 GameIconFlashEffect(CGameObject* obj) {
    AnimWorkerObj* w = obj->m_7c;
    i32 state = reinterpret_cast<i32>(w->m_1c);
    if (state != 0) {
        if (state == 5) {
            CAniAdvanceCursor* a = &static_cast<CWwdGameObjectA*>(obj)->m_1a0; // the handed obj IS the A-kind sprite
            a->Advance(g_engineFrameDelta);
            if (a->m_28 != 0 && a->m_20 == 0) {
                obj->m_flags |= 0x10000;
                return 1;
            }
        }
        return 1;
    }
    obj->m_flags |= 1;
    static_cast<CWwdGameObjectA*>(obj)->ApplyLookupGeometry("GAME_ICONFLASH", 0);
    w->m_1c = reinterpret_cast<void*>(5);
    return 1;
}
