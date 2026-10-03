#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/VoiceManager.h>

#include <Bute/ButeMgr.h>
#include <Dsndmgr/SoundStream.h>
#include <Dsndmgr/StreamFeeder.h>
#include <Dsndmgr/StreamVoice.h>
#include <Enums.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntVoice.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/SpawnList.h>
#include <Gruntz/SpawnListInline.h>
#include <Rez/RezArchive.h>
#include <Rez/RezTypeTag.h>

#include <dsound.h>

CVoiceManager::~CVoiceManager() {
    Clear();
}

BOOL CVoiceManager::Init(CGruntzMgr* game) {
    if (game == NULL) {
        return false;
    }
    m_world = NULL;
    CLEAR_VOICE_INDICATORS;
    memset(m_streamVoices, 0, sizeof(m_streamVoices));
    m_game = game;
    m_world = game->m_world;
    m_voiceVolume = 0x64;
    return BuildVoiceGroups() != false;
}

void CVoiceManager::Clear() {
    for (i32 i = 0; i < m_voiceGroups.GetSize(); i++) {
        CSpawnList* group = static_cast<CSpawnList*>(m_voiceGroups[i]);

        delete group;
    }
    m_voiceGroups.RemoveAll();
    if (m_world != NULL && m_world->GetSoundStream() != NULL) {
        StreamVoice** stream = m_streamVoices;
        for (i32 k = 0; k < 2; k++) {
            if (stream[0] != NULL) {
                m_world->GetSoundStream()->DestroyVoice(stream[0]);
                stream[0] = NULL;
            }
            stream++;
        }
    }
    m_game = NULL;
    m_world = NULL;
    CLEAR_VOICE_INDICATORS;
    memset(m_streamVoices, 0, sizeof(m_streamVoices));
}

BOOL CVoiceManager::CreateVoiceIndicators() {
    ClearVoiceIndicatorSlots();
    i32 i = 0;
    CGruntVoice** slot = m_indicators;
    for (; i < 2; i++, slot++) {
        CGameObject* spr = m_world->ChildGroup()->CreateSprite(
            0,
            0,
            0,
            0xdbba1,
            "GruntVoice",
            WWD_GAME_OBJECT_FLAGS_SKIP_ACTIVE_WORLD_SPRITE
        );
        spr->m_logicRecord->m_dispatch(spr);
        CGruntVoice* got = static_cast<CGruntVoice*>(spr->m_logicRecord->m_userLogic);
        *slot = got;
        if (got == NULL) {
            return false;
        }
    }
    return true;
}

void CVoiceManager::ClearVoiceIndicatorSlots() {
    CLEAR_VOICE_INDICATORS;
}

