#include <rva.h>

#include <Gruntz/VoiceManager.h>

#include <Mfc.h>

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
#include <Rez/RezArchive.h>
#include <Rez/RezTypeTag.h>

#define CLEAR_VOICE_INDICATORS memset(m_indicators, 0, sizeof(m_indicators))

RVA(0x00085df0, 0x4a)
CVoiceManager::~CVoiceManager() {
    Clear();
}

RVA(0x0011adc0, 0x44)
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

// @early-stop
RVA(0x0011ae30, 0x95)
void CVoiceManager::Clear() {
    for (i32 i = 0; i < m_voiceGroups.GetSize(); i++) {
        CSpawnList* group = static_cast<CSpawnList*>(m_voiceGroups[i]);

        delete group;
    }
    m_voiceGroups.SetSize(0, -1);
    if (m_world != NULL && m_world->m_soundStream != NULL) {
        StreamVoice** stream = m_streamVoices;
        for (i32 k = 0; k < 2; k++) {
            if (stream[0] != NULL) {
                m_world->m_soundStream->DestroyVoice(stream[0]);
                stream[0] = NULL;
            }
            stream++;
        }
    }
    m_game = NULL;
    m_world = NULL;
    CGruntVoice** indicator = m_indicators;
    for (i32 a = 0; a < 2; a++) {
        *indicator = NULL;
        indicator++;
    }
    StreamVoice** stream = m_streamVoices;
    for (i32 b = 0; b < 2; b++) {
        *stream = NULL;
        stream++;
    }
}

