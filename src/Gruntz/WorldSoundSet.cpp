#include <rva.h>

#include <Gruntz/WorldSoundSet.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawSubMgrLeafScanInline.h>
#include <Gruntz/AmbientSound.h>
#include <Gruntz/GameObjectFactory.h>
#include <Gruntz/GameRand.h>
#include <Gruntz/GruntDirStatics.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/RandomAmbientSound.h>
#include <Gruntz/SpriteStateFlags.h>
#include <Gruntz/UserLogic.h>
#include <Rez/RezMgr.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wwd/WwdGameObjectFamily.h>

#include <math.h>
#include <new>

DATA(0x0022990c)
i32 g_posSoundReq;

RVA(0x0000b5e0, 0x29)
i32 CWorldSoundSet::Init(CDDrawSubMgrLeafScan* world, i32 volume) {
    if (world == NULL) {
        return 0;
    }
    m_world = world;
    m_volume = volume;
    m_active = 1;
    m_listenerX = 0;
    m_listenerY = 0;
    return 1;
}

RVA(0x0000b620, 0x26)
void CWorldSoundSet::Deactivate() {
    if (m_world != NULL && m_world->m_soundStream != NULL) {
        m_world->m_soundStream->FreeSamples();
    }
    Teardown();
    m_world = NULL;
}

RVA(0x0000b660, 0x2b)
void CWorldSoundSet::Teardown() {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CAmbientSound* ch = static_cast<CAmbientSound*>(m_list.GetNext(pos));
        if (ch != NULL) {
            delete ch;
        }
    }
    m_list.RemoveAll();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0000b6a0, 0x83)