BOOL CVoiceManager::PlayGruntVoiceCue(
    CGrunt* grunt,
    i32 cueId,
    i32 variantIndex,
    i32 priority,
    i32 percent
) {
    if (m_indicators[0] == NULL && !CreateVoiceIndicators()) {
        return false;
    }
    if (grunt == NULL) {
        return false;
    }
    if (!IsVoiceEnabled()) {
        return false;
    }
    i32 voiceGroup = ResolveGruntVoiceGroup(grunt, cueId);
    CString voiceSection;
    CString cueKey;
    voiceSection.Format("SG%i", voiceGroup);
    cueKey.Format("G%i", cueId);
    if (percent == -1) {
        percent = g_buteMgr.GetInt(static_cast<LPCTSTR>(voiceSection), "Per", -1);
        if (percent == -1) {
            percent = g_buteMgr.GetInt("GruntPercent", static_cast<LPCTSTR>(cueKey), 0);
        }
    }
    if (percent < 100 && g_gameReg->Rand() % 0x65 > percent) {
        return false;
    }
    if (priority == -1) {
        priority = g_buteMgr.GetInt(static_cast<LPCTSTR>(voiceSection), "Pri", -1);
        if (priority == -1) {
            priority = g_buteMgr.GetInt("GruntPriority", static_cast<LPCTSTR>(cueKey), 1);
        }
    }
    for (i32 i = 0; i < 2; i++) {
        if (m_indicators[i]->m_priority >= priority) {
            return false;
        }
    }
    CRezItm* source = SelectVoiceVariant(voiceGroup, variantIndex);
    if (source == NULL || m_world->GetSoundStream() == NULL) {
        return false;
    }
    CGruntVoice* firstIndicator = m_indicators[0];
    CGruntVoice* secondIndicator = m_indicators[1];
    i32 firstPriority = firstIndicator->m_priority;
    i32 secondPriority = secondIndicator->m_priority;
    i32 firstSourceObjectId = firstIndicator->m_sourceObjectId;
    i32 secondSourceObjectId = secondIndicator->m_sourceObjectId;
    i32 slotIndex;
    if (firstPriority <= secondPriority) {
        slotIndex = 0;
        if (secondSourceObjectId == grunt->m_object->m_objectId) {
            slotIndex = 1;
            if (firstPriority != 0 && m_streamVoices[0] != NULL) {
                m_streamVoices[0]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
            }
        } else if (secondPriority != 0 && m_streamVoices[1] != NULL) {
            m_streamVoices[1]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
        }
    } else {
        slotIndex = 1;
        if (firstSourceObjectId == grunt->m_object->m_objectId) {
            slotIndex = 0;
            if (secondPriority != 0 && m_streamVoices[1] != NULL) {
                m_streamVoices[1]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
            }
        } else if (firstPriority != 0 && m_streamVoices[0] != NULL) {
            m_streamVoices[0]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
        }
    }
    if (m_streamVoices[slotIndex] == NULL) {
        m_streamVoices[slotIndex] = m_world->GetSoundStream()->OpenStream(
            source,
            0x5000,
            0x1400,
            DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLDEFAULT,
            0,
            0
        );
        if (m_streamVoices[slotIndex] == NULL) {
            return false;
        }
    }
    if (m_streamVoices[slotIndex]->PlaySource(source, m_voiceVolume, 0, 0, false) != 0) {
        if (m_indicators[slotIndex]->BeginPlayback(
                grunt->m_object->m_objectId,
                m_streamVoices[slotIndex],
                priority,
                VOICE_INDICATOR_AT_LOGIC_OBJECT
            )) {
            return true;
        } else {
            return false;
        }
    }
    return false;
}

