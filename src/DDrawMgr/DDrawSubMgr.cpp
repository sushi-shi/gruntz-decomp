#include <StdAfx.h>

#include <Ints.h>

#include <DDrawMgr/DDrawSubMgr.h>

#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawPaletteRegistry.h>
#include <DDrawMgr/DDrawPlacedWorker.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <DDrawMgr/ResolveNodeMacros.h>
#include <DDrawMgr/WorkerLookup.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/SoundStream.h>
#include <Dsndmgr/VolumeScale.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SoundState.h>
#include <Gruntz/Sprite.h>
#include <Gruntz/StateId.h>
#include <Image/CImage.h>
#include <Io/FileMem.h>
#include <Pix16.h>
#include <Rez/FrameClock.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveDir.h>
#include <Rez/RezArchiveEntry.h>
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const float g_volumePercentUnitScale = 0.009999999776482582f;

CWapObj::CWapObj(CDDrawSurfaceMgr* owner, i32 id, i32 flags) {
    m_id = id;
    m_flags = flags;
    m_ownerCtx = owner;
}

i32 CDDrawPaletteRegistry::IsLoaded() {
    if (m_ownerCtx == NULL) {
        goto fail;
    }
    if (m_id != -1) {
        return 1;
    }

fail:
    return 0;
}

LoadableClassId CDDrawPaletteRegistry::GetClassId() {
    return CLASSID_PALETTE_REGISTRY;
}

CDDrawPaletteRegistry::~CDDrawPaletteRegistry() {
    Unload();
}

i32 CDDrawPaletteRegistry::IsReady() {
    return 1;
}

i32 CDDrawWorkerRegistry::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

LoadableClassId CDDrawWorkerRegistry::GetClassId() {
    return CLASSID_WORKERREGISTRY;
}

CDDrawWorkerRegistry::~CDDrawWorkerRegistry() {
    Unload();
}

i32 CDDrawWorkerRegistry::ProbeWorkerKey(CRezMgr* parser, const char* key) {
    CRezDir* result = parser->GetRootDir()->GetDir(key);

    if (result != NULL) {
        return InstallTree(result, "", "_");
    }
    return 0;
}

void CDDrawWorkerRegistry::RemoveByKey(const char* key) {
    CDDrawWorker* worker = MapFind<CDDrawWorker>(m_workersByName, key);
    if (worker != NULL) {
        m_workersByName.RemoveKey(key);
        delete worker;
    }
}

i32 CDDrawWorkerList::IsLoaded() {
    if (m_ownerCtx == NULL) {
        goto fail;
    }
    if (m_id != -1) {
        return 1;
    }

fail:
    return 0;
}

LoadableClassId CDDrawWorkerList::GetClassId() {
    return CLASSID_WORKERLIST;
}

CDDrawWorkerList::~CDDrawWorkerList() {
    Unload();
}

i32 CDDrawWorkerList::IsReady() {
    return 1;
}

