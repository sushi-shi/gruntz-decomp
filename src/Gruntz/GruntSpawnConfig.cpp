#include <rva.h>

#include <Gruntz/GruntSpawnConfig.h>

#include <Mfc.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
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
#include <Rez/RezTypeTag.h>

RVA(0x00085df0, 0x4a)
CGruntSpawnConfig::~CGruntSpawnConfig() {
    Clear();
}

RVA(0x0011adc0, 0x44)
BOOL CGruntSpawnConfig::Init(CGruntzMgr* owner) {
    if (owner == NULL) {
        return 0;
    }
    m_configTree = NULL;
    memset(m_voices, 0, sizeof(m_voices));
    memset(m_streams, 0, sizeof(m_streams));
    m_owner = owner;
    m_configTree = owner->m_world;
    m_voiceVolume = 0x64;
    return BuildVoiceList() != 0;
}

// @early-stop
RVA(0x0011ae30, 0x95)
void CGruntSpawnConfig::Clear() {
    for (i32 i = 0; i < m_voiceLists.GetSize(); i++) {
        CSpawnList* e = static_cast<CSpawnList*>(m_voiceLists[i]);

        delete e;
    }
    m_voiceLists.SetSize(0, -1);
    if (m_configTree != NULL && m_configTree->m_soundStream != NULL) {
        StreamVoice** p = m_streams;
        for (i32 k = 0; k < 2; k++) {
            if (p[0] != NULL) {
                m_configTree->m_soundStream->DestroyVoice(p[0]);
                p[0] = NULL;
            }
            p++;
        }
    }
    m_owner = NULL;
    m_configTree = NULL;
    CGruntVoice** v = m_voices;
    for (i32 a = 0; a < 2; a++) {
        *v = NULL;
        v++;
    }
    StreamVoice** s = m_streams;
    for (i32 b = 0; b < 2; b++) {
        *s = NULL;
        s++;
    }
}

RVA(0x0011af00, 0x62)
BOOL CGruntSpawnConfig::LoadGruntVoices() {
    ClearSprites();
    i32 i = 0;
    CGruntVoice** slot = m_voices;
    for (; i < 2; i++, slot++) {
        CGameObject* spr =
            m_configTree->m_childGroup->CreateSprite(0, 0, 0, 0xdbba1, "GruntVoice", 0x4040003);
        spr->m_animWorker->m_notify(spr);
        CGruntVoice* got = static_cast<CGruntVoice*>(spr->m_animWorker->m_logic);
        *slot = got;
        if (got == NULL) {
            return 0;
        }
    }
    return 1;
}

RVA(0x0011af90, 0xb)
void CGruntSpawnConfig::ClearSprites() {
    memset(m_voices, 0, sizeof(m_voices));
}

