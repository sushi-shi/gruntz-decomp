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
#include <Ints.h>

class CLightFx;

class SoundBuffer;

class CProjectile : public CMovingLogic, public CWapX {
public:
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*) OVERRIDE;
    RVA(0x00012970, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_PROJECTILE;
    }
    CProjectile();
    CProjectile(CUserLogic::EInlineBase) {}
    CProjectile(CGameObject* owner);
    virtual ~CProjectile() OVERRIDE;

    virtual i32 LoadProjectileSprites(
        PickupType kind,
        i32 sourcePlayerIndex,
        i32 sourceUnitIndex,
        i32 targetPxX,
        i32 targetPxY,
        i32 sourcePxX,
        i32 sourcePxY
    );

    virtual void FireActivation(i32 id) OVERRIDE;
    static void RegisterType();

    i32 AdvanceAnimationAndDeleteWhenComplete();
    void ScanTargets(i32 impact);
    i32 LaunchSound(const char* key);
    virtual void AdvanceMotion() OVERRIDE;

    PickupType m_kind;
    i32 m_sourcePlayerIndex, m_sourceUnitIndex;
    i32 m_targetPxX, m_targetPxY;
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
    b32 m_isArcing;
    b32 m_arrived;

    enum {
        PF_IMPACT = 5,
        PF_FALL = 6
    };
    CAniElement* m_frames[7];
    CWwdSpriteObject* m_shadow;
    SoundBuffer* m_sound;
    CPtrList m_hitList;
    i32 m_sourcePxX, m_sourcePxY;
};

#endif // GRUNTZ_PROJECTILE_H