RVA(0x0011af00, 0x62)
BOOL CVoiceManager::CreateVoiceIndicators() {
    ClearVoiceIndicatorSlots();
    i32 i = 0;
    CGruntVoice** slot = m_indicators;
    for (; i < 2; i++, slot++) {
        CGameObject* spr = m_world->m_childGroup->CreateSprite(
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

RVA(0x0011af90, 0xb)
void CVoiceManager::ClearVoiceIndicatorSlots() {
    CLEAR_VOICE_INDICATORS;
}

RVA(0x0011afb0, 0x321)
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
    if (source == NULL || m_world->m_soundStream == NULL) {
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
        m_streamVoices[slotIndex] =
            m_world->m_soundStream->OpenStream(source, 0x5000, 0x1400, 0x100e0, 0, 0);
        if (m_streamVoices[slotIndex] == NULL) {
            return false;
        }
    }
    StreamVoice* stream = m_streamVoices[slotIndex];
    i32 volume = m_voiceVolume;
    stream->m_feeder.Pause();
    if (stream->SetSource(source) != 0 && stream->Configure(volume, 0, 0, false) != 0) {
        CGruntVoice* indicator = m_indicators[slotIndex];
        if (indicator->BeginPlayback(
                grunt->m_object->m_objectId,
                stream,
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

RVA(0x0011b3b0, 0x338)
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
    if (m_world->m_soundStream == NULL) {
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
        m_streamVoices[slotIndex] =
            m_world->m_soundStream->OpenStream(source, 0x5000, 0x1400, 0x100e0, 0, 0);
        if (m_streamVoices[slotIndex] == NULL) {
            return 0;
        }
    }
    StreamVoice* stream = m_streamVoices[slotIndex];
    i32 volume = m_voiceVolume;
    stream->m_feeder.Pause();
    if (stream->SetSource(source) == 0) {
        goto streamFailed;
    }
    if (stream->Configure(volume, 0, 0, false) == 0) {
        goto streamFailed;
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

streamFailed:
    return 0;
}

RVA(0x0011b7c0, 0x304)
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
    if (m_world->m_soundStream == NULL) {
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
        m_streamVoices[slotIndex] =
            m_world->m_soundStream->OpenStream(source, 0x5000, 0x1400, 0x100e0, 0, 0);
        if (m_streamVoices[slotIndex] == NULL) {
            return 0;
        }
    }
    StreamVoice* stream = m_streamVoices[slotIndex];
    i32 volume = m_voiceVolume;
    stream->m_feeder.Pause();
    if (stream->SetSource(source) == 0) {
        goto streamFailed;
    }
    if (stream->Configure(volume, 0, 0, false) == 0) {
        goto streamFailed;
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

streamFailed:
    return 0;
}

RVA(0x0011bba0, 0x280)
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

// @identity-TODO: the one-argument overload shape is inferred from the adjacent picker.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0011bec0, 0x5)
CRezItm* CVoiceManager::SelectVoiceVariant(i32 voiceGroup) {
    return NULL;
}

RVA(0x0011bee0, 0x230)
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
    if (selectedIndex == -1 || selectedIndex >= group->m_list.GetCount()) {
        i32 lastIndex = group->m_list.GetCount() - 1;

        CGruntzMgr* game = g_gameReg;
        i32 variantCount = lastIndex + 1;
        if (variantCount == 0) {
            i32 seed;
            const i32 rnd = GetRandomNumber();
            selectedIndex = ((rnd & 1)) ? 0 : lastIndex;
        } else {
            selectedIndex = game->Rand() % variantCount;
        }
        if (group->m_list.GetCount() > 1) {
            i32 tries = 5;
            while (selectedIndex == group->m_lastPicked && tries > 0) {
                i32 retryLastIndex = group->m_list.GetCount() - 1;
                i32 retryVariantCount = retryLastIndex + 1;

                if (retryVariantCount == 0) {
                    selectedIndex = (GetRandomNumber() & 1) ? 0 : retryLastIndex;
                } else {
                    selectedIndex = GetRandomNumber() % retryVariantCount;
                }
                tries--;
            }
        }
    }

    group->m_lastPicked = selectedIndex;
    CSpawnEntry* variant;
    if (selectedIndex >= group->m_list.GetCount()) {
        variant = NULL;
    } else {

        CGruntCoordList* nodes = static_cast<CGruntCoordList*>(&group->m_list);
        POSITION& cursor = group->m_cursor;
        cursor = group->m_list.GetHeadPosition();
        if (cursor == NULL) {
            variant = NULL;
        } else {
            variant = static_cast<CSpawnEntry*>(nodes->NextData(cursor));
        }
        for (i32 i = selectedIndex; i > 0; i--) {
            if (cursor == NULL) {
                variant = NULL;
            } else {
                variant = static_cast<CSpawnEntry*>(nodes->NextData(cursor));
            }
        }
    }
    if (variant == NULL) {
        return NULL;
    }
    return m_game->m_resourceArchive->GetRezFromPath(
        static_cast<LPCTSTR>(variant->GetName()),
        REZ_TAG_WAV
    );
}

RVA(0x0011c1a0, 0x46)
BOOL CVoiceManager::BuildVoiceGroups() {
    m_voiceGroups.SetSize(0, -1);
    m_voiceGroups.SetAtGrow(0, NULL);
    for (i32 i = 1; i < 0x4b0; i++) {
        m_voiceGroups.SetAtGrow(i, BuildVoiceGroup(i));
    }
    return true;
}

// @early-stop
RVA(0x0011c210, 0x29d)
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
            CRezItm* source = m_game->m_resourceArchive->GetRezFromPath(
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

RVA(0x0011c560, 0x91)
void CSpawnList::AddVoiceSound(CString resourceName, i32 data) {
    CSpawnEntry* node = new CSpawnEntry(resourceName, data);
    if (node != NULL) {
        m_list.AddTail(node);
    }
}

RVA(0x0011c630, 0x6e)
CSpawnEntry::CSpawnEntry(CString name, i32 data) {
    m_name = name;
    m_flag = false;
    m_data = data;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0011c6c0, 0x27)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0011c700, 0x20)
i32 CVoiceManager::IsVoiceSlotPlaying(i32 slotIndex) {
    CGruntVoice* indicator = m_indicators[slotIndex];
    if (indicator != NULL && indicator->m_priority != 0) {
        return 1;
    }
    return 0;
}

RVA(0x0011c730, 0x5c)
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

RVA(0x0011c7b0, 0x2d)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0011c7f0, 0x2b)
void CVoiceManager::ResetVoiceSelections() {
    PauseAllVoices();
    for (i32 i = 0; i < m_voiceGroups.GetSize(); i++) {
        CSpawnList* group = static_cast<CSpawnList*>(m_voiceGroups[i]);
        if (group != NULL) {
            group->m_lastPicked = -1;
        }
    }
}

RVA(0x0011c830, 0x12)
BOOL CVoiceManager::IsVoiceEnabled() {
    return m_game->m_isVoiceEnabled != false;
}