i32 CVoiceManager::PlayVoice(
    CGrunt* sourceGrunt,
    i32 voiceGroup,
    i32 variantIndex,
    i32 unpositioned,
    i32 priority,
    i32 percent
) {
    CGruntVoice** indicators = m_indicators;
    if (indicators[0] == NULL && !CreateVoiceIndicators()) {
        return 0;
    }
    if (sourceGrunt == NULL && unpositioned == 0) {
        return 0;
    }
    if (!IsVoiceEnabled()) {
        return 0;
    }
    CString voiceSection;
    voiceSection.Format("SG%i", voiceGroup);
    if (percent == -1) {
        percent = g_buteMgr.GetInt(static_cast<LPCTSTR>(voiceSection), "Per", 100);
    }
    if (percent < 100 && GetRandomNumber() % 0x65 > percent) {
        return 0;
    }
    if (priority == -1) {
        priority = g_buteMgr.GetInt(static_cast<LPCTSTR>(voiceSection), "Pri", 1);
    }
    for (i32 i = 0; i < 2; i++) {
        if (indicators[i]->m_priority >= priority) {
            return 0;
        }
    }
    i32 sourceObjectId = 0;
    if (unpositioned == 0 && sourceGrunt != NULL) {
        sourceObjectId = sourceGrunt->m_object->m_objectId;
    }
    CRezItm* source = SelectVoiceVariant(voiceGroup, variantIndex);
    if (source == NULL) {
        return 0;
    }
    if (m_world->GetSoundStream() == NULL) {
        return 0;
    }
    CGruntVoice* firstIndicator = indicators[0];
    CGruntVoice* secondIndicator = m_indicators[1];
    i32 firstPriority = firstIndicator->m_priority;
    i32 secondPriority = secondIndicator->m_priority;
    i32 firstSourceObjectId = firstIndicator->m_sourceObjectId;
    i32 secondSourceObjectId = secondIndicator->m_sourceObjectId;
    i32 slotIndex;
    if (firstPriority <= secondPriority) {
        slotIndex = 0;
        if (secondSourceObjectId == sourceObjectId) {
            slotIndex = 1;
            if (firstPriority != 0 && m_streamVoices[0] != NULL) {
                m_streamVoices[0]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
            }
        } else if (secondPriority != 0 && sourceObjectId != 0) {
            m_streamVoices[1]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
        }
    } else {
        slotIndex = 1;
        if (firstSourceObjectId == sourceObjectId) {
            slotIndex = 0;
            if (secondPriority != 0 && m_streamVoices[1] != NULL) {
                m_streamVoices[1]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
            }
        } else if (firstPriority != 0 && m_streamVoices[0] != NULL) {
            m_streamVoices[0]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
        }
    }
    if (m_streamVoices[slotIndex] == NULL) {
        m_streamVoices[slotIndex] = m_world->GetSoundStream()->OpenStream(
            source,
            0x5000,
            0x1400,
            DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLDEFAULT,
            0,
            0
        );
        if (m_streamVoices[slotIndex] == NULL) {
            return 0;
        }
    }
    if (m_streamVoices[slotIndex]->PlaySource(source, m_voiceVolume, 0, 0, false) == 0) {
        return 0;
    }
    if (m_indicators[slotIndex]->BeginPlayback(
            sourceObjectId,
            m_streamVoices[slotIndex],
            priority,
            VOICE_INDICATOR_AT_LOGIC_OBJECT
        )
        == 0) {
        return 0;
    }
    return 1;
}

i32 CVoiceManager::PlayVoice(
    i32 sourceObjectId,
    i32 voiceGroup,
    i32 variantIndex,
    i32 priority,
    i32 percent
) {
    CGruntVoice** indicators = m_indicators;
    if (indicators[0] == NULL && !CreateVoiceIndicators()) {
        return 0;
    }
    if (sourceObjectId == 0) {
        return 0;
    }
    if (!IsVoiceEnabled()) {
        return 0;
    }
    CString voiceSection;
    voiceSection.Format("SG%i", voiceGroup);
    if (percent == -1) {
        percent = g_buteMgr.GetInt(static_cast<LPCTSTR>(voiceSection), "Per", 100);
    }
    if (percent < 100 && GetRandomNumber() % 0x65 > percent) {
        return 0;
    }
    if (priority == -1) {
        priority = g_buteMgr.GetInt(static_cast<LPCTSTR>(voiceSection), "Pri", 1);
    }
    for (i32 i = 0; i < 2; i++) {
        if (indicators[i]->m_priority >= priority) {
            return 0;
        }
    }
    CRezItm* source = SelectVoiceVariant(voiceGroup, variantIndex);
    if (source == NULL) {
        return 0;
    }
    if (m_world->GetSoundStream() == NULL) {
        return 0;
    }
    CGruntVoice* firstIndicator = indicators[0];
    CGruntVoice* secondIndicator = m_indicators[1];
    i32 firstPriority = firstIndicator->m_priority;
    i32 secondPriority = secondIndicator->m_priority;
    i32 firstSourceObjectId = firstIndicator->m_sourceObjectId;
    i32 secondSourceObjectId = secondIndicator->m_sourceObjectId;
    i32 slotIndex;
    if (firstPriority <= secondPriority) {
        slotIndex = 0;
        if (secondSourceObjectId == sourceObjectId) {
            slotIndex = 1;
            if (firstPriority != 0 && m_streamVoices[0] != NULL) {
                m_streamVoices[0]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
            }
        } else if (secondPriority != 0 && m_streamVoices[1] != NULL) {
            m_streamVoices[1]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
        }
    } else {
        slotIndex = 1;
        if (firstSourceObjectId == sourceObjectId) {
            slotIndex = 0;
            if (secondPriority != 0 && m_streamVoices[1] != NULL) {
                m_streamVoices[1]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
            }
        } else if (firstPriority != 0 && m_streamVoices[0] != NULL) {
            m_streamVoices[0]->SetVolumePercent(g_gameReg->m_voiceVolume / 2);
        }
    }
    if (m_streamVoices[slotIndex] == NULL) {
        m_streamVoices[slotIndex] = m_world->GetSoundStream()->OpenStream(
            source,
            0x5000,
            0x1400,
            DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLDEFAULT,
            0,
            0
        );
        if (m_streamVoices[slotIndex] == NULL) {
            return 0;
        }
    }
    if (m_streamVoices[slotIndex]->PlaySource(source, m_voiceVolume, 0, 0, false) == 0) {
        return 0;
    }
    if (m_indicators[slotIndex]->BeginPlayback(
            sourceObjectId,
            m_streamVoices[slotIndex],
            priority,
            VOICE_INDICATOR_AT_IMAGE_ORIGIN
        )
        == 0) {
        return 0;
    }
    return 1;
}

