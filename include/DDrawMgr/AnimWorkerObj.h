#ifndef GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H
#define GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H

#include <Ints.h>
#include <Gruntz/Loadable.h>
#include <rva.h>

class CUserLogic;
class CFileMemBase;

struct CGameObject;

typedef i32(__cdecl* GameObjNotifyFn)(CGameObject* obj);

class CDDrawSurfaceMgr;

struct AnimWorkerObj : public CLoadable {

    virtual ~AnimWorkerObj() OVERRIDE;
    virtual i32 IsLoaded() OVERRIDE;

    virtual void Unload() OVERRIDE;
    virtual i32 GetClassId() OVERRIDE;

    virtual i32 Init(GameObjNotifyFn callback, i32 frame);

    AnimWorkerObj() {}

    AnimWorkerObj(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags);

    AnimWorkerObj(CDDrawSurfaceMgr* owner, i32 id) : CLoadable(owner, id) {
        m_notify = 0;
        m_payload = 0;
        m_logic = 0;
        m_target = 0;
        m_1c = 0;
        m_targetId = 0;
        m_payloadSize = 0;
    }

    i32 Consume(i32 amount);

    i32 Dispatch(CFileMemBase* a, i32 mode, i32 c, void* d);
    i32 CacheTargetId(void* a);
    i32 Save(CFileMemBase* ar);
    i32 Load(CFileMemBase* ar);

    i32 ResolveTarget(void* a);

    GameObjNotifyFn m_notify;
    u8* m_payload;
    CUserLogic* m_logic;

    i32 ActKey() const {
        return m_1c;
    }
    void SetActKey(i32 id) {
        m_1c = id;
    }
    i32 m_1c;

    i32 m_timeDelay;
    i32 m_frameDelay;
    i32 m_userFlags;
    i32 m_minX;
    i32 m_maxX;
    i32 m_minY;
    i32 m_maxY;
    char m_pad3c[0x40 - 0x3c];
    i32 m_40;
    i32 m_tweakX;
    i32 m_tweakY;
    i32 m_scrollTargetX;
    i32 m_scrollTargetY;
    char m_pad54[0x58 - 0x54];
    i32 m_58;
    i32 m_5c;
    i32 m_60;
    i32 m_user1;
    i32 m_user2;
    i32 m_user3;
    i32 m_user4;
    i32 m_user5;
    i32 m_user6;
    i32 m_user7;
    i32 m_user8;
    i32 m_84;
    i32 m_88;
    i32 m_8c;
    i32 m_90;
    i32 m_94;
    i32 m_98;
    i32 m_9c;
    i32 m_a0;
    i32 m_a4;
    i32 m_a8;
    i32 m_ac;
    i32 m_b0;
    i32 m_b4;
    i32 m_counter;
    i32 m_speed;

    char m_padc0[0xc4 - 0xc0];
    i32 m_c4;
    i32 m_width;
    i32 m_height;
    RECT m_d0;
    RECT m_e0;

    RECT m_switchRectA;
    RECT m_switchRectB;
    char m_pad110[0x120 - 0x110];
    RECT m_120;
    i32 m_130;
    char m_pad134[0x138 - 0x134];
    i32 m_138;
    i32 m_13c;
    i32 m_140;
    i32 m_144;
    i32 m_148;
    i32 m_14c;
    i32 m_150;
    i32 m_154;
    i32 m_158;
    i32 m_15c;
    i32 m_160;
    i32 m_164;
    i32 m_168;
    i32 m_16c;
    CGameObject* m_target;
    i32 m_targetId;
    u32 m_payloadSize;
};
SIZE(0x17c);

#ifndef ANIMWORKEROBJ_OOL_CTOR
inline AnimWorkerObj::AnimWorkerObj(CDDrawSurfaceMgr* owner, i32 id, i32 stateFlags)
    : CLoadable(id, stateFlags, owner) {
    m_notify = 0;
    m_payload = 0;
    m_logic = 0;
    m_target = 0;
    m_1c = 0;
    m_targetId = 0;
    m_payloadSize = 0;
}
#endif
VTBL(AnimWorkerObj, 0x001efb80);

#endif // GRUNTZ_DDRAWMGR_ANIMWORKEROBJ_H
