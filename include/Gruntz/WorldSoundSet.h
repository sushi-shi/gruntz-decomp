#ifndef GRUNTZ_CWORLDSOUNDSET_H
#define GRUNTZ_CWORLDSOUNDSET_H

#include <Mfc.h>
#include <rva.h>

#include <Dsndmgr/SoundDevice.h>

class CAmbientSound;

class CAmbientPosSound;
class CRandomAmbientSound;
struct AmbientPoint;

struct CRandomAmbientWorld {
    char m_pad00[0x10];
    CMapStringToPtr m_map;
    SoundDevice* m_soundDev;
};
SIZE_UNKNOWN();

enum {
    kSoundVolumeMax = 100
};

class CWorldSoundSet {
public:
    i32 Init(void* world, i32 volume);
    void Teardown();
    void Restart(i32 volume);
    void Stop();
    void Resume();
    void Retune(i32 x, i32 y);
    void Deactivate();
    CWorldSoundSet();
    ~CWorldSoundSet();

    CAmbientSound* CreateAmbient6(const char* key, i32 level, RECT* box, i32 scaleB, i32 unused);
    CAmbientSound*
    CreateAmbient5(DirectSoundMgr* mgr, i32 level, RECT* box, i32 scaleB, i32 unused);
    CAmbientPosSound*
    CreatePos6(const char* key, i32 level, AmbientPoint* pos, i32 scaleB, i32 unused);
    CAmbientPosSound*
    CreatePos5(DirectSoundMgr* mgr, i32 level, AmbientPoint* pos, i32 scaleB, i32 unused);

    CRandomAmbientSound* CreateRandom(
        DirectSoundMgr* mgr,
        i32 level,
        RECT* box,
        i32 scaleB,
        i32 intervalLoA,
        i32 intervalHiA,
        i32 intervalLoB,
        i32 intervalHiB,
        i32 unused
    );

    CRandomAmbientSound* CreateRandomBox(
        const char* key,
        i32 level,
        RECT* box,
        i32 scaleB,
        i32 intervalLoA,
        i32 intervalHiA,
        i32 intervalLoB,
        i32 intervalHiB,
        i32 unused
    );

    CRandomAmbientWorld* m_world;
    i32 m_volume;
    CPtrList m_list;
    i32 m_active;

    i32 m_listenerX;
    i32 m_listenerY;
};
SIZE_UNKNOWN();

inline CWorldSoundSet::CWorldSoundSet() : m_list(0xa) {
    m_world = 0;
    m_volume = kSoundVolumeMax;
}

extern "C" void DefaultActionHandler_2d15();

extern i32 g_posSoundReq;

struct PosSoundObj;

i32 SpawnPosSound(PosSoundObj* obj);

#endif // GRUNTZ_CWORLDSOUNDSET_H