RVA(0x0011afb0, 0x321)
BOOL CGruntSpawnConfig::LoadGruntSpawnConfig(
    CGrunt* who,
    i32 cue,
    i32 which,
    i32 priority,
    i32 percent
) {
    if (m_voices[0] == NULL && !LoadGruntVoices()) {
        return 0;
    }
    if (who == NULL) {
        return 0;
    }
    if (!IsReady()) {
        return 0;
    }
    i32 voiceId = GetButeSlot(who, cue);
    CString voiceSection;
    CString cueKey;
    voiceSection.Format("SG%i", voiceId);
    cueKey.Format("G%i", cue);
    if (percent == -1) {
        percent = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(voiceSection), "Per", -1);
        if (percent == -1) {
            percent = g_buteMgr.GetIntDef("GruntPercent", static_cast<LPCTSTR>(cueKey), 0);
        }
    }
    if (percent < 100 && g_gameReg->Rand() % 0x65 > percent) {
        return 0;
    }
    if (priority == -1) {
        priority = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(voiceSection), "Pri", -1);
        if (priority == -1) {
            priority = g_buteMgr.GetIntDef("GruntPriority", static_cast<LPCTSTR>(cueKey), 1);
        }
    }
    for (i32 i = 0; i < 2; i++) {
        if (m_voices[i]->m_playFlags >= priority) {
            return 0;
        }
    }
    CParseSource* src = PickWeighted(voiceId, which);
    if (src == NULL || m_configTree->m_soundStream == NULL) {
        return 0;
    }
    CGruntVoice* v8 = m_voices[0];
    CGruntVoice* v0c = m_voices[1];
    i32 a = v8->m_playFlags;
    i32 b = v0c->m_playFlags;
    i32 c = v8->m_source;
    i32 d = v0c->m_source;
    i32 chosen;
    if (a <= b) {
        chosen = 0;
        if (d == who->m_object->m_objectId) {
            chosen = 1;
            if (a != 0 && m_streams[0] != NULL) {
                m_streams[0]->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (b != 0 && m_streams[1] != NULL) {
            m_streams[1]->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    } else {
        chosen = 1;
        if (c == who->m_object->m_objectId) {
            chosen = 0;
            if (b != 0 && m_streams[1] != NULL) {
                m_streams[1]->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (a != 0 && m_streams[0] != NULL) {
            m_streams[0]->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    }
    if (m_streams[chosen] == NULL) {
        m_streams[chosen] =
            m_configTree->m_soundStream->OpenStream(src, 0x5000, 0x1400, 0x100e0, 0, 0);
        if (m_streams[chosen] == NULL) {
            return 0;
        }
    }
    StreamVoice* stream = m_streams[chosen];
    i32 vol = m_voiceVolume;
    stream->m_feeder.Pause();
    if (stream->SetSource(src) != 0 && stream->Configure(vol, 0, 0, 0) != 0) {
        CGruntVoice* voice = m_voices[chosen];
        if (voice->Setup(who->m_object->m_objectId, stream, priority, 0)) {
            return 1;
        } else {
            return 0;
        }
    }
    return 0;
}

RVA(0x0011b3b0, 0x338)
i32 CGruntSpawnConfig::SpawnVoiceDriver(
    CGrunt* who,
    i32 voiceId,
    i32 which,
    i32 objId,
    i32 priority,
    i32 percent
) {
    CGruntVoice** voices = m_voices;
    if (voices[0] == NULL && !LoadGruntVoices()) {
        return 0;
    }
    if (who == NULL && objId == 0) {
        return 0;
    }
    if (!IsReady()) {
        return 0;
    }
    CString voiceSection;
    voiceSection.Format("SG%i", voiceId);
    if (percent == -1) {
        percent = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(voiceSection), "Per", 100);
    }
    if (percent < 100 && GetRandomNumber() % 0x65 > percent) {
        return 0;
    }
    if (priority == -1) {
        priority = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(voiceSection), "Pri", 1);
    }
    for (i32 i = 0; i < 2; i++) {
        if (voices[i]->m_playFlags >= priority) {
            return 0;
        }
    }
    i32 id = 0;
    if (objId == 0 && who != NULL) {
        id = who->m_object->m_objectId;
    }
    CParseSource* src = PickWeighted(voiceId, which);
    if (src == NULL) {
        return 0;
    }
    if (m_configTree->m_soundStream == NULL) {
        return 0;
    }
    CGruntVoice* v8 = voices[0];
    CGruntVoice* v0c = m_voices[1];
    i32 a = v8->m_playFlags;
    i32 b = v0c->m_playFlags;
    i32 c = v8->m_source;
    i32 d = v0c->m_source;
    i32 chosen;
    if (a > b) {
        chosen = 1;
        if (c == id) {
            chosen = 0;
            if (b != 0 && m_streams[1] != NULL) {
                (static_cast<DirectSoundMgr*>(m_streams[1]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (a != 0 && m_streams[0] != NULL) {
            (static_cast<DirectSoundMgr*>(m_streams[0]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    } else {
        chosen = 0;
        if (d == id) {
            chosen = 1;
            if (a != 0 && m_streams[0] != NULL) {
                (static_cast<DirectSoundMgr*>(m_streams[0]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }

        } else if (b != 0 && id != 0) {
            (static_cast<DirectSoundMgr*>(m_streams[1]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    }
    if (m_streams[chosen] == NULL) {
        m_streams[chosen] =
            m_configTree->m_soundStream->OpenStream(src, 0x5000, 0x1400, 0x100e0, 0, 0);
        if (m_streams[chosen] == NULL) {
            return 0;
        }
    }
    StreamVoice* stream = m_streams[chosen];
    i32 vol = m_voiceVolume;
    stream->m_feeder.Pause();
    if (stream->SetSource(src) == 0) {
        goto streamFailed;
    }
    if (stream->Configure(vol, 0, 0, 0) == 0) {
        goto streamFailed;
    }
    if (m_voices[chosen]->Setup(id, stream, priority, 0) == 0) {
        return 0;
    }
    return 1;

streamFailed:
    return 0;
}

RVA(0x0011b7c0, 0x304)
i32 CGruntSpawnConfig::SpawnVoiceDriver(
    i32 objId,
    i32 voiceId,
    i32 which,
    i32 priority,
    i32 percent
) {
    CGruntVoice** voices = m_voices;
    if (voices[0] == NULL && !LoadGruntVoices()) {
        return 0;
    }
    if (objId == 0) {
        return 0;
    }
    if (!IsReady()) {
        return 0;
    }
    CString voiceSection;
    voiceSection.Format("SG%i", voiceId);
    if (percent == -1) {
        percent = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(voiceSection), "Per", 100);
    }
    if (percent < 100 && GetRandomNumber() % 0x65 > percent) {
        return 0;
    }
    if (priority == -1) {
        priority = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(voiceSection), "Pri", 1);
    }
    for (i32 i = 0; i < 2; i++) {
        if (voices[i]->m_playFlags >= priority) {
            return 0;
        }
    }
    CParseSource* src = PickWeighted(voiceId, which);
    if (src == NULL) {
        return 0;
    }
    if (m_configTree->m_soundStream == NULL) {
        return 0;
    }
    CGruntVoice* v8 = voices[0];
    CGruntVoice* v0c = m_voices[1];
    i32 a = v8->m_playFlags;
    i32 b = v0c->m_playFlags;
    i32 c = v8->m_source;
    i32 d = v0c->m_source;
    i32 chosen;
    if (a > b) {
        chosen = 1;
        if (c == objId) {
            chosen = 0;
            if (b != 0 && m_streams[1] != NULL) {
                (static_cast<DirectSoundMgr*>(m_streams[1]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (a != 0 && m_streams[0] != NULL) {
            (static_cast<DirectSoundMgr*>(m_streams[0]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    } else {
        chosen = 0;
        if (d == objId) {
            chosen = 1;
            if (a != 0 && m_streams[0] != NULL) {
                (static_cast<DirectSoundMgr*>(m_streams[0]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (b != 0 && m_streams[1] != NULL) {
            (static_cast<DirectSoundMgr*>(m_streams[1]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    }
    if (m_streams[chosen] == NULL) {
        m_streams[chosen] =
            m_configTree->m_soundStream->OpenStream(src, 0x5000, 0x1400, 0x100e0, 0, 0);
        if (m_streams[chosen] == NULL) {
            return 0;
        }
    }
    StreamVoice* stream = m_streams[chosen];
    i32 vol = m_voiceVolume;
    stream->m_feeder.Pause();
    if (stream->SetSource(src) == 0) {
        goto streamFailed;
    }
    if (stream->Configure(vol, 0, 0, 0) == 0) {
        goto streamFailed;
    }
    if (m_voices[chosen]->Setup(objId, stream, priority, 1) == 0) {
        return 0;
    }
    return 1;

streamFailed:
    return 0;
}

RVA(0x0011bba0, 0x280)
i32 CGruntSpawnConfig::GetButeSlot(CGrunt* config, i32 cue) {
    if (config == NULL) {
        return 0;
    }
    if (config->m_gruntKind == GRUNT_DEATHTOUCH) {
        return VOICE_CUES_PER_BAND * 19 + cue;
    }
    if (config->m_gruntKind == GRUNT_CONVERSION) {
        return VOICE_CUES_PER_BAND * 13 + cue;
    }
    // The UNSIGNED key is codegen steering, not a widening for its own sake:
    // cl emits the unsigned `ja` range check only for an unsigned switch key
    // (docs/patterns/switch-key-unsigned-ja-vs-jg.md).
    switch (static_cast<u32>(IDX(config->m_entranceReason))) {
        case IDX(PICKUP_NONE):
            return VOICE_CUES_PER_BAND * 17 + cue;
        case IDX(PICKUP_BOMB):
            return VOICE_CUES_PER_BAND * 3 + cue;
        case IDX(PICKUP_BOOMERANG):
            return VOICE_CUES_PER_BAND * 4 + cue;
        case IDX(PICKUP_BRICK):
            return VOICE_CUES_PER_BAND * 5 + cue;
        case IDX(PICKUP_CLUB):
            return VOICE_CUES_PER_BAND * 6 + cue;
        case IDX(PICKUP_GAUNTLETZ):
            return VOICE_CUES_PER_BAND * 7 + cue;
        case IDX(PICKUP_GLOVEZ):
            return VOICE_CUES_PER_BAND * 8 + cue;
        case IDX(PICKUP_GOOBER):
            return VOICE_CUES_PER_BAND * 10 + cue;
        case IDX(PICKUP_GRAVITYBOOTZ):
            return VOICE_CUES_PER_BAND * 11 + cue;
        case IDX(PICKUP_GUNHAT):
            return VOICE_CUES_PER_BAND * 12 + cue;
        case IDX(PICKUP_NERFGUN):
            return VOICE_CUES_PER_BAND * 16 + cue;
        case IDX(PICKUP_ROCK):
            return VOICE_CUES_PER_BAND * 20 + cue;
        case IDX(PICKUP_SHIELD):
            return VOICE_CUES_PER_BAND * 22 + cue;
        case IDX(PICKUP_SHOVEL):
            return VOICE_CUES_PER_BAND * 23 + cue;
        case IDX(PICKUP_SPRING):
            return VOICE_CUES_PER_BAND * 24 + cue;
        case IDX(PICKUP_SPY):
            return VOICE_CUES_PER_BAND * 25 + cue;
        case IDX(PICKUP_SWORD):
            return VOICE_CUES_PER_BAND * 27 + cue;
        case IDX(PICKUP_TIMEBOMB):
            return VOICE_CUES_PER_BAND * 28 + cue;
        case IDX(PICKUP_TOOB):
            if (config->m_coordToggle != 0) {
                return VOICE_CUES_PER_BAND * 30 + cue;
            }
            return VOICE_CUES_PER_BAND * 29 + cue;
        case IDX(PICKUP_WAND):
            return VOICE_CUES_PER_BAND * 31 + cue;
        case IDX(PICKUP_WARPSTONE):
            return VOICE_CUES_PER_BAND * 32 + cue;
        case IDX(PICKUP_WELDER):
            return VOICE_CUES_PER_BAND * 33 + cue;
        case IDX(PICKUP_WINGZ):
            return VOICE_CUES_PER_BAND * 34 + cue;
        case IDX(PICKUP_BABYWALKER):
            return cue;
        case IDX(PICKUP_BEACHBALL):
            return VOICE_CUES_PER_BAND * 1 + cue;
        case IDX(PICKUP_BIGWHEEL):
            return VOICE_CUES_PER_BAND * 2 + cue;
        case IDX(PICKUP_GOKART):
            return VOICE_CUES_PER_BAND * 9 + cue;
        case IDX(PICKUP_JACKINTHEBOX):
            return VOICE_CUES_PER_BAND * 14 + cue;
        case IDX(PICKUP_JUMPROPE):
            return VOICE_CUES_PER_BAND * 15 + cue;
        case IDX(PICKUP_POGOSTICK):
            return VOICE_CUES_PER_BAND * 18 + cue;
        case IDX(PICKUP_SCROLL):
            return VOICE_CUES_PER_BAND * 21 + cue;
        case IDX(PICKUP_SQUEAKTOY):
            return VOICE_CUES_PER_BAND * 26 + cue;
        case IDX(PICKUP_YOYO):
            return VOICE_CUES_PER_BAND * 35 + cue;
        default:
            return 0;
    }
}

// @identity-TODO: the one-argument overload shape is inferred from the adjacent picker.
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0011bec0, 0x5)
CParseSource* CGruntSpawnConfig::PickWeighted(i32 voiceId) {
    return NULL;
}

RVA(0x0011bee0, 0x230)
CParseSource* CGruntSpawnConfig::PickWeighted(i32 voiceId, i32 which) {
    if (voiceId < 0) {
        return 0;
    }
    if (voiceId == 0) {
        return 0;
    }
    if (voiceId >= m_voiceLists.GetSize()) {
        return 0;
    }
    CSpawnList* list = static_cast<CSpawnList*>(m_voiceLists[voiceId]);
    if (list == NULL) {
        return 0;
    }

    i32 pick = which;
    if (pick == -1 || pick >= list->m_list.GetCount()) {
        i32 hi = list->m_list.GetCount() - 1;

        CGruntzMgr* reg = g_gameReg;
        i32 span = hi + 1;
        if (span == 0) {
            i32 seed;
            const i32 rnd = GetRandomNumber();
            pick = ((rnd & 1)) ? 0 : hi;
        } else {
            pick = reg->Rand() % span;
        }
        if (list->m_list.GetCount() > 1) {
            i32 tries = 5;
            while (pick == list->m_lastPicked && tries > 0) {
                i32 rehi = list->m_list.GetCount() - 1;
                i32 respan = rehi + 1;

                if (respan == 0) {
                    pick = (GetRandomNumber() & 1) ? 0 : rehi;
                } else {
                    pick = GetRandomNumber() % respan;
                }
                tries--;
            }
        }
    }

    list->m_lastPicked = pick;
    CSpawnEntry* entry;
    if (pick >= list->m_list.GetCount()) {
        entry = NULL;
    } else {

        CGruntCoordList* nodes = static_cast<CGruntCoordList*>(&list->m_list);
        POSITION& cursor = list->m_cursor;
        cursor = list->m_list.GetHeadPosition();
        if (cursor == NULL) {
            entry = NULL;
        } else {
            entry = static_cast<CSpawnEntry*>(nodes->NextData(cursor));
        }
        for (i32 i = pick; i > 0; i--) {
            if (cursor == NULL) {
                entry = NULL;
            } else {
                entry = static_cast<CSpawnEntry*>(nodes->NextData(cursor));
            }
        }
    }
    if (entry == NULL) {
        return 0;
    }
    return m_owner->m_symParser->ResolveQualified(
        static_cast<LPCTSTR>(entry->GetName()),
        REZ_TAG_WAV
    );
}

RVA(0x0011c1a0, 0x46)
BOOL CGruntSpawnConfig::BuildVoiceList() {
    m_voiceLists.SetSize(0, -1);
    m_voiceLists.SetAtGrow(0, 0);
    for (i32 i = 1; i < 0x4b0; i++) {
        m_voiceLists.SetAtGrow(i, BuildVoiceSoundList(i));
    }
    return 1;
}

// @early-stop
RVA(0x0011c210, 0x29d)
CSpawnList* CGruntSpawnConfig::BuildVoiceSoundList(i32 n) {
    if (n <= 0) {
        return 0;
    }
    if (n >= 0x4b0) {
        return 0;
    }

    CSpawnList* list = NULL;
    CString dflt, section, key, name;
    section.Format("SG%i", n);
    CString dirName = *g_buteMgr.GetStringDef(static_cast<LPCTSTR>(section), "DIR", &dflt);

    key.Format("S%i", 1);
    CString sndName =
        *g_buteMgr.GetStringDef(static_cast<LPCTSTR>(section), static_cast<LPCTSTR>(key), &dflt);

    i32 stop = 0;
    if (!sndName.IsEmpty()) {
        list = new CSpawnList();
    }

    if (!sndName.IsEmpty()) {
        i32 i = 1;
        while (!sndName.IsEmpty() && stop == 0) {
            i++;
            if (dirName.IsEmpty()) {
                name.Format("VOICES_%s", static_cast<LPCTSTR>(sndName));
            } else {
                name.Format(
                    "VOICES_%s_%s",
                    static_cast<LPCTSTR>(dirName),
                    static_cast<LPCTSTR>(sndName)
                );
            }
            CParseSource* res =
                m_owner->m_symParser->ResolveQualified(static_cast<LPCTSTR>(name), REZ_TAG_WAV);
            if (res != NULL) {

                list->AddVoiceSound(name, 0);
                key.Format("S%i", i);
                sndName = *g_buteMgr.GetStringDef(
                    static_cast<LPCTSTR>(section),
                    static_cast<LPCTSTR>(key),
                    &dflt
                );
            } else {
                stop = 1;
            }
        }
    }
    return list;
}

RVA(0x0011c560, 0x91)
void CSpawnList::AddVoiceSound(CString s, i32 flag) {
    CSpawnEntry* node = new CSpawnEntry(s, flag);
    if (node != NULL) {
        m_list.AddTail(node);
    }
}

RVA(0x0011c630, 0x6e)
CSpawnEntry::CSpawnEntry(CString name, i32 data) {
    m_name = name;
    m_flag = 0;
    m_data = data;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0011c6c0, 0x27)
i32 CGruntSpawnConfig::AnyVoicePlaying() {
    i32 i = 0;
    CGruntVoice** p = m_voices;
    for (; i < 2; i++, p++) {
        if (*p != NULL && (*p)->m_playFlags != 0) {
            return 1;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0011c700, 0x20)
i32 CGruntSpawnConfig::VoicePlaying(i32 i) {
    CGruntVoice* v = m_voices[i];
    if (v != NULL && v->m_playFlags != 0) {
        return 1;
    }
    return 0;
}

RVA(0x0011c730, 0x5c)
void CGruntSpawnConfig::StopVoice(i32 id) {
    i32 tag08 = m_voices[0]->m_source;
    i32 tag0c = m_voices[1]->m_source;
    if (tag08 == id) {
        if (m_streams[0] != NULL) {
            m_streams[0]->m_feeder.Pause();
        }
        if (m_voices[0] != NULL) {
            m_voices[0]->Reset();
        }
    } else if (tag0c == id) {
        if (m_streams[1] != NULL) {
            m_streams[1]->m_feeder.Pause();
        }
        if (m_voices[1] != NULL) {
            m_voices[1]->Reset();
        }
    }
}

RVA(0x0011c7b0, 0x2d)
void CGruntSpawnConfig::PauseAllVoices() {

    for (i32 k = 0; k < 2; k++) {
        if (m_streams[k] != NULL) {
            m_streams[k]->m_feeder.Pause();
        }
        if (m_voices[k] != NULL) {
            m_voices[k]->Reset();
        }
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0011c7f0, 0x2b)
void CGruntSpawnConfig::ResetPicks() {
    PauseAllVoices();
    for (i32 i = 0; i < m_voiceLists.GetSize(); i++) {
        CSpawnList* e = static_cast<CSpawnList*>(m_voiceLists[i]);
        if (e != NULL) {
            e->m_lastPicked = -1;
        }
    }
}

RVA(0x0011c830, 0x12)
BOOL CGruntSpawnConfig::IsReady() {
    return m_owner->m_isVoiceEnabled != 0;
}
