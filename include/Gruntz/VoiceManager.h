#ifndef GRUNTZ_VOICEMANAGER_H
#define GRUNTZ_VOICEMANAGER_H

#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SpawnList.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>

class CGruntVoice;
struct StreamVoice;

class CGruntzMgr;
class CDDrawSurfaceMgr;

enum {
    VOICE_CUES_PER_BAND = 20
};

class CGrunt;

class CVoiceManager {
public:
    CVoiceManager() {
        m_game = NULL;
        m_world = NULL;
        memset(m_indicators, 0, sizeof(m_indicators));
        memset(m_streamVoices, 0, sizeof(m_streamVoices));
    }

    BOOL Init(CGruntzMgr* game);
    void Clear();
    BOOL CreateVoiceIndicators();
    void ClearVoiceIndicatorSlots();
    i32 ResolveGruntVoiceGroup(CGrunt* grunt, i32 cueId);

    struct CParseSource* SelectVoiceVariant(i32 voiceGroup, i32 variantIndex);
    struct CParseSource* SelectVoiceVariant(i32 voiceGroup);
    BOOL BuildVoiceGroups();

    BOOL
    PlayGruntVoiceCue(class CGrunt* grunt, i32 cueId, i32 variantIndex, i32 priority, i32 percent);

    i32 PlayVoice(
        CGrunt* sourceGrunt,
        i32 voiceGroup,
        i32 variantIndex,
        i32 unpositioned,
        i32 priority,
        i32 percent
    );

    i32 PlayVoice(i32 sourceObjectId, i32 voiceGroup, i32 variantIndex, i32 priority, i32 percent);
    CSpawnList* BuildVoiceGroup(i32 voiceGroup);
    i32 IsAnyVoicePlaying();
    i32 IsVoiceSlotPlaying(i32 slotIndex);
    void StopVoice(i32 sourceObjectId);
    void PauseAllVoices();
    void ResetVoiceSelections();
    BOOL IsVoiceEnabled();
    ~CVoiceManager();

    CGruntzMgr* m_game;

    CDDrawSurfaceMgr* m_world;
    CGruntVoice* m_indicators[2];

    StreamVoice* m_streamVoices[2];

    CPtrArray m_voiceGroups;
    i32 m_voiceVolume;
};

#endif // GRUNTZ_VOICEMANAGER_H
