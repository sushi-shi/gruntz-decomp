#include <Mfc.h>
#include <Gruntz/MovingLogic.h>
#include <rva.h>
#include <Gruntz/ActReg.h>
#include <Gruntz/HaznColl.h>
#ifndef GRUNTZ_PROJECTILE_H
#define GRUNTZ_PROJECTILE_H

class CLightFx;

class DirectSoundMgr;

class CProjectile : public CMovingLogic, public CWapX {
public:
    virtual i32 SerializeMove(CFileMemBase*, i32, i32, CGameObject*) OVERRIDE;
    RVA(0x00012960, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PROJECTILE;
    }
    CProjectile();
    CProjectile(CGameObject* owner);
    virtual ~CProjectile() OVERRIDE;

    virtual i32 LoadProjectileSprites(i32 kind, i32 a, i32 b, i32 sx, i32 sy, i32 t0, i32 t1);

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterType();

    i32 DetachRenderObj();
    void ScanTargets(i32 impact);
    i32 LaunchSound(const char* key);
    virtual void AdvanceMotion() OVERRIDE;

    i32 m_kind;
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

extern const double g_movingLogicMax;

extern const double g_projPhase1;
#endif // GRUNTZ_PROJECTILE_H
