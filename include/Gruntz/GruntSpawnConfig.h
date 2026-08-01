#ifndef GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H
#define GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H

#include <rva.h>

#include <Ints.h>
#include <Mfc.h>

#include <Gruntz/GameRegistry.h>
#include <Gruntz/SpawnList.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/UserLogic.h>
#include <Rez/RezAlloc.h>

class CGruntVoice;
struct StreamVoice;

class CGruntzMgr;
class CDDrawSurfaceMgr;

enum {
    VOICE_CUES_PER_BAND = 20
};

class CGrunt;

class CGruntSpawnConfig {
public:
    BOOL Init(CGruntzMgr* owner);
    void Clear();
    BOOL LoadGruntVoices();
    void ClearSprites();
    i32 GetButeSlot(CGrunt* who, i32 cue);

    struct CParseSource* PickWeighted(i32 voiceId, i32 which);
    BOOL BuildVoiceList();

    BOOL LoadGruntSpawnConfig(class CGrunt* who, i32 cue, i32 which, i32 priority, i32 percent);

    i32 SpawnVoiceDriver(CGrunt* who, i32 voiceId, i32 which, i32 objId, i32 priority, i32 percent);

    i32 SpawnVoiceDriver(i32 objId, i32 voiceId, i32 which, i32 priority, i32 percent);
    CSpawnList* BuildVoiceSoundList(i32 i);
    i32 AnyVoicePlaying();
    i32 VoicePlaying(i32 i);
    void StopVoice(i32 id);
    void PauseAllVoices();
    void Stop();
    void ResetPicks();
    BOOL IsReady();
    ~CGruntSpawnConfig();

    CGruntzMgr* m_owner;

    CDDrawSurfaceMgr* m_configTree;
    CGruntVoice* m_voices[2];

    StreamVoice* m_streams[2];

    CPtrArray m_voiceLists;
    i32 m_voiceVolume;
};
SIZE_UNKNOWN();

extern "C" i32 SpawnResolveName(void* resolver, void* nameStr, i32 mode);

#endif // GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H
