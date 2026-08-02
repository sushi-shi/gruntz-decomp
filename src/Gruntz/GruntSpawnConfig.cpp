#include <rva.h>

#include <Gruntz/GruntSpawnConfig.h>

#include <Bute/ButeMgr.h>
#include <Bute/SymParser.h>
#include <Dsndmgr/StreamFeeder.h>
#include <Dsndmgr/StreamVoice.h>
#include <Gruntz/Enums.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntVoice.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Random.h>

RVA(0x00085df0, 0x4a)
CGruntSpawnConfig::~CGruntSpawnConfig() {
    Clear();
}

// @early-stop
RVA(0x0011adc0, 0x44)
BOOL CGruntSpawnConfig::Init(CGruntzMgr* owner) {
    if (owner == 0) {
        return 0;
    }
    m_configTree = 0;
    m_voices[0] = 0;
    m_voices[1] = 0;
    m_streams[0] = 0;
    m_streams[1] = 0;
    m_owner = owner;
    m_voiceVolume = 0x64;
    m_configTree = owner->m_world;
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
    if (m_configTree != 0 && m_configTree->m_soundStream != 0) {
        StreamVoice** p = m_streams;
        for (i32 k = 0; k < 2; k++) {
            if (p[0] != 0) {
                m_configTree->m_soundStream->DestroyVoice(p[0]);
                p[0] = 0;
            }
            p++;
        }
    }
    m_owner = 0;
    m_configTree = 0;
    m_voices[0] = 0;
    m_voices[1] = 0;
    m_streams[0] = 0;
    m_streams[1] = 0;
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
        if (got == 0) {
            return 0;
        }
    }
    return 1;
}

// @early-stop
RVA(0x0011af90, 0xb)
void CGruntSpawnConfig::ClearSprites() {
    CGruntVoice** p = m_voices;
    for (i32 i = 0; i < 2; i++) {
        p[i] = 0;
    }
}

// @early-stop