CDDrawPixelWorker* CDDrawWorkerList::CreatePixelWorker(i32 x, i32 y, i32 pixelValue) {
    CDDrawPixelWorker* w = new CDDrawPixelWorker(OwnerMgr());
    if (w->PlacePixel(x, y, pixelValue) == 0) {
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    m_workers.AddTail(static_cast<CObject*>(w));
    return w;
}

i32 CDDrawPixelWorker::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

i32 CDDrawPlacedWorker::SetPosition(i32 x, i32 y) {
    SET_RESOLVE_POSITION_REFERENCED(x, y);
}

LoadableClassId CDDrawPixelWorker::GetClassId() {
    return CLASSID_PIXEL_WORKER;
}

CDDrawPixelWorker::~CDDrawPixelWorker() {
    m_pixelValue = 0;
    m_dirty.Reset();
}

i32 CDDrawPixelWorker::PlacePixel(i32 x, i32 y, i32 pixelValue) {
    m_pixelValue = static_cast<char>(pixelValue);
    SET_RESOLVE_POSITION_REFERENCED(x, y);
}

void CDDrawPixelWorker::Unload() {
    m_pixelValue = 0;
    CResolveNode::Unload();
}

CDDrawFrameWorker* CDDrawWorkerList::CreateFrameWorker(
    i32 x,
    i32 y,
    const char* workerName,
    i32 frameIndex,
    i32 addHead
) {
    CDDrawFrameWorker* w = new CDDrawFrameWorker(OwnerMgr());
    if (w->PlaceFrame(x, y, workerName, frameIndex) == 0) {
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    if (addHead & 1) {
        m_workers.AddHead(static_cast<CObject*>(w));
    } else {
        m_workers.AddTail(static_cast<CObject*>(w));
    }
    return w;
}

i32 CDDrawPlacedWorker::IsLoaded() {
    return m_contentValue != 0;
}

LoadableClassId CDDrawPlacedWorker::GetClassId() {
    return CLASSID_PLACED_WORKER;
}

CDDrawFrameWorker::~CDDrawFrameWorker() {
    m_contentValue = 0;
    m_dirty.Reset();
}

i32 CDDrawFrameWorker::PlaceFrame(i32 x, i32 y, const char* workerName, i32 frameIndex) {
    ResolveFrame(workerName, frameIndex);
    SET_RESOLVE_POSITION_REFERENCED(x, y);
}

i32 CDDrawFrameWorker::PlaceFrame(i32 x, i32 y, CDDrawWorker* source, i32 frameIndex) {
    CImage* frame = source->GetAt(frameIndex);
    m_frame = frame;
    SET_RESOLVE_POSITION_REFERENCED(x, y);
}

i32 CDDrawFrameWorker::PlaceFrame(i32 x, i32 y, CImage* frame) {
    m_frame = frame;
    SET_RESOLVE_POSITION_REFERENCED(x, y);
}

void CDDrawPlacedWorker::Unload() {
    m_contentValue = 0;
    CResolveNode::Unload();
}

CDDrawFrameWorker* CDDrawWorkerList::CreateFrameWorker(
    i32 x,
    i32 y,
    CDDrawWorker* source,
    i32 frameIndex,
    i32 addHead
) {
    CDDrawFrameWorker* w = new CDDrawFrameWorker(OwnerMgr());
    if (w->PlaceFrame(x, y, source, frameIndex) == 0) {
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    if (addHead & 1) {
        m_workers.AddHead(static_cast<CObject*>(w));
    } else {
        m_workers.AddTail(static_cast<CObject*>(w));
    }
    return w;
}

CDDrawFrameWorker* CDDrawWorkerList::CreateFrameWorker(i32 x, i32 y, CImage* frame, i32 addHead) {
    CDDrawFrameWorker* w = new CDDrawFrameWorker(OwnerMgr());
    if (w->PlaceFrame(x, y, frame) == 0) {
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    if (addHead & 1) {
        m_workers.AddHead(static_cast<CObject*>(w));
    } else {
        m_workers.AddTail(static_cast<CObject*>(w));
    }
    return w;
}

i32 CDDrawSubMgrPages::IsLoaded() {
    if (m_backPair == NULL) {
        goto fail;
    }
    if (m_overlayPair == NULL) {
        goto fail;
    }
    if (m_frontSurface != NULL) {
        return 1;
    }

fail:
    return 0;
}

CDDrawSubMgrPages::~CDDrawSubMgrPages() {
    Unload();
}

SoundCueRegistry::~SoundCueRegistry() {

    Unload();
}

i32 CDDrawChildGroup::IsLoaded() {
    if (m_ownerCtx == NULL || m_id == -1) {
        return 0;
    }
    return 1;
}

LoadableClassId CDDrawChildGroup::GetClassId() {
    return CLASSID_CHILDGROUP;
}

CDDrawChildGroup::~CDDrawChildGroup() {
    Unload();
}

i32 CDDrawChildGroup::IsReady() {
    return 1;
}

CLogicRecordRegistry::~CLogicRecordRegistry() {
    Unload();
}

i32 AnimationRegistry::IsLoaded() {
    if (m_ownerCtx == NULL) {
        goto fail;
    }
    if (m_id != -1) {
        return 1;
    }

fail:
    return 0;
}

AnimationRegistry::~AnimationRegistry() {
    Unload();
}

CString CFileMemBase::GetName() {
    return m_name;
}

i32 CFileMemBase::WantRead() {
    return m_mode;
}

i32 CFileMemBase::WantCreate() {
    return m_mode == 0;
}

i32 CFileMem::GetLength() {
    return m_length;
}

i32 CFileMem::GetOffset() {
    return m_offset;
}

void CFileMemBase::Reset() {
    m_option = 0;
    m_mode = 0;
    m_name.Empty();
}

i32 SoundCueRegistry::BindSoundStream(b32 allowUnavailable) {
    CDDrawSurfaceMgr* mgr = OwnerMgr();
    if (mgr == NULL) {
        return 0;
    }
    SoundStream* stream = mgr->GetSoundStream();
    if (allowUnavailable == false) {
        if (stream == NULL) {
            return 0;
        }
        if (stream->m_initialized == false) {
            return 0;
        }
    }
    if (stream == NULL) {
        m_silentMode = true;
    } else {
        m_silentMode = false;
    }
    m_soundStream = stream;
    g_soundVolumePercent = VOLUME_PCT_MAX;
    return 1;
}

void SoundCueRegistry::Unload() {
    ClearCues();
    m_soundStream = NULL;
}

void SoundCueRegistry::RemoveCue(SoundCue* cue) {
    if (cue == NULL) {
        return;
    }
    POSITION pos = m_cues.GetStartPosition();
    CString key;
    SoundCue* mappedCue = NULL;
    while (pos != NULL) {
        MapGetNext(m_cues, pos, key, mappedCue);
        if (cue == mappedCue) {
            m_cues.RemoveKey(key);
            delete cue;
            return;
        }
    }
}

void SoundCueRegistry::ClearCues() {
    POSITION pos = m_cues.GetStartPosition();
    CString key;
    SoundCue* cue = NULL;
    if (pos != NULL) {
        do {
            MapGetNext(m_cues, pos, key, cue);
            if (cue != NULL) {
                delete cue;
            }
        } while (pos != NULL);
    }
    m_cues.RemoveAll();
}

i32 SoundCueRegistry::RemoveWithPrefix(const char* prefix, const char* separator) {
    CString match(prefix);
    match += separator;
    i32 prefixLength = match.GetLength();
    CString key;
    SoundCue* cue = NULL;
    POSITION pos = m_cues.GetStartPosition();
    i32 removedCount = 0;
    while (pos != NULL) {
        MapGetNext(m_cues, pos, key, cue);
        if (strncmp(key, match, prefixLength) == 0) {
            m_cues.RemoveKey(key);
            if (cue != NULL) {
                delete cue;
            }
            ++removedCount;
        }
    }
    return removedCount;
}

SoundCue* SoundCueRegistry::LoadCueFromSource(const char* key, CRezItm* source) {
    if (m_silentMode != false) {
        return NULL;
    }
    SoundCue* cue = new SoundCue(CueCount(), m_ownerCtx);
    if (cue == NULL) {
        return NULL;
    }
    if (cue->LoadFromSource(source) == 0) {
        delete cue;
        return NULL;
    }
    ADD_SOUND_CUE_ENTRY(cue, key);
    return cue;
}

SoundCue* SoundCueRegistry::LoadCueFromFile(const char* key, char* path) {
    if (m_silentMode != false) {
        return NULL;
    }
    SoundCue* cue = new SoundCue(CueCount(), m_ownerCtx);
    if (cue == NULL) {
        return NULL;
    }
    if (cue->LoadFromFile(path) == 0) {
        delete cue;
        return NULL;
    }
    ADD_SOUND_CUE_ENTRY(cue, key);
    return cue;
}

SoundCue* SoundCueRegistry::LoadNamedCue(CRezItm* source) {
    if (m_silentMode != false) {
        return NULL;
    }
    if (source == NULL) {
        return NULL;
    }
    return LoadCueFromSource(source->GetName(), source);
}

void SoundCueRegistry::AddCue(SoundCue* cue, const char* key) {
    ADD_SOUND_CUE_ENTRY(cue, key);
}

i32 SoundCueRegistry::LoadFromTree(CRezDir* tree, const char* prefix, const char* separator) {
    if (m_silentMode != false) {
        return 0;
    }
    i32 count = 0;
    char* cueKey = new char[0x100];
    if (cueKey == NULL) {
        return 0;
    }
    cueKey[0] = 0;
    CRezDir* node = static_cast<CRezDir*>(tree->GetFirstSubDir());
    while (node != NULL) {
        if (prefix != NULL && *prefix != 0) {
            sprintf(cueKey, "%s%s%s", prefix, separator, node->GetDirName());
        } else {
            strcpy(cueKey, node->GetDirName());
        }
        count += LoadFromTree(node, cueKey, separator);
        node = static_cast<CRezDir*>(tree->GetNextSubDir(node));
    }

    CRezTyp* file = tree->GetFirstType();
    if (file != NULL) {
        do {
            CRezItm* source = tree->GetFirstItem(file);
            while (source != NULL) {
                if (source->GetType() == REZ_TAG_WAV) {
                    if (prefix != NULL && *prefix != 0) {
                        sprintf(cueKey, "%s%s%s", prefix, separator, source->GetName());
                    } else {
                        strcpy(cueKey, source->GetName());
                    }
                    SoundCue* cue = FindCue(cueKey);
                    if (cue == NULL) {
                        if (LoadCueFromSource(cueKey, source) != NULL) {
                            ++count;
                        }
                    }
                }
                source = tree->GetNextItem(source);
            }
            file = tree->GetNextType(file);
        } while (file != NULL);
    }
    delete[] cueKey;
    return count;
}

i32 SoundCueRegistry::SumAudioBytes(const char* prefix) {
    if (m_silentMode != false) {
        return 0;
    }
    POSITION pos = m_cues.GetStartPosition();
    i32 sum = 0;
    SoundCue* cue = NULL;
    CString key;
    while (pos != NULL) {
        cue = NULL;
        MapGetNext(m_cues, pos, key, cue);
        if (cue != NULL) {
            if (prefix == NULL || *prefix == 0) {
                sum += cue->m_sound->m_sampleCount;
            } else if (strncmp(key, prefix, strlen(prefix)) == 0) {
                sum += cue->m_sound->m_sampleCount;
            }
        }
    }
    return sum;
}

i32 SoundCueRegistry::PlaySpatializedCue(
    const char* key,
    i32 sourceX,
    i32 maxPanOffsetPx,
    i32 fullPanOffsetPx
) {
    CGameLevel* level = OwnerMgr()->m_level;
    if (level != NULL && level->m_mainPlane != NULL && m_silentMode == false) {
        SoundCue* cue = FindCue(key);
        if (cue != NULL) {
            return cue->PlaySpatialized(sourceX, -1, maxPanOffsetPx, fullPanOffsetPx);
        }
    }
    return 0;
}

SoundCue* SoundCueRegistry::GetFirstCue() {
    if (m_silentMode != false) {
        return NULL;
    }
    POSITION pos = m_cues.GetStartPosition();
    if (pos == NULL) {
        return NULL;
    }
    SoundCue* cue = NULL;
    CString key;
    MapGetNext(m_cues, pos, key, cue);
    return cue;
}

SoundCue* SoundCueRegistry::GetNextCueAfter(SoundCue* target) {
    if (target == NULL) {
        return NULL;
    }
    if (m_silentMode != false) {
        return NULL;
    }
    POSITION pos = m_cues.GetStartPosition();
    if (pos == NULL) {
        return NULL;
    }
    SoundCue* cue = NULL;
    CString key;
    while (pos != NULL) {
        MapGetNext(m_cues, pos, key, cue);
        if (cue == target) {
            if (pos == NULL) {
                return NULL;
            }
            cue = NULL;
            MapGetNext(m_cues, pos, key, cue);
            return cue;
        }
    }
    return NULL;
}

i32 SoundCueRegistry::HasWithPrefix(const char* prefix) {
    i32 prefixLength = strlen(prefix);
    CString key;
    SoundCue* cue = NULL;
    POSITION pos = m_cues.GetStartPosition();
    while (pos != NULL) {
        MapGetNext(m_cues, pos, key, cue);
        if (strncmp(key, prefix, prefixLength) == 0) {
            return 1;
        }
    }
    return 0;
}

i32 SoundCueRegistry::ConfigurePrimaryFromFirstCue(i32 startPrimary) {
    if (m_soundStream == NULL) {
        return 0;
    }
    SoundCue* cue = GetFirstCue();
    if (cue == NULL) {
        return 0;
    }

    if (cue->m_sound == NULL) {
        return 0;
    }
    return ConfigurePrimaryFromCue(cue, startPrimary) != 0;
}

i32 SoundCueRegistry::ConfigurePrimaryFromCue(SoundCue* cue, i32 startPrimary) {
    if (cue == NULL) {
        return 0;
    }
    if (m_soundStream == NULL) {
        return 0;
    }

    WAVEFORMATEX fmt;
    if (cue->m_sound->GetFormat(&fmt, sizeof(fmt), NULL) == 0) {
        return 0;
    }
    if (m_soundStream->SetPrimaryFormat(&fmt) == 0) {
        return 0;
    }
    if (startPrimary != 0) {
        if (m_soundStream->StartPrimaryBuffer() == 0) {
            return 0;
        }
    }
    return 1;
}

CString SoundCueRegistry::FindCueKey(SoundCue* target) {
    CString key;
    if (target == NULL) {
        return key;
    }
    SoundCue* cue = NULL;
    POSITION pos = m_cues.GetStartPosition();
    while (pos != NULL) {
        MapGetNext(m_cues, pos, key, cue);
        if (cue == target) {
            return key;
        }
    }
    key.Empty();
    return key;
}

SoundCue::~SoundCue() {
    Unload();
}

i32 SoundCue::LoadFromWave(RiffWaveHeader* riff) {
    SoundDevice* dev = OwnerMgr()->GetSoundStream();
    if (!dev) {
        return 0;
    }
    m_sound = dev->LoadSample(riff, 0x100ea, 0);
    return m_sound != NULL;
}

i32 SoundCue::LoadFromFile(char* path) {
    SoundDevice* dev = OwnerMgr()->GetSoundStream();
    if (!dev) {
        return 0;
    }
    m_sound = dev->LoadSampleFile(path, 0x100ea, 0);
    return m_sound != NULL;
}

i32 SoundCue::LoadFromSource(CRezItm* source) {
    u8* blob = source->Load();
    if (blob == NULL) {
        return 0;
    }
    SoundDevice* dev = OwnerMgr()->GetSoundStream();
    b32 ok;
    if (dev == NULL) {
        ok = false;
    } else {
        RecordBytes<RiffWaveHeader> riff;
        riff.m_bytes = blob;
        m_sound = dev->LoadSample(riff.m_rec, 0x100ea, 0);
        ok = m_sound != NULL;
    }
    source->UnLoad();
    return ok;
}

void SoundCue::Unload() {
    if (m_sound != NULL) {
        SoundDevice* dev = OwnerMgr()->GetSoundStream();
        if (dev != NULL) {
            dev->DestroyBuffer(m_sound);
            m_sound = NULL;
        }
    }
}

i32 SoundCue::PlaySpatialized(i32 sourceX, i32 listenerX, i32 maxPanOffsetPx, i32 fullPanOffsetPx) {
    if (g_soundEnabled == false) {
        return 0;
    }
    if (listenerX <= 0) {
        listenerX = OwnerMgr()->m_level->m_mainPlane->m_scrollPixelX;
    }
    if (maxPanOffsetPx <= 0) {
        maxPanOffsetPx = OwnerMgr()->GetDrawTarget()->GetFrontSurface()->GetWidth() << 2;
    }
    if (fullPanOffsetPx <= 0) {
        fullPanOffsetPx = OwnerMgr()->GetDrawTarget()->GetFrontSurface()->GetWidth() / 3;
    }

    i32 panOffsetPx = sourceX - listenerX;
    if (panOffsetPx >= 0) {

        if (panOffsetPx >= maxPanOffsetPx || panOffsetPx >= fullPanOffsetPx) {

            panOffsetPx = min(maxPanOffsetPx, fullPanOffsetPx);
        }
    } else {

        i32 absPanOffsetPx = abs(panOffsetPx);
        if (absPanOffsetPx >= maxPanOffsetPx || absPanOffsetPx >= fullPanOffsetPx) {
            panOffsetPx = -min(maxPanOffsetPx, fullPanOffsetPx);
        }
    }
    i32 panPercent = (panOffsetPx * VOLUME_PCT_MAX) / fullPanOffsetPx;

    i32 volumePercent = abs(VOLUME_PCT_MAX);
    if (g_soundVolumePercent != VOLUME_PCT_MAX) {
        volumePercent =
            static_cast<i32>(volumePercent * (g_soundVolumePercent * g_volumePercentUnitScale));
    }
    return m_sound->AcquireAndPlay(volumePercent, panPercent, 0, false);
}