i32 CVoiceManager::ResolveGruntVoiceGroup(CGrunt* grunt, i32 cueId) {
    if (grunt == NULL) {
        return 0;
    }
    if (grunt->m_gruntKind == GRUNT_DEATHTOUCH) {
        return VOICE_CUES_PER_BAND * 19 + cueId;
    }
    if (grunt->m_gruntKind == GRUNT_CONVERSION) {
        return VOICE_CUES_PER_BAND * 13 + cueId;
    }
    switch (static_cast<u32>(IDX(grunt->m_entranceReason))) {
        case IDX(PICKUP_NONE):
            return VOICE_CUES_PER_BAND * 17 + cueId;
        case IDX(PICKUP_BOMB):
            return VOICE_CUES_PER_BAND * 3 + cueId;
        case IDX(PICKUP_BOOMERANG):
            return VOICE_CUES_PER_BAND * 4 + cueId;
        case IDX(PICKUP_BRICK):
            return VOICE_CUES_PER_BAND * 5 + cueId;
        case IDX(PICKUP_CLUB):
            return VOICE_CUES_PER_BAND * 6 + cueId;
        case IDX(PICKUP_GAUNTLETZ):
            return VOICE_CUES_PER_BAND * 7 + cueId;
        case IDX(PICKUP_GLOVEZ):
            return VOICE_CUES_PER_BAND * 8 + cueId;
        case IDX(PICKUP_GOOBER):
            return VOICE_CUES_PER_BAND * 10 + cueId;
        case IDX(PICKUP_GRAVITYBOOTZ):
            return VOICE_CUES_PER_BAND * 11 + cueId;
        case IDX(PICKUP_GUNHAT):
            return VOICE_CUES_PER_BAND * 12 + cueId;
        case IDX(PICKUP_NERFGUN):
            return VOICE_CUES_PER_BAND * 16 + cueId;
        case IDX(PICKUP_ROCK):
            return VOICE_CUES_PER_BAND * 20 + cueId;
        case IDX(PICKUP_SHIELD):
            return VOICE_CUES_PER_BAND * 22 + cueId;
        case IDX(PICKUP_SHOVEL):
            return VOICE_CUES_PER_BAND * 23 + cueId;
        case IDX(PICKUP_SPRING):
            return VOICE_CUES_PER_BAND * 24 + cueId;
        case IDX(PICKUP_SPY):
            return VOICE_CUES_PER_BAND * 25 + cueId;
        case IDX(PICKUP_SWORD):
            return VOICE_CUES_PER_BAND * 27 + cueId;
        case IDX(PICKUP_TIMEBOMB):
            return VOICE_CUES_PER_BAND * 28 + cueId;
        case IDX(PICKUP_TOOB):
            if (grunt->m_coordToggle != false) {
                return VOICE_CUES_PER_BAND * 30 + cueId;
            }
            return VOICE_CUES_PER_BAND * 29 + cueId;
        case IDX(PICKUP_WAND):
            return VOICE_CUES_PER_BAND * 31 + cueId;
        case IDX(PICKUP_WARPSTONE):
            return VOICE_CUES_PER_BAND * 32 + cueId;
        case IDX(PICKUP_WELDER):
            return VOICE_CUES_PER_BAND * 33 + cueId;
        case IDX(PICKUP_WINGZ):
            return VOICE_CUES_PER_BAND * 34 + cueId;
        case IDX(PICKUP_BABYWALKER):
            return cueId;
        case IDX(PICKUP_BEACHBALL):
            return VOICE_CUES_PER_BAND * 1 + cueId;
        case IDX(PICKUP_BIGWHEEL):
            return VOICE_CUES_PER_BAND * 2 + cueId;
        case IDX(PICKUP_GOKART):
            return VOICE_CUES_PER_BAND * 9 + cueId;
        case IDX(PICKUP_JACKINTHEBOX):
            return VOICE_CUES_PER_BAND * 14 + cueId;
        case IDX(PICKUP_JUMPROPE):
            return VOICE_CUES_PER_BAND * 15 + cueId;
        case IDX(PICKUP_POGOSTICK):
            return VOICE_CUES_PER_BAND * 18 + cueId;
        case IDX(PICKUP_SCROLL):
            return VOICE_CUES_PER_BAND * 21 + cueId;
        case IDX(PICKUP_SQUEAKTOY):
            return VOICE_CUES_PER_BAND * 26 + cueId;
        case IDX(PICKUP_YOYO):
            return VOICE_CUES_PER_BAND * 35 + cueId;
        default:
            return 0;
    }
}

