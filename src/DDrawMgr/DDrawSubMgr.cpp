#include <rva.h>

#include <DDrawMgr/DDrawSubMgr.h>

#include <Mfc.h>

#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawDeviceManager.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DDrawWorkerNode.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <DDrawMgr/LogicRecordRegistry.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/SoundStream.h>
#include <Dsndmgr/VolumeScale.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/ParseSource.h>
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
#include <Utils/MapTyped.h>
#include <Wap32/CoordUnset.h>
#include <Wap32/Object.h>
#include <Wap32/WapObj.h>

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DATA(0x001eff2c)
const float c_volumePercentUnitScale = 0.009999999776482582f;

// The pinned half of the CWapObj two-entity split; the tagged inline sibling
// lives in Wap32/WapObj.h.
RVA(0x00156cb0, 0x20)
CWapObj::CWapObj(CDDrawSurfaceMgr* owner, i32 id, i32 flags) {
    m_id = id;
    m_flags = flags;
    m_ownerCtx = owner;
}

RVA(0x00156cd0, 0x16)
i32 CDDrawWorkerMapSmall::IsLoaded() {
    if (m_ownerCtx == NULL) {
        goto fail;
    }
    if (m_id != -1) {
        return 1;
    }

fail:
    return 0;
}

RVA(0x00156cf0, 0x6)
LoadableClassId CDDrawWorkerMapSmall::GetClassId() {
    return CLASSID_WORKERMAPSMALL;
}

RVA_COMPGEN(0x00156d00, 0x1e, ??_GCDDrawWorkerMapSmall@@UAEPAXI@Z)
RVA(0x00156d20, 0x82)
CDDrawWorkerMapSmall::~CDDrawWorkerMapSmall() {
    Unload();
}

RVA(0x00156db0, 0x6)
i32 CDDrawWorkerMapSmall::IsReady() {
    return 1;
}

RVA(0x00156dc0, 0x16)
i32 CDDrawWorkerRegistry::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

RVA(0x00156de0, 0x6)
LoadableClassId CDDrawWorkerRegistry::GetClassId() {
    return CLASSID_WORKERREGISTRY;
}

RVA_COMPGEN(0x00156df0, 0x1e, ??_GCDDrawWorkerRegistry@@UAEPAXI@Z)
RVA(0x00156e10, 0x68)
CDDrawWorkerRegistry::~CDDrawWorkerRegistry() {
    Unload();
}

RVA(0x00156e80, 0x38)
i32 CDDrawWorkerRegistry::ProbeWorkerKey(CSymParser* parser, const char* key) {
    CSymTab* result = parser->GetRoot()->FindSub(key);

    if (result != NULL) {
        return InstallTree(result, "", "_");
    }
    return 0;
}

// @early-stop
// retail loads the Lookup out-slot ONCE straight into edi (`mov edi,[esp+0xc]`)
// and tests the copy; our cl live-range-splits it (`mov eax,[mem]; test eax;
// mov edi,eax`). Flat across: test-w/test-val, assignment-in-condition,
// register, w-in-body, w-decl-first, typed out-param via a union helper (that
// one also re-loads + re-emits delete's null check), and the generated AST tree.
RVA(0x00156ec0, 0x40)
void CDDrawWorkerRegistry::RemoveByKey(const char* key) {
    CObject* val = NULL;
    m_workersByName.Lookup(key, val);
    CDDrawWorker* w = static_cast<CDDrawWorker*>(val);
    if (val != NULL) {
        m_workersByName.RemoveKey(key);
        delete w;
    }
}

RVA(0x00156f00, 0x16)
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

RVA(0x00156f20, 0x6)
LoadableClassId CDDrawWorkerList::GetClassId() {
    return CLASSID_WORKERLIST;
}

RVA_COMPGEN(0x00156f30, 0x1e, ??_GCDDrawWorkerList@@UAEPAXI@Z)
RVA(0x00156f50, 0x68)
CDDrawWorkerList::~CDDrawWorkerList() {
    Unload();
}

RVA(0x00156fc0, 0x6)
i32 CDDrawWorkerList::IsReady() {
    return 1;
}

