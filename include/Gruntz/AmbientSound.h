#ifndef GRUNTZ_CAMBIENTSOUND_H
#define GRUNTZ_CAMBIENTSOUND_H

#include <rva.h>

#include <Enums.h>

GZ_ENUM_CONST_BEGIN(AmbientSoundActState)
    AMBIENT_SOUND_ACTIVE = 0x1e
GZ_ENUM_CONST_END(AmbientSoundActState)

#include <Mfc.h>

#include <Dsndmgr/DirectSoundMgr.h>
#include <Enums.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>

struct AmbientPoint {
    i32 x;
    i32 y;
};

struct CDDrawSubMgrLeafScan;

class CAmbientSound : public CUserBase {
public:
    CAmbientSound() {
        m_voice = NULL;
        m_level = 0x64;
        m_isPlaying = 0;
        m_listNode = NULL;
    }

    virtual ~CAmbientSound() OVERRIDE {
        m_voice = NULL;
        m_listNode = NULL;
    }

    i32 SetLevel(i32 value, i32 mode, i32 extra);

    virtual void Update(i32 x, i32 y, i32 force);

    void Fade(i32 playFlag, i32 level, i32 mode);

    void Restart();

    void Recompute(i32 master);

    i32 InitFromKey(
        CDDrawSubMgrLeafScan* world,
        const char* key,
        i32 level,
        i32 master,
        RECT* box,
        i32 scaleB
    );
    i32 InitFromSound(DirectSoundMgr* mgr, i32 level, i32 master, RECT* box, i32 scaleB);

    DirectSoundMgr* m_voice;
    i32 m_level;
    i32 m_scaleA;
    i32 m_scaleB;
    i32 m_isPlaying;
    RECT m_box1;
    RECT m_box2;
    i32 m_panIndex;
    POSITION m_listNode;
};

class CAmbientPosSound : public CAmbientSound {
public:
    CAmbientPosSound() {}

    virtual ~CAmbientPosSound() OVERRIDE {}
    virtual void Update(i32 x, i32 y, i32 force) OVERRIDE;

    i32 InitFromKey(
        CDDrawSubMgrLeafScan* world,
        const char* key,
        i32 level,
        i32 master,
        AmbientPoint* pos,
        i32 scaleB
    );
    i32 InitFromSound(DirectSoundMgr* mgr, i32 level, i32 master, AmbientPoint* pos, i32 scaleB);

    AmbientPoint m_position;
};

#endif // GRUNTZ_CAMBIENTSOUND_H