RVA(0x0011afb0, 0x321)
BOOL CGruntSpawnConfig::LoadGruntSpawnConfig(
    CGrunt* who,
    i32 cue,
    i32 which,
    i32 priority,
    i32 percent
) {
    if (m_voices[0] == 0 && !LoadGruntVoices()) {
        return 0;
    }
    if (who == 0) {
        return 0;
    }
    if (!IsReady()) {
        return 0;
    }
    i32 voiceId = GetButeSlot(who, cue);
    CString local_10;
    CString local_14;
    local_14.Format("SG%i", voiceId);
    local_10.Format("G%i", cue);
    if (percent == -1) {
        percent = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(local_14), "Per", -1);
        if (percent == -1) {
            percent = g_buteMgr.GetIntDef("GruntPercent", static_cast<LPCTSTR>(local_10), 0);
        }
    }
    if (percent < 100 && percent < g_gameReg->Rand() % 0x65) {
        return 0;
    }
    if (priority == -1) {
        priority = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(local_14), "Pri", -1);
        if (priority == -1) {
            priority = g_buteMgr.GetIntDef("GruntPriority", static_cast<LPCTSTR>(local_10), 1);
        }
    }
    CGruntVoice** voices = m_voices;
    for (i32 i = 0; i < 2; i++) {
        if (priority <= voices[i]->m_playFlags) {
            return 0;
        }
    }
    CParseSource* src = PickWeighted(voiceId, which);
    if (src == 0 || m_configTree->m_soundStream == 0) {
        return 0;
    }
    CGruntVoice* v8 = m_voices[0];
    CGruntVoice* v0c = m_voices[1];
    i32 a = v8->m_playFlags;
    i32 b = v0c->m_playFlags;
    i32 c = v8->m_source;
    i32 d = v0c->m_source;
    StreamVoice** streams = m_streams;

    CGameObject* gate = who->m_object;
    i32 chosen;
    if (b < a) {
        chosen = 1;
        if (c == gate->m_objectId) {
            chosen = 0;
            if (b != 0 && streams[1] != 0) {
                (static_cast<DirectSoundMgr*>(streams[1]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (a != 0 && streams[0] != 0) {
            (static_cast<DirectSoundMgr*>(streams[0]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    } else {
        chosen = 0;
        if (d == gate->m_objectId) {
            chosen = 1;
            if (a != 0 && streams[0] != 0) {
                (static_cast<DirectSoundMgr*>(streams[0]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (b != 0 && streams[1] != 0) {
            (static_cast<DirectSoundMgr*>(streams[1]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    }
    if (streams[chosen] == 0) {
        streams[chosen] =
            m_configTree->m_soundStream->OpenStream(src, 0x5000, 0x1400, 0x100e0, 0, 0);
        if (streams[chosen] == 0) {
            return 0;
        }
    }
    StreamVoice* stream = streams[chosen];
    i32 vol = m_voiceVolume;
    stream->m_feeder.Pause();
    if (stream->SetSource(src) != 0) {
        stream->Configure(vol, 0, 0, 0);
    }
    CGruntVoice* voice = voices[chosen];
    return voice->Setup(gate->m_objectId, stream, priority, 0) != 0;
}

// @early-stop
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
    if (voices[0] == 0 && !LoadGruntVoices()) {
        return 0;
    }
    if (who == 0 && objId == 0) {
        return 0;
    }
    if (!IsReady()) {
        return 0;
    }
    CString local_10;
    local_10.Format("SG%i", voiceId);
    if (percent == -1) {
        percent = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(local_10), "Per", 100);
    }
    if (percent < 100 && GameRand() % 0x65 > percent) {
        return 0;
    }
    if (priority == -1) {
        priority = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(local_10), "Pri", 1);
    }
    for (i32 i = 0; i < 2; i++) {
        if (voices[i]->m_playFlags >= priority) {
            return 0;
        }
    }
    i32 id = 0;
    if (objId == 0 && who != 0) {
        id = who->m_object->m_objectId;
    }
    CParseSource* src = PickWeighted(voiceId, which);
    if (src == 0) {
        return 0;
    }
    if (m_configTree->m_soundStream == 0) {
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
            if (b != 0 && m_streams[1] != 0) {
                (static_cast<DirectSoundMgr*>(m_streams[1]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (a != 0 && m_streams[0] != 0) {
            (static_cast<DirectSoundMgr*>(m_streams[0]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    } else {
        chosen = 0;
        if (d == id) {
            chosen = 1;
            if (a != 0 && m_streams[0] != 0) {
                (static_cast<DirectSoundMgr*>(m_streams[0]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }

        } else if (b != 0 && id != 0) {
            (static_cast<DirectSoundMgr*>(m_streams[1]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    }
    if (m_streams[chosen] == 0) {
        m_streams[chosen] =
            m_configTree->m_soundStream->OpenStream(src, 0x5000, 0x1400, 0x100e0, 0, 0);
        if (m_streams[chosen] == 0) {
            return 0;
        }
    }
    StreamVoice* stream = m_streams[chosen];
    i32 vol = m_voiceVolume;
    stream->m_feeder.Pause();
    if (stream->SetSource(src) != 0 && stream->Configure(vol, 0, 0, 0) != 0) {
        return m_voices[chosen]->Setup(id, stream, priority, 0) != 0;
    }
    return 0;
}

// @early-stop
RVA(0x0011b7c0, 0x304)
i32 CGruntSpawnConfig::SpawnVoiceDriver(
    i32 objId,
    i32 voiceId,
    i32 which,
    i32 priority,
    i32 percent
) {
    CGruntVoice** voices = m_voices;
    if (voices[0] == 0 && !LoadGruntVoices()) {
        return 0;
    }
    if (objId == 0) {
        return 0;
    }
    if (!IsReady()) {
        return 0;
    }
    CString local_10;
    local_10.Format("SG%i", voiceId);
    if (percent == -1) {
        percent = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(local_10), "Per", 100);
    }
    if (percent < 100 && GameRand() % 0x65 > percent) {
        return 0;
    }
    if (priority == -1) {
        priority = g_buteMgr.GetIntDef(static_cast<LPCTSTR>(local_10), "Pri", 1);
    }
    for (i32 i = 0; i < 2; i++) {
        if (voices[i]->m_playFlags >= priority) {
            return 0;
        }
    }
    CParseSource* src = PickWeighted(voiceId, which);
    if (src == 0) {
        return 0;
    }
    if (m_configTree->m_soundStream == 0) {
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
            if (b != 0 && m_streams[1] != 0) {
                (static_cast<DirectSoundMgr*>(m_streams[1]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (a != 0 && m_streams[0] != 0) {
            (static_cast<DirectSoundMgr*>(m_streams[0]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    } else {
        chosen = 0;
        if (d == objId) {
            chosen = 1;
            if (a != 0 && m_streams[0] != 0) {
                (static_cast<DirectSoundMgr*>(m_streams[0]))
                    ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
            }
        } else if (b != 0 && m_streams[1] != 0) {
            (static_cast<DirectSoundMgr*>(m_streams[1]))
                ->SetVolumeByIndex(g_gameReg->m_voiceVolume / 2);
        }
    }
    if (m_streams[chosen] == 0) {
        m_streams[chosen] =
            m_configTree->m_soundStream->OpenStream(src, 0x5000, 0x1400, 0x100e0, 0, 0);
        if (m_streams[chosen] == 0) {
            return 0;
        }
    }
    StreamVoice* stream = m_streams[chosen];
    i32 vol = m_voiceVolume;
    stream->m_feeder.Pause();
    if (stream->SetSource(src) != 0 && stream->Configure(vol, 0, 0, 0) != 0) {
        return m_voices[chosen]->Setup(objId, stream, priority, 1) != 0;
    }
    return 0;
}

RVA(0x0011bba0, 0x280)
i32 CGruntSpawnConfig::GetButeSlot(CGrunt* config, i32 cue) {
    if (config == 0) {
        return 0;
    }
    if (config->m_gruntKind == 0x3a) {
        return VOICE_CUES_PER_BAND * 19 + cue;
    }
    if (config->m_gruntKind == 0x39) {
        return VOICE_CUES_PER_BAND * 13 + cue;
    }
    switch (static_cast<u32>(config->m_entranceReason)) {
        case 0:
            return VOICE_CUES_PER_BAND * 17 + cue;
        case 1:
            return VOICE_CUES_PER_BAND * 3 + cue;
        case 2:
            return VOICE_CUES_PER_BAND * 4 + cue;
        case 3:
            return VOICE_CUES_PER_BAND * 5 + cue;
        case 4:
            return VOICE_CUES_PER_BAND * 6 + cue;
        case 5:
            return VOICE_CUES_PER_BAND * 7 + cue;
        case 6:
            return VOICE_CUES_PER_BAND * 8 + cue;
        case 7:
            return VOICE_CUES_PER_BAND * 10 + cue;
        case 8:
            return VOICE_CUES_PER_BAND * 11 + cue;
        case 9:
            return VOICE_CUES_PER_BAND * 12 + cue;
        case 10:
            return VOICE_CUES_PER_BAND * 16 + cue;
        case 11:
            return VOICE_CUES_PER_BAND * 20 + cue;
        case 12:
            return VOICE_CUES_PER_BAND * 22 + cue;
        case 13:
            return VOICE_CUES_PER_BAND * 23 + cue;
        case 14:
            return VOICE_CUES_PER_BAND * 24 + cue;
        case 15:
            return VOICE_CUES_PER_BAND * 25 + cue;
        case 16:
            return VOICE_CUES_PER_BAND * 27 + cue;
        case 17:
            return VOICE_CUES_PER_BAND * 28 + cue;
        case 18:
            if (config->m_coordToggle != 0) {
                return VOICE_CUES_PER_BAND * 30 + cue;
            }
            return VOICE_CUES_PER_BAND * 29 + cue;
        case 19:
            return VOICE_CUES_PER_BAND * 31 + cue;
        case 20:
            return VOICE_CUES_PER_BAND * 32 + cue;
        case 21:
            return VOICE_CUES_PER_BAND * 33 + cue;
        case 22:
            return VOICE_CUES_PER_BAND * 34 + cue;
        case 23:
            return cue;
        case 24:
            return VOICE_CUES_PER_BAND * 1 + cue;
        case 25:
            return VOICE_CUES_PER_BAND * 2 + cue;
        case 26:
            return VOICE_CUES_PER_BAND * 9 + cue;
        case 27:
            return VOICE_CUES_PER_BAND * 14 + cue;
        case 28:
            return VOICE_CUES_PER_BAND * 15 + cue;
        case 29:
            return VOICE_CUES_PER_BAND * 18 + cue;
        case 30:
            return VOICE_CUES_PER_BAND * 21 + cue;
        case 31:
            return VOICE_CUES_PER_BAND * 26 + cue;
        case 32:
            return VOICE_CUES_PER_BAND * 35 + cue;
        default:
            return 0;
    }
}

// @early-stop
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
    if (list == 0) {
        return 0;
    }

    i32 pick = which;
    if (pick == -1 || pick >= list->m_list.GetCount()) {
        i32 hi = list->m_list.GetCount() - 1;

        CGruntzMgr* reg = g_gameReg;
        i32 span = hi + 1;
        if (span == 0) {
            i32 seed;
            if (!(g_randSeeded & 1)) {
                g_randSeeded |= 1;
                seed = timeGetTime();
            } else {
                seed = g_randSeed;
            }
            g_randSeed = seed * 214013 + 2531011;
            pick = (g_randSeed & 0x10000) ? 0 : hi;
        } else {
            pick = reg->Rand() % span;
        }
        if (list->m_list.GetCount() > 1) {
            i32 tries = 5;
            while (pick == list->m_lastPicked && tries > 0) {
                i32 rehi = list->m_list.GetCount() - 1;
                i32 respan = rehi + 1;

                if (respan == 0) {
                    i32 seed;
                    if (g_randSeeded & 1) {
                        seed = g_randSeed;
                    } else {
                        g_randSeeded |= 1;
                        seed = timeGetTime();
                    }
                    g_randSeed = seed * 214013 + 2531011;
                    pick = (g_randSeed & 0x10000) ? 0 : rehi;
                } else {
                    i32 seed;
                    if (g_randSeeded & 1) {
                        seed = g_randSeed;
                    } else {
                        g_randSeeded |= 1;
                        seed = timeGetTime();
                    }
                    g_randSeed = seed * 214013 + 2531011;
                    pick = ((g_randSeed >> 0x10) & 0x7fff) % respan;
                }
                tries--;
            }
        }
    }

    list->m_lastPicked = pick;
    CSpawnEntry* entry;
    if (pick >= list->m_list.GetCount()) {
        entry = 0;
    } else {

        CGruntCoordList* nodes = static_cast<CGruntCoordList*>(&list->m_list);
        POSITION& cursor = list->m_cursor;
        cursor = list->m_list.GetHeadPosition();
        if (cursor == 0) {
            entry = 0;
        } else {
            entry = static_cast<CSpawnEntry*>(nodes->NextData(cursor));
        }
        for (i32 i = pick; i > 0; i--) {
            if (cursor == 0) {
                entry = 0;
            } else {
                entry = static_cast<CSpawnEntry*>(nodes->NextData(cursor));
            }
        }
    }
    if (entry == 0) {
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

RVA(0x0011c560, 0x91)
void CSpawnList::AddVoiceSound(CString s, i32 flag) {
    CSpawnEntry* node = new CSpawnEntry(s, flag);
    if (node != 0) {
        m_list.AddTail(node);
    }
}

RVA(0x0011c6c0, 0x27)
i32 CGruntSpawnConfig::AnyVoicePlaying() {
    i32 i = 0;
    CGruntVoice** p = m_voices;
    for (; i < 2; i++, p++) {
        if (*p != 0 && (*p)->m_playFlags != 0) {
            return 1;
        }
    }
    return 0;
}

RVA(0x0011c700, 0x20)
i32 CGruntSpawnConfig::VoicePlaying(i32 i) {
    CGruntVoice* v = m_voices[i];
    if (v != 0 && v->m_playFlags != 0) {
        return 1;
    }
    return 0;
}

RVA(0x0011c730, 0x5c)
void CGruntSpawnConfig::StopVoice(i32 id) {
    i32 tag08 = m_voices[0]->m_source;
    i32 tag0c = m_voices[1]->m_source;
    if (tag08 == id) {
        if (m_streams[0] != 0) {
            m_streams[0]->m_feeder.Pause();
        }
        if (m_voices[0] != 0) {
            m_voices[0]->Reset();
        }
    } else if (tag0c == id) {
        if (m_streams[1] != 0) {
            m_streams[1]->m_feeder.Pause();
        }
        if (m_voices[1] != 0) {
            m_voices[1]->Reset();
        }
    }
}

RVA(0x0011c7b0, 0x2d)
void CGruntSpawnConfig::PauseAllVoices() {

    for (i32 k = 0; k < 2; k++) {
        if (m_streams[k] != 0) {
            m_streams[k]->m_feeder.Pause();
        }
        if (m_voices[k] != 0) {
            m_voices[k]->Reset();
        }
    }
}

RVA(0x0011c7f0, 0x2b)
void CGruntSpawnConfig::ResetPicks() {
    PauseAllVoices();
    for (i32 i = 0; i < m_voiceLists.GetSize(); i++) {
        CSpawnList* e = static_cast<CSpawnList*>(m_voiceLists[i]);
        if (e != 0) {
            e->m_lastPicked = -1;
        }
    }
}

RVA(0x0011c830, 0x12)
BOOL CGruntSpawnConfig::IsReady() {
    return m_owner->m_isVoiceEnabled != 0;
}
