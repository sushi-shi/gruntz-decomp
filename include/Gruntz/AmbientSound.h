#ifndef GRUNTZ_CAMBIENTSOUND_H
#define GRUNTZ_CAMBIENTSOUND_H

#include <rva.h>

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
SIZE_UNKNOWN();

struct CRandomAmbientWorld;

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
        CRandomAmbientWorld* world,
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
SIZE(0x40);

class CAmbientPosSound : public CAmbientSound {
public:
    CAmbientPosSound() {}

    virtual ~CAmbientPosSound() OVERRIDE {}
    virtual void Update(i32 x, i32 y, i32 force) OVERRIDE;

    i32 InitFromKey(
        CRandomAmbientWorld* world,
        const char* key,
        i32 level,
        i32 master,
        AmbientPoint* pos,
        i32 scaleB
    );
    i32 InitFromSound(DirectSoundMgr* mgr, i32 level, i32 master, AmbientPoint* pos, i32 scaleB);

    AmbientPoint m_position;
};
SIZE(0x48);

#endif // GRUNTZ_CAMBIENTSOUND_H