RVA(0x00156fd0, 0x8b)
CDDrawWorkerA* CDDrawWorkerList::CreatePixelWorker(i32 x, i32 y, i32 pixelValue) {
    CDDrawWorkerA* w = new CDDrawWorkerA(OwnerMgr());
    if (w->PlacePixel(x, y, pixelValue) == 0) {
        if (w != NULL) {
            delete w;
        }
        return NULL;
    }
    m_workers.AddTail(static_cast<CObject*>(w));
    return w;
}

RVA(0x00157060, 0x16)
i32 CDDrawWorkerA::IsLoaded() {
    if (m_ownerCtx != NULL && m_id != -1) {
        return 1;
    }
    return 0;
}

#define SET_RESOLVE_POSITION_REFERENCED(x, y)                                                      \
    m_refCount = 2;                                                                                \
    return CResolveNode::SetPosition(x, y)

RVA(0x00157080, 0x19)
i32 CDDrawWorkerBase::SetPosition(i32 x, i32 y) {
    SET_RESOLVE_POSITION_REFERENCED(x, y);
}

RVA(0x001570a0, 0x6)
LoadableClassId CDDrawWorkerA::GetClassId() {
    return CLASSID_WORKERPIXEL;
}

RVA_COMPGEN(0x001570b0, 0x1e, ??_GCDDrawWorkerA@@UAEPAXI@Z)
RVA(0x001570d0, 0x39)
CDDrawWorkerA::~CDDrawWorkerA() {
    m_pixelValue = 0;
    m_dirty.Reset();
}

RVA(0x00157110, 0x20)
i32 CDDrawWorkerA::PlacePixel(i32 x, i32 y, i32 pixelValue) {
    m_pixelValue = static_cast<char>(pixelValue);
    SET_RESOLVE_POSITION_REFERENCED(x, y);
}

RVA(0x00157130, 0x17)
void CDDrawWorkerA::Unload() {

    i32 v = COORD_UNSET;
    m_pixelValue = 0;
    m_screenX = v;
    m_dirty.m_rect.left = v;
    m_dirty.m_armed = -1;
}