CAmbientSound* CWorldSoundSet::CreateAmbientFromKey(
    const char* key,
    i32 level,
    RECT* box,
    i32 scaleB,
    i32 unused
) {
    CAmbientSound* obj = new CAmbientSound;
    if (obj == NULL) {
        return 0;
    }
    if (obj->InitFromKey(m_world, key, level, m_volume, box, scaleB) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

RVA_COMPGEN(0x0000b760, 0x1e, ??_GCAmbientSound@@UAEPAXI@Z)

RVA_COMPGEN(0x0000b790, 0xf, ??1CAmbientSound@@UAE@XZ)

RVA(0x0000b7b0, 0x80)
CAmbientSound* CWorldSoundSet::CreateAmbientFromSound(
    DirectSoundMgr* mgr,
    i32 level,
    RECT* box,
    i32 scaleB,
    i32 unused
) {
    CAmbientSound* obj = new CAmbientSound;
    if (obj == NULL) {
        return 0;
    }
    if (obj->InitFromSound(mgr, level, m_volume, box, scaleB) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0000b850, 0x83)
CAmbientPosSound* CWorldSoundSet::CreatePositionedFromKey(
    const char* key,
    i32 level,
    AmbientPoint* pos,
    i32 scaleB,
    i32 unused
) {
    CAmbientPosSound* obj = new CAmbientPosSound;
    if (obj == NULL) {
        return 0;
    }
    if (obj->InitFromKey(m_world, key, level, m_volume, pos, scaleB) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

RVA_COMPGEN(0x0000b910, 0x1e, ??_GCAmbientPosSound@@UAEPAXI@Z)

RVA_COMPGEN(0x0000b940, 0xf, ??1CAmbientPosSound@@UAE@XZ)

RVA(0x0000b960, 0x80)
CAmbientPosSound* CWorldSoundSet::CreatePositionedFromSound(
    DirectSoundMgr* mgr,
    i32 level,
    AmbientPoint* pos,
    i32 scaleB,
    i32 unused
) {
    CAmbientPosSound* obj = new CAmbientPosSound;
    if (obj == NULL) {
        return 0;
    }
    if (obj->InitFromSound(mgr, level, m_volume, pos, scaleB) == 0) {
        delete obj;
        return 0;
    }
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0000ba00, 0xc6)
CRandomAmbientSound* CWorldSoundSet::CreateRandomBox(
    const char* key,
    i32 level,
    RECT* box,
    i32 scaleB,
    i32 intervalLoA,
    i32 intervalHiA,
    i32 intervalLoB,
    i32 intervalHiB,
    i32 unused
) {
    if (static_cast<u32>(intervalHiA) < static_cast<u32>(intervalLoA)) {
        return 0;
    }
    if (static_cast<u32>(intervalHiB) < static_cast<u32>(intervalLoB)) {
        return 0;
    }
    CRandomAmbientSound* obj = new CRandomAmbientSound;
    if (obj == NULL) {
        return 0;
    }
    if (obj->InitFromKey(m_world, key, level, m_volume, box, scaleB) == 0) {
        delete obj;
        return 0;
    }
    obj->InitCycleTiming(intervalLoA, intervalHiA, intervalLoB, intervalHiB);
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

RVA_COMPGEN(0x0000bb10, 0x1e, ??_GCRandomAmbientSound@@UAEPAXI@Z)
RVA_COMPGEN(0x0000bb40, 0xf, ??1CRandomAmbientSound@@UAE@XZ)

RVA(0x0000bb60, 0x9b)
CRandomAmbientSound* CWorldSoundSet::CreateRandom(
    DirectSoundMgr* mgr,
    i32 level,
    RECT* box,
    i32 scaleB,
    i32 intervalLoA,
    i32 intervalHiA,
    i32 intervalLoB,
    i32 intervalHiB,
    i32 unused
) {
    CRandomAmbientSound* obj = new CRandomAmbientSound;
    if (obj == NULL) {
        return 0;
    }
    if (obj->InitFromSound(mgr, level, m_volume, box, scaleB) == 0) {
        delete obj;
        return 0;
    }
    obj->InitCycleTiming(intervalLoA, intervalHiA, intervalLoB, intervalHiB);
    obj->m_listNode = m_list.AddTail(obj);
    return obj;
}

RVA(0x0000bc30, 0x3a)
void CWorldSoundSet::Restart(i32 volume) {
    m_volume = volume;
    if (m_world->m_soundStream != NULL) {
        m_world->m_soundStream->FreeSamples();
    }
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CAmbientSound* ch = static_cast<CAmbientSound*>(m_list.GetNext(pos));
        if (ch != NULL) {
            ch->Recompute(static_cast<i32>(volume));
        }
    }
}

RVA(0x0000bc80, 0x44)
void CWorldSoundSet::Stop() {
    if (m_world != NULL && m_world->m_soundStream != NULL) {
        m_world->m_soundStream->FreeSamples();
    }
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CAmbientSound* ch = static_cast<CAmbientSound*>(m_list.GetNext(pos));
        if (ch != NULL && ch->m_voice != NULL) {
            ch->m_voice->StopAndRewind();
            ch->m_isPlaying = 0;
        }
    }
}

RVA(0x0000bcf0, 0x43)
void CWorldSoundSet::Resume() {
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CAmbientSound* ch = static_cast<CAmbientSound*>(m_list.GetNext(pos));
        if (ch != NULL) {
            ch->m_isPlaying = 0;
            ch->Update(m_listenerX, m_listenerY, 1);
        }
    }

    PurgeVoices(m_world);
}

RVA(0x0000bd60, 0x4b)
void CWorldSoundSet::Retune(i32 x, i32 y) {
    m_listenerX = x;
    m_listenerY = y;
    POSITION pos = m_list.GetHeadPosition();
    while (pos != NULL) {
        CAmbientSound* ch = static_cast<CAmbientSound*>(m_list.GetNext(pos));
        if (ch != NULL) {
            ch->Update(x, y, 0);
        }
    }

    PurgeVoices(m_world);
}

RVA(0x0000bdd0, 0x53)
i32 CAmbientSound::InitFromKey(
    CDDrawSubMgrLeafScan* world,
    const char* key,
    i32 level,
    i32 master,
    RECT* box,
    i32 scaleB
) {
    AmbSoundRecord* out = NULL;
    MapLookup(world->m_cues, key, out);
    if (out == NULL) {
        return 0;
    }
    return InitFromSound(out->m_mgr, level, master, box, scaleB);
}

RVA(0x0000be50, 0x8f)
i32 CAmbientSound::InitFromSound(
    DirectSoundMgr* mgr,
    i32 level,
    i32 master,
    RECT* box,
    i32 scaleB
) {
    if (mgr == NULL) {
        return 0;
    }
    m_voice = mgr;
    m_level = level;
    m_scaleA = master;
    m_scaleB = scaleB;
    m_panIndex = 0;
    m_isPlaying = 0;
    RECT* p = &m_box1;
    if (box != NULL) {
        *p = *box;
    } else {
        p->left = COORD_UNSET;
    }
    if (p->left == 0 && m_box1.top == 0 && m_box1.right == 0 && m_box1.bottom == 0) {
        p->left = COORD_UNSET;
    }
    m_box2.left = COORD_UNSET;
    return 1;
}

RVA(0x0000bf10, 0x72)
void CAmbientSound::Recompute(i32 master) {
    if (m_scaleA == master) {
        return;
    }
    i32 mult = m_level;
    m_scaleA = master;
    i32 v = ScaleVolume(mult);
    m_voice->SetVolumeByIndex(v);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0000bfb0, 0xa9)
void CAmbientSound::Restart() {
    DirectSoundMgr* voice = m_voice;
    i32 pos = m_level;
    if (voice == NULL) {
        return;
    }
    if (m_isPlaying != 0) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    if (g_gameReg->m_inputState->m_active == 0) {
        return;
    }
    m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
    m_level = pos;
    i32 v = ScaleVolume(pos);
    m_voice->SetVolumeByIndex(v);
    m_level = pos;
    m_isPlaying = 1;
}

RVA(0x0000c090, 0x118)
void CAmbientSound::Update(i32 x, i32 y, i32 force) {
    i32 inRange;
    if (m_box1.left == COORD_UNSET) {

        if (m_isPlaying != 0) {
            return;
        }
        DirectSoundMgr* voice = m_voice;
        i32 lvl = m_level;
        if (voice == NULL) {
            return;
        }
        if (lvl == 0) {
            return;
        }
        if (g_gameReg->m_soundEnabled == 0) {
            return;
        }
        if (g_gameReg->m_inputState->m_active == 0) {
            return;
        }
        voice->ApplyAndPlay(1, m_panIndex, 0, 1);
        SetLevel(0x64, 0, 0);
        m_level = 0x64;
        m_isPlaying = 1;
        return;
    }

    if (x > m_box1.left && x < m_box1.right && y > m_box1.top && y < m_box1.bottom) {
        inRange = 1;
    } else if (m_box2.left != COORD_UNSET && x > m_box2.left && x < m_box2.right && y > m_box2.top
               && y < m_box2.bottom) {
        inRange = 1;
    } else {
        inRange = 0;
    }

    if (m_isPlaying == 0) {

        if (inRange == 0) {
            return;
        }
        if (g_gameReg->m_soundEnabled == 0) {
            return;
        }
        if (g_gameReg->m_inputState->m_active == 0) {
            return;
        }
        if (force != 0) {
            if (m_voice == NULL) {
                return;
            }
            m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
            SetLevel(0x64, 0, 0);
            m_level = 0x64;
            m_isPlaying = 1;
        } else {
            Fade(1, 0x64, 0x3e8);
        }
    } else {

        if (inRange != 0) {
            return;
        }
        Fade(0, 0, 0x3e8);
    }
}

RVA(0x0000c200, 0x7e)
i32 CAmbientSound::SetLevel(i32 value, i32 mode, i32 extra) {
    m_level = value;
    i32 v = ScaleVolume(value);
    if (mode == 0) {
        return m_voice->SetVolumeByIndex(v);
    }
    return m_voice->CloneAndPlay(v, mode, extra);
}

// @early-stop
RVA(0x0000c2a0, 0x19e)
void CAmbientSound::Fade(i32 playFlag, i32 level, i32 mode) {
    if (m_voice == NULL) {
        return;
    }
    if (playFlag != 0) {

        if (m_isPlaying != 0) {
            return;
        }
        if (g_gameReg->m_soundEnabled == 0) {
            return;
        }
        if (g_gameReg->m_inputState->m_active == 0) {
            return;
        }
        if (mode == 0) {
            m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
            i32 t = m_scaleA;
            m_level = level;
            if (t > 5) {
                t -= 0xf;
            }
            i32 v = (t * level) / 100;
            if (m_scaleB > 0) {
                v = (v * m_scaleB) / 100;
            }
            if (v < 0) {
                v = 0;
            } else if (v > 0x64) {
                v = 0x64;
            }
            m_voice->SetVolumeByIndex(v);
            m_level = level;
            m_isPlaying = 1;
            return;
        }

        m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
        i32 t = m_scaleA;
        m_level = level;
        if (t > 5) {
            t -= 0xf;
        }
        i32 v = (t * level) / 100;
        if (m_scaleB > 0) {
            v = (v * m_scaleB) / 100;
        }
        if (v < 0) {
            v = 0;
        } else if (v > 0x64) {
            v = 0x64;
        }
        m_voice->CloneAndPlay(v, mode, 0);
        m_level = level;
        m_isPlaying = 1;
        return;
    }

    if (m_isPlaying == 0) {
        return;
    }
    if (mode == 0) {
        m_voice->StopAndRewind();
        m_isPlaying = 0;
        return;
    }
    m_level = 0;
    m_voice->CloneAndPlay(0, mode, 1);
    m_isPlaying = 0;
}

RVA(0x0000c4b0, 0x53)
i32 CAmbientPosSound::InitFromKey(
    CDDrawSubMgrLeafScan* world,
    const char* key,
    i32 level,
    i32 master,
    AmbientPoint* pos,
    i32 scaleB
) {
    AmbSoundRecord* out = NULL;
    MapLookup(world->m_cues, key, out);
    if (out == NULL) {
        return 0;
    }
    return InitFromSound(out->m_mgr, level, master, pos, scaleB);
}

RVA(0x0000c530, 0x51)
i32 CAmbientPosSound::InitFromSound(
    DirectSoundMgr* mgr,
    i32 level,
    i32 master,
    AmbientPoint* pos,
    i32 scaleB
) {
    if (mgr == NULL) {
        return 0;
    }
    if (pos == NULL) {
        return 0;
    }
    m_voice = mgr;
    m_level = level;
    m_scaleA = master;
    m_panIndex = 0;
    m_scaleB = scaleB;
    m_isPlaying = 0;
    m_position = *pos;
    return 1;
}

RVA(0x0000c5b0, 0x1df)
void CAmbientPosSound::Update(i32 x, i32 y, i32 force) {
    i32 dx = abs(m_position.x - x);
    i32 dy = abs(m_position.y - y);
    i32 dist2 = dx * dx + dy * dy;
    if (dx > 0x280 || dy > 0x280) {
        if (m_voice != NULL && m_isPlaying != 0) {
            m_voice->StopAndRewind();
            m_isPlaying = 0;
        }
        return;
    }

    i32 dist = static_cast<i32>(sqrt(static_cast<double>(dist2)));
    i32 vol = 0x64 - dist / 12;
    if (vol > 0x64) {
        vol = 0x64;
    } else if (vol < 0) {
        vol = 0;
    }
    i32 pan = dx / 4;
    if (pan > 0x64) {
        pan = 0x64;
    } else if (pan < 0) {
        pan = 0;
    }
    if (m_position.x < x) {
        pan = -pan;
    }

    {
        m_level = vol;
        i32 v = ScaleVolume(vol);
        m_voice->SetVolumeByIndex(v);
    }
    m_panIndex = pan;
    m_voice->SetPanByIndex(pan);

    if (m_isPlaying != 0) {
        return;
    }
    if (m_voice == NULL) {
        return;
    }
    if (g_gameReg->m_soundEnabled == 0) {
        return;
    }
    if (g_gameReg->m_inputState->m_active == 0) {
        return;
    }
    m_voice->ApplyAndPlay(1, m_panIndex, 0, 1);
    {
        m_level = vol;
        i32 v = ScaleVolume(vol);
        m_voice->SetVolumeByIndex(v);
    }
    m_level = vol;
    m_isPlaying = 1;
}

RVA(0x0000c810, 0x18)
i32 CreateGlobalAmbientSound(CGameObject* obj) {
    g_posSoundReq = 1;
    return CreateAmbientSound(obj);
}

RVA(0x0000c840, 0x13d)
i32 CreateAmbientSound(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_animWorker;
    CWwdGameObjectA* sprite = static_cast<CWwdGameObjectA*>(obj);
    if (aux->m_actKey == 0) {
        obj->m_flags |= 1;
        obj->m_stateFlags |= SPRITE_STATE_HIDDEN;
        if (aux->m_notify == CreateGlobalAmbientSound) {
            obj->m_flags |= 2;
        } else {
            obj->m_flags &= ~2;
        }
        LeafCue* layer = sprite->m_soundCue;
        if (layer && g_gameReg) {
            RECT rc;
            CopyRect(&rc, &obj->m_area);
            if (aux->m_minX > 0 || aux->m_maxX > 0) {
                SetRect(&rc, aux->m_minX, aux->m_minY, aux->m_maxX, aux->m_maxY);
            }
            if (g_gameReg->m_inputState) {
                CAmbientSound* placed;
                if (obj->m_extent.top > 0) {
                    placed = g_gameReg->m_inputState->CreateRandom(
                        layer->m_sound,
                        0x64,
                        &rc,
                        obj->m_damage,
                        obj->m_extent.left,
                        obj->m_extent.top,
                        obj->m_extent.right,
                        obj->m_extent.bottom,
                        0
                    );
                } else {
                    placed =
                        g_gameReg->m_inputState
                            ->CreateAmbientFromSound(layer->m_sound, 0x64, &rc, obj->m_damage, 0);
                }
                if (placed && obj->m_switchRect.top > 0) {
                    placed->m_box2 = obj->m_switchRect;
                }
            }
        }
        obj->m_flags |= 0x10000;
        aux->SetActKey(5);
    }
    return 1;
}

RVA(0x0000c9d0, 0x18)
i32 CreateAmbientPosSound(CGameObject* obj) {
    g_posSoundReq = 2;
    return CreateSpotAmbientSound(obj);
}

RVA(0x0000ca00, 0xf0)
i32 CreateSpotAmbientSound(CGameObject* obj) {
    AnimWorkerObj* aux = obj->m_animWorker;
    CWwdGameObjectA* sprite = static_cast<CWwdGameObjectA*>(obj);
    i32 state = aux->m_actKey;
    if (state != 0) {
        if (state != AMBIENT_SOUND_ACTIVE) {
            return 1;
        }
        CAmbientPosSound* sound = aux->m_positionedSound;
        if (sound == NULL) {
            return 1;
        }

        CWorldSoundSet* set = g_gameReg->m_inputState;
        if (sound->m_voice != NULL) {
            sound->m_voice->StopAndRewind();
            sound->m_isPlaying = 0;
        }
        if (sound->m_listNode != NULL) {
            set->m_list.RemoveAt(sound->m_listNode);
            delete sound;
        }
        aux->m_positionedSound = NULL;
        aux->SetActKey(0);
        return 1;
    }

    obj->m_stateFlags |= SPRITE_STATE_HIDDEN;
    obj->m_flags = (obj->m_flags & ~2) | 0x100001;
    aux->m_positionedSound = NULL;
    LeafCue* layer = sprite->m_soundCue;
    if (layer != NULL && g_gameReg != NULL) {

        CWorldSoundSet* set = g_gameReg->m_inputState;
        if (set != NULL) {
            AmbientPoint pt;
            pt.x = obj->m_screenX;
            pt.y = obj->m_screenY;

            CAmbientPosSound* v =
                set->CreatePositionedFromSound(layer->m_sound, 0x64, &pt, obj->m_damage, 0);
            if (v != NULL) {
                aux->m_positionedSound = v;
            }
        }
    }
    aux->SetActKey(5);
    return 1;
}

static inline i32 RandRange(CGruntzMgr* mgr, i32 lo, i32 hi) {
    i32 range = hi - lo + 1;
    if (range == 0) {
        return (mgr->Rand() & 1) ? lo : hi;
    }
    return mgr->Rand() % range + lo;
}

// @early-stop
RVA(0x0000cb30, 0x168)
void CRandomAmbientSound::Update(i32 x, i32 y, i32 force) {

    i32 b1 = m_box1.left;
    i32 inBox = 0;
    if (b1 == COORD_UNSET) {
        inBox = 1;
    } else if (x > b1 && x < m_box1.right && y > m_box1.top && y < m_box1.bottom) {
        inBox = 1;
    } else {
        i32 b2 = m_box2.left;
        if (b2 != COORD_UNSET && x > b2 && x < m_box2.right && y > m_box2.top
            && y < m_box2.bottom) {
            inBox = 1;
        }
    }

    if (inBox == 0) {
        if (m_isPlaying != 0 && m_voice != NULL) {
            SetLevel(0, 0x3e8, 1);
            m_isPlaying = 0;
        }
        m_phase = 0;
        return;
    }

    if (force != 0 && m_phase != 0 && m_isPlaying != 0) {
        return;
    }

    if (g_frameDelta >= static_cast<u32>(m_countdownMs)) {
        m_countdownMs = 0;
    } else {
        m_countdownMs = m_countdownMs - g_frameDelta;
    }
    if (m_countdownMs != 0) {
        return;
    }

    m_phase ^= 1;
    if (m_phase != 0) {
        i32 r = RandRange(g_gameReg, m_playDurationMin, m_playDurationMax);
        m_countdownMs = r;
        i32 half = static_cast<u32>(r) >> 1;
        if (half > 0x3e8) {
            half = 0x3e8;
        }
        Fade(1, 0x64, half);
    } else {
        i32 r = RandRange(g_gameReg, m_silenceDurationMin, m_silenceDurationMax);
        m_countdownMs = r;
        i32 half = static_cast<u32>(r) >> 1;
        if (half > 0x3e8) {
            half = 0x3e8;
        }
        Fade(0, 0x64, half);
    }
}

RVA(0x0000cd00, 0x46)
i32 CGruntzMgr::Rand() {
    i32 seed;
    return GetRandomNumber();
}

RVA(0x0000cd70, 0xe5)
void CRandomAmbientSound::InitCycleTiming(
    i32 playDurationMin,
    i32 playDurationMax,
    i32 silenceDurationMin,
    i32 silenceDurationMax
) {
    m_playDurationMin = playDurationMin;
    m_playDurationMax = playDurationMax;
    m_silenceDurationMin = silenceDurationMin;
    m_silenceDurationMax = silenceDurationMax;
    i32 span = playDurationMax - playDurationMin + 1;
    i32 random;
    if (span == 0) {
        random = GetRandomNumber();
        if (random & 1) {
            i32 countdown = playDurationMin;
            m_phase = 1;
            m_countdownMs = countdown;
        } else {
            i32 countdown = playDurationMax;
            m_phase = 1;
            m_countdownMs = countdown;
        }
        return;
    }
    random = GetRandomNumber();
    m_phase = 1;
    m_countdownMs = playDurationMin + random % span;
}