CRezItm* CVoiceManager::SelectVoiceVariant(i32 voiceGroup) {
    return NULL;
}

CRezItm* CVoiceManager::SelectVoiceVariant(i32 voiceGroup, i32 variantIndex) {
    if (voiceGroup < 0) {
        return NULL;
    }
    if (voiceGroup == 0) {
        return NULL;
    }
    if (voiceGroup >= m_voiceGroups.GetSize()) {
        return NULL;
    }
    CSpawnList* group = static_cast<CSpawnList*>(m_voiceGroups[voiceGroup]);
    if (group == NULL) {
        return NULL;
    }

    i32 selectedIndex = variantIndex;
    if (selectedIndex == -1 || selectedIndex >= group->GetCount()) {
        i32 lastIndex = group->GetCount() - 1;

        CGruntzMgr* game = g_gameReg;
        i32 variantCount = lastIndex + 1;
        if (variantCount == 0) {
            const i32 rnd = GetRandomNumber();
            selectedIndex = ((rnd & 1)) ? 0 : lastIndex;
        } else {
            selectedIndex = game->Rand() % variantCount;
        }
        if (group->GetCount() > 1) {
            i32 tries = 5;
            while (selectedIndex == group->m_lastPicked && tries > 0) {
                selectedIndex = RandRange(0, group->GetCount() - 1);
                tries--;
            }
        }
    }

    group->m_lastPicked = selectedIndex;
    CSpawnEntry* variant = group->GetEntry(selectedIndex);
    if (variant == NULL) {
        return NULL;
    }
    return m_game->ResourceArchive()->GetRezFromPath(
        static_cast<LPCTSTR>(variant->GetName()),
        REZ_TAG_WAV
    );
}

BOOL CVoiceManager::BuildVoiceGroups() {
    m_voiceGroups.RemoveAll();
    m_voiceGroups.SetAtGrow(0, NULL);
    for (i32 i = 1; i < 0x4b0; i++) {
        m_voiceGroups.SetAtGrow(i, BuildVoiceGroup(i));
    }
    return true;
}

