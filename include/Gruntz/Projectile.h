#ifndef GRUNTZ_PROJECTILE_H
#define GRUNTZ_PROJECTILE_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/ActReg.h>
#include <Gruntz/HaznColl.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MovingLogic.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SerialArchive.h>

class CLightFx;

class DirectSoundMgr;

class CProjectile : public CMovingLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00012960, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PROJECTILE;
    }
    // Two entities, same tag types.  The out-of-line 0x126e0 expands the whole
    // CMovingLogic chain (one `call ??0CUserBaseLink` + the CPtrList member) and
    // CBoomerang `call`s it; the inline sibling leaves the base a
    // `call ??0CMovingLogic`, which is what `new CProjectile` expands.
    CProjectile();
    CProjectile(CUserLogic::EInlineBase) {}
    CProjectile(CGameObject* owner);
    virtual ~CProjectile() OVERRIDE;

    virtual i32
    LoadProjectileSprites(PickupType kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterType();

    i32 DetachRenderObj();
    void ScanTargets(i32 impact);
    i32 LaunchSound(const char* key);
    virtual void AdvanceMotion() OVERRIDE;

    PickupType m_kind;
    i32 m_srcRow, m_srcCol;
    i32 m_targetX, m_targetY;
    double m_flightDist;
    i32 m_timePerTile;
    double m_velScale;
    double m_posX;
    double m_posY;
    double m_velX;
    double m_velY;
    double m_roundX;
    double m_roundY;
    i32 m_curX, m_curY;
    i32 m_isArcing;
    i32 m_arrived;

    enum {
        PF_IMPACT = 5,
        PF_FALL = 6
    };
    CAniElement* m_frames[7];
    CWwdGameObjectA* m_shadow;
    DirectSoundMgr* m_sound;
    CPtrList m_hitList;
    i32 m_targetId, m_ownerId;
};

SIZE_UNKNOWN();

extern const double g_projPhase1;
#endif // GRUNTZ_PROJECTILE_H
