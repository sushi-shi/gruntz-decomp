#ifndef GRUNTZ_PROJECTILE_H
#define GRUNTZ_PROJECTILE_H

#include <string>

#include <list>
struct Coord;

#include <Ints.h>

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
    virtual i32 SerializeDispatch(CFileMemBase*, SerialMode, LogicTypeId, CGameObject*)  ;

    virtual LogicTypeId GetTypeTag()   {
        return LOGIC_PROJECTILE;
    }
    CProjectile();
    CProjectile(CUserLogic::EInlineBase) {}
    CProjectile(CGameObject* owner);
    virtual ~CProjectile()  ;

    virtual i32 LoadProjectileSprites(
        PickupType kind,
        i32 sourcePlayerIndex,
        i32 sourceUnitIndex,
        i32 targetPxX,
        i32 targetPxY,
        i32 sourcePxX,
        i32 sourcePxY
    );

    virtual void FireActivation(i32 id)  ;
    static void RegisterType();

    i32 AdvanceAnimationAndDeleteWhenComplete();
    void ScanTargets(i32 impact);
    i32 LaunchSound(const std::string& key);
    virtual void AdvanceMotion()  ;

    PickupType m_kind;
    i32 m_sourcePlayerIndex, m_sourceUnitIndex;
    i32 m_targetPxX, m_targetPxY;
    double m_flightDist;
    u32 m_timePerTile;
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
    std::list<Coord*> m_hitList;
    i32 m_sourcePxX, m_sourcePxY;
};

#endif