CSpawnList* CVoiceManager::BuildVoiceGroup(i32 voiceGroup) {
    if (voiceGroup <= 0) {
        return NULL;
    }
    if (voiceGroup >= 0x4b0) {
        return NULL;
    }

    CSpawnList* group = NULL;
    CString fallback, section, key, resourceName;
    section.Format("SG%i", voiceGroup);
    CString directory = *g_buteMgr.GetString(static_cast<LPCTSTR>(section), "DIR", &fallback);

    key.Format("S%i", 1);
    CString soundName =
        *g_buteMgr.GetString(static_cast<LPCTSTR>(section), static_cast<LPCTSTR>(key), &fallback);

    i32 missingResource = 0;
    if (!soundName.IsEmpty()) {
        group = new CSpawnList();
    }

    if (!soundName.IsEmpty()) {
        i32 i = 1;
        while (!soundName.IsEmpty() && missingResource == 0) {
            i++;
            if (directory.IsEmpty()) {
                resourceName.Format("VOICES_%s", static_cast<LPCTSTR>(soundName));
            } else {
                resourceName.Format(
                    "VOICES_%s_%s",
                    static_cast<LPCTSTR>(directory),
                    static_cast<LPCTSTR>(soundName)
                );
            }
            CRezItm* source = m_game->ResourceArchive()->GetRezFromPath(
                static_cast<LPCTSTR>(resourceName),
                REZ_TAG_WAV
            );
            if (source != NULL) {

                group->AddVoiceSound(resourceName, 0);
                key.Format("S%i", i);
                soundName = *g_buteMgr.GetString(
                    static_cast<LPCTSTR>(section),
                    static_cast<LPCTSTR>(key),
                    &fallback
                );
            } else {
                missingResource = 1;
            }
        }
    }
    return group;
}

void CSpawnList::AddVoiceSound(CString resourceName, i32 data) {
    CSpawnEntry* node = new CSpawnEntry(resourceName, data);
    if (node != NULL) {
        m_list.AddTail(node);
    }
}

CSpawnEntry::CSpawnEntry(CString name, i32 data) {
    m_name = name;
    m_flag = false;
    m_data = data;
}

i32 CVoiceManager::IsAnyVoicePlaying() {
    i32 i = 0;
    CGruntVoice** indicator = m_indicators;
    for (; i < 2; i++, indicator++) {
        if (*indicator != NULL && (*indicator)->m_priority != 0) {
            return 1;
        }
    }
    return 0;
}

i32 CVoiceManager::IsVoiceSlotPlaying(i32 slotIndex) {
    CGruntVoice* indicator = m_indicators[slotIndex];
    if (indicator != NULL && indicator->m_priority != 0) {
        return 1;
    }
    return 0;
}

void CVoiceManager::StopVoice(i32 sourceObjectId) {
    i32 firstSourceObjectId = m_indicators[0]->m_sourceObjectId;
    i32 secondSourceObjectId = m_indicators[1]->m_sourceObjectId;
    if (firstSourceObjectId == sourceObjectId) {
        if (m_streamVoices[0] != NULL) {
            m_streamVoices[0]->m_feeder.Pause();
        }
        if (m_indicators[0] != NULL) {
            m_indicators[0]->ResetPlayback();
        }
    } else if (secondSourceObjectId == sourceObjectId) {
        if (m_streamVoices[1] != NULL) {
            m_streamVoices[1]->m_feeder.Pause();
        }
        if (m_indicators[1] != NULL) {
            m_indicators[1]->ResetPlayback();
        }
    }
}

void CVoiceManager::PauseAllVoices() {

    for (i32 k = 0; k < 2; k++) {
        if (m_streamVoices[k] != NULL) {
            m_streamVoices[k]->m_feeder.Pause();
        }
        if (m_indicators[k] != NULL) {
            m_indicators[k]->ResetPlayback();
        }
    }
}

void CVoiceManager::ResetVoiceSelections() {
    PauseAllVoices();
    for (i32 i = 0; i < m_voiceGroups.GetSize(); i++) {
        CSpawnList* group = static_cast<CSpawnList*>(m_voiceGroups[i]);
        if (group != NULL) {
            group->m_lastPicked = -1;
        }
    }
}

BOOL CVoiceManager::IsVoiceEnabled() {
    return m_game->m_isVoiceEnabled != false;
}