RVA(0x00157150, 0xa5)
CDDrawWorkerB* CDDrawWorkerList::CreateFrameWorker(
    i32 x,
    i32 y,
    const char* workerName,
    i32 frameIndex,
    i32 addHead
) {
    CDDrawWorkerB* w = new CDDrawWorkerB(OwnerMgr());
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

RVA(0x00157200, 0xb)
i32 CDDrawWorkerBase::IsLoaded() {
    return m_frameValue != 0;
}

RVA(0x00157210, 0x6)
LoadableClassId CDDrawWorkerBase::GetClassId() {
    return CLASSID_WORKERNODE;
}

RVA_COMPGEN(0x00157220, 0x1e, ??_GCDDrawWorkerB@@UAEPAXI@Z)
RVA(0x00157240, 0x3c)
CDDrawWorkerB::~CDDrawWorkerB() {
    m_frameValue = 0;
    m_dirty.Reset();
}

RVA(0x00157280, 0x30)
i32 CDDrawWorkerB::PlaceFrame(i32 x, i32 y, const char* workerName, i32 frameIndex) {
    ResolveFrame(workerName, frameIndex);
    SET_RESOLVE_POSITION_REFERENCED(x, y);
}

RVA(0x001572b0, 0x38)
i32 CDDrawWorkerB::PlaceFrame(i32 x, i32 y, CDDrawWorker* source, i32 frameIndex) {
    CImage* frame = source->GetAt(frameIndex);
    m_frame = frame;
    SET_RESOLVE_POSITION_REFERENCED(x, y);
}

RVA(0x001572f0, 0x20)
i32 CDDrawWorkerB::PlaceFrame(i32 x, i32 y, CImage* frame) {
    m_frame = frame;
    SET_RESOLVE_POSITION_REFERENCED(x, y);
}

RVA(0x00157310, 0x1a)
void CDDrawWorkerBase::Unload() {

    i32 v = COORD_UNSET;
    m_frameValue = 0;
    m_screenX = v;
    m_dirty.m_rect.left = v;
    m_dirty.m_armed = -1;
}

RVA(0x00157330, 0xa5)
CDDrawWorkerB* CDDrawWorkerList::CreateFrameWorker(
    i32 x,
    i32 y,
    CDDrawWorker* source,
    i32 frameIndex,
    i32 addHead
) {
    CDDrawWorkerB* w = new CDDrawWorkerB(OwnerMgr());
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

RVA(0x001573e0, 0xa0)
CDDrawWorkerB* CDDrawWorkerList::CreateFrameWorker(i32 x, i32 y, CImage* frame, i32 addHead) {
    CDDrawWorkerB* w = new CDDrawWorkerB(OwnerMgr());
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

RVA(0x00157480, 0x1e)
i32 CDDrawSubMgrPages::IsLoaded() {
    if (m_backPair == NULL) {
        goto fail;
    }
    if (m_overlayPair == NULL) {
        goto fail;
    }
    if (m_frontPair != NULL) {
        return 1;
    }

fail:
    return 0;
}

RVA_COMPGEN(0x001574b0, 0x1e, ??_GCDDrawSubMgrPages@@UAEPAXI@Z)

RVA(0x001574d0, 0x5b)
CDDrawSubMgrPages::~CDDrawSubMgrPages() {
    Unload();
}

RVA_COMPGEN(0x00157550, 0x1e, ??_GSoundCueRegistry@@UAEPAXI@Z)

RVA(0x00157570, 0x68)
SoundCueRegistry::~SoundCueRegistry() {

    Unload();
}

RVA(0x001575e0, 0x16)
i32 CDDrawChildGroup::IsLoaded() {
    if (m_ownerCtx == NULL || m_id == -1) {
        return 0;
    }
    return 1;
}

RVA(0x00157600, 0x6)
LoadableClassId CDDrawChildGroup::GetClassId() {
    return CLASSID_CHILDGROUP;
}

RVA_COMPGEN(0x00157610, 0x1e, ??_GCDDrawChildGroup@@UAEPAXI@Z)
RVA(0x00157630, 0x82)
CDDrawChildGroup::~CDDrawChildGroup() {
    Unload();
}

RVA(0x001576c0, 0x6)
i32 CDDrawChildGroup::IsReady() {
    return 1;
}

RVA_COMPGEN(0x00157700, 0x1e, ??_GCLogicRecordRegistry@@UAEPAXI@Z)

RVA(0x00157720, 0x68)
CLogicRecordRegistry::~CLogicRecordRegistry() {
    Unload();
}

RVA(0x001577a0, 0x16)
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

RVA_COMPGEN(0x001577c0, 0x1e, ??_GAnimationRegistry@@UAEPAXI@Z)

RVA(0x001577e0, 0x68)
AnimationRegistry::~AnimationRegistry() {
    Unload();
}

RVA(0x00157920, 0x20)
CString CFileMemBase::GetName() {
    return m_name;
}

RVA(0x00157940, 0x4)
i32 CFileMemBase::WantRead() {
    return m_mode;
}

RVA(0x00157950, 0xb)
i32 CFileMemBase::WantCreate() {
    return m_mode == 0;
}

RVA(0x00157a00, 0x4)
i32 CFileMem::GetLength() {
    return m_length;
}

RVA(0x00157a10, 0x4)
i32 CFileMem::GetOffset() {
    return m_offset;
}

RVA(0x00157a40, 0x10)
void CFileMemBase::Reset() {
    m_option = 0;
    m_mode = 0;
    m_name.Empty();
}

RVA(0x00157a80, 0x51)
i32 SoundCueRegistry::BindSoundStream(i32 allowUnavailable) {
    CDDrawSurfaceMgr* mgr = OwnerMgr();
    if (mgr == NULL) {
        return 0;
    }
    SoundStream* stream = mgr->m_soundStream;
    if (allowUnavailable == 0) {
        if (stream == NULL) {
            return 0;
        }
        if (stream->m_initialized == 0) {
            return 0;
        }
    }
    if (stream == NULL) {
        m_silentMode = 1;
    } else {
        m_silentMode = 0;
    }
    m_soundStream = stream;
    g_soundVolumePercent = VOLUME_PCT_MAX;
    return 1;
}

RVA(0x00157ae0, 0x11)
void SoundCueRegistry::Unload() {
    ClearCues();
    m_soundStream = NULL;
}

// @early-stop
// The CString local and the POSITION local hold each other's stack slots. Not
// steered by: all decl orders, renames, guard shape, scope block, for/while,
// uninit-decl + late assign, or TU-state islands (measured on the 0x152660 twin).
// The exact sibling CDDrawWorkerMapSmall::RemoveByValue (0x165c40) gets the retail
// layout from this same shape; the coin is allocator state outside the body text.
RVA(0x00157b00, 0xb2)
void SoundCueRegistry::RemoveCue(SoundCue* cue) {
    if (cue == NULL) {
        return;
    }
    POSITION pos = m_cues.GetStartPosition();
    CString key;
    SoundCue* mappedCue = NULL;
    while (pos != static_cast<POSITION>(0)) {
        MapGetNext(m_cues, pos, key, mappedCue);
        if (cue == mappedCue) {
            m_cues.RemoveKey(key);
            delete cue;
            break;
        }
    }
}

RVA(0x00157bc0, 0xa2)
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

RVA(0x00157c70, 0xf8)
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

#define ADD_SOUND_CUE_ENTRY(cue, key)                                                              \
    m_cues[key] = cue;                                                                             \
    cue->m_replayDelayMs = m_defaultReplayDelayMs

RVA(0x00157d70, 0x90)
SoundCue* SoundCueRegistry::LoadCueFromSource(const char* key, CParseSource* source) {
    if (m_silentMode != 0) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00157e00, 0x90)
SoundCue* SoundCueRegistry::LoadCueFromFile(const char* key, char* path) {
    if (m_silentMode != 0) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00157e90, 0x23)
SoundCue* SoundCueRegistry::LoadNamedCue(CParseSource* source) {
    if (m_silentMode != 0) {
        return NULL;
    }
    if (source == NULL) {
        return NULL;
    }
    return LoadCueFromSource(source->m_name, source);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00157ec0, 0x20)
void SoundCueRegistry::AddCue(SoundCue* cue, const char* key) {
    ADD_SOUND_CUE_ENTRY(cue, key);
}

RVA(0x00157ee0, 0x1c6)
i32 SoundCueRegistry::LoadFromTree(CSymTab* tree, const char* prefix, const char* separator) {
    if (m_silentMode != 0) {
        return 0;
    }
    i32 count = 0;
    char* cueKey = new char[0x100];
    if (cueKey == NULL) {
        return 0;
    }
    cueKey[0] = 0;
    CSymTab* node = static_cast<CSymTab*>(tree->FirstSub());
    while (node != NULL) {
        if (prefix != NULL && *prefix != 0) {
            sprintf(cueKey, "%s%s%s", prefix, separator, node->m_name);
        } else {
            strcpy(cueKey, node->m_name);
        }
        count += LoadFromTree(node, cueKey, separator);
        node = static_cast<CSymTab*>(tree->NextSub(node));
    }

    CSymRec* file = tree->FirstSym();
    if (file != NULL) {
        do {
            CParseSource* source = tree->NextSym2(file);
            while (source != NULL) {
                if (source->GetEntryTag() == PARSETAG_VAW) {
                    if (prefix != NULL && *prefix != 0) {
                        sprintf(cueKey, "%s%s%s", prefix, separator, source->m_name);
                    } else {
                        strcpy(cueKey, source->m_name);
                    }
                    SoundCue* cue = NULL;
                    MapLookup(m_cues, cueKey, cue);
                    if (cue == NULL) {
                        if (LoadCueFromSource(cueKey, source) != NULL) {
                            ++count;
                        }
                    }
                }
                source = tree->NextSym3(source);
            }
            file = tree->NextSym(file);
        } while (file != NULL);
    }
    delete[] cueKey;
    return count;
}

RVA(0x001580b0, 0xf6)
i32 SoundCueRegistry::SumAudioBytes(const char* prefix) {
    if (m_silentMode != 0) {
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
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001581b0, 0x5b)
i32 SoundCueRegistry::PlaySpatializedCue(
    const char* key,
    i32 sourceX,
    i32 maxPanOffsetPx,
    i32 fullPanOffsetPx
) {
    CGameLevel* level = OwnerMgr()->m_level;
    if (level != NULL && level->m_mainPlane != NULL && m_silentMode == 0) {
        SoundCue* cue = NULL;
        MapLookup(m_cues, key, cue);
        if (cue != NULL) {
            return cue->PlaySpatialized(sourceX, -1, maxPanOffsetPx, fullPanOffsetPx);
        }
    }
    return 0;
}

RVA(0x00158210, 0xaa)
SoundCue* SoundCueRegistry::GetFirstCue() {
    if (m_silentMode != 0) {
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001582c0, 0xf6)
SoundCue* SoundCueRegistry::GetNextCueAfter(SoundCue* target) {
    if (target == NULL) {
        return NULL;
    }
    if (m_silentMode != 0) {
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

RVA(0x001583c0, 0xdc)
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

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001584a0, 0x43)
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

RVA(0x001584f0, 0x80)
i32 SoundCueRegistry::ConfigurePrimaryFromCue(SoundCue* cue, i32 startPrimary) {
    if (cue == NULL) {
        return 0;
    }
    if (m_soundStream == NULL) {
        return 0;
    }

    WaveFormatX fmt;
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

RVA(0x00158570, 0xd4)
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

RVA_COMPGEN(0x00158660, 0x1e, ??_GSoundCue@@UAEPAXI@Z)
RVA(0x00158680, 0x5b)
SoundCue::~SoundCue() {
    Unload();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001586e0, 0x34)
i32 SoundCue::LoadFromWave(RiffWaveHeader* riff) {
    SoundDevice* dev = OwnerMgr()->m_soundStream;
    if (!dev) {
        return 0;
    }
    m_sound = dev->LoadSample(riff, 0x100ea, 0);
    return m_sound != NULL;
}

RVA(0x00158720, 0x34)
i32 SoundCue::LoadFromFile(char* path) {
    SoundDevice* dev = OwnerMgr()->m_soundStream;
    if (!dev) {
        return 0;
    }
    m_sound = dev->LoadSampleFile(path, 0x100ea, 0);
    return m_sound != NULL;
}

RVA(0x00158760, 0x59)
i32 SoundCue::LoadFromSource(CParseSource* source) {
    char* blob = source->BeginParse();
    if (blob == NULL) {
        return 0;
    }
    SoundDevice* dev = OwnerMgr()->m_soundStream;
    i32 ok;
    if (dev == NULL) {
        ok = 0;
    } else {
        RecordBytes<RiffWaveHeader> riff;
        riff.m_chars = blob;
        m_sound = dev->LoadSample(riff.m_rec, 0x100ea, 0);
        ok = m_sound != NULL;
    }
    source->EndParse();
    return ok;
}

RVA(0x001587c0, 0x23)
void SoundCue::Unload() {
    if (m_sound != NULL) {
        SoundDevice* dev = OwnerMgr()->m_soundStream;
        if (dev != NULL) {
            dev->DestroyBuffer(m_sound);
            m_sound = NULL;
        }
    }
}

RVA(0x001587f0, 0xf1)
i32 SoundCue::PlaySpatialized(i32 sourceX, i32 listenerX, i32 maxPanOffsetPx, i32 fullPanOffsetPx) {
    if (g_soundEnabled == 0) {
        return 0;
    }
    if (listenerX <= 0) {
        listenerX = OwnerMgr()->m_level->m_mainPlane->m_snappedX;
    }
    if (maxPanOffsetPx <= 0) {
        maxPanOffsetPx = OwnerMgr()->m_drawTarget->m_frontPair->m_width << 2;
    }
    if (fullPanOffsetPx <= 0) {
        fullPanOffsetPx = OwnerMgr()->m_drawTarget->m_frontPair->m_width / 3;
    }

    i32 panOffsetPx = sourceX - listenerX;
    if (panOffsetPx >= 0) {

        if (panOffsetPx >= maxPanOffsetPx || panOffsetPx >= fullPanOffsetPx) {

            panOffsetPx = maxPanOffsetPx < fullPanOffsetPx ? maxPanOffsetPx : fullPanOffsetPx;
        }
    } else {

        i32 absPanOffsetPx = abs(panOffsetPx);
        if (absPanOffsetPx >= maxPanOffsetPx || absPanOffsetPx >= fullPanOffsetPx) {
            panOffsetPx = -(maxPanOffsetPx < fullPanOffsetPx ? maxPanOffsetPx : fullPanOffsetPx);
        }
    }
    i32 panPercent = (panOffsetPx * VOLUME_PCT_MAX) / fullPanOffsetPx;

    i32 volumePercent = abs(VOLUME_PCT_MAX);
    if (g_soundVolumePercent != VOLUME_PCT_MAX) {
        volumePercent =
            static_cast<i32>(volumePercent * (g_soundVolumePercent * c_volumePercentUnitScale));
    }
    return m_sound->AcquireAndPlay(volumePercent, panPercent, 0, 0);
}
