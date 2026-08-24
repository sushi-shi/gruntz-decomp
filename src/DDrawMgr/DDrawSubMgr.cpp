#include <rva.h>

#include <DDrawMgr/DDrawSubMgr.h>

#include <Mfc.h>

#include <Bute/SymParser.h>
#include <Bute/SymTab.h>
#include <DDrawMgr/AniAdvance.h>
#include <DDrawMgr/ColorDepth.h>
#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawPtrCollections.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawSurfacePair.h>
#include <DDrawMgr/DDrawWorker.h>
#include <DDrawMgr/DDrawWorkerCache.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <DDrawMgr/DDrawWorkerList.h>
#include <DDrawMgr/DDrawWorkerMapSmall.h>
#include <DDrawMgr/DDrawWorkerNode.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <DDrawMgr/DirectDrawMgr.h>
#include <Dsndmgr/SoundBuffer.h>
#include <Dsndmgr/SoundDevice.h>
#include <Dsndmgr/SoundStream.h>
#include <Enums.h>
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/AniElement.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/LeafCue.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/SerialArchive.h>
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
const float g_sndPanScale = 0.009999999776482582f;

// The pinned half of the CWapObj two-entity split; the tagged inline sibling
// lives in Wap32/WapObj.h.
RVA(0x00156cb0, 0x20)
CWapObj::CWapObj(CDDrawSurfaceMgr* owner, i32 field04, i32 field08) {
    m_id = field04;
    m_flags = field08;
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

RVA_COMPGEN(0x00157550, 0x1e, ??_GCDDrawSubMgrLeafScan@@UAEPAXI@Z)

RVA(0x00157570, 0x68)
CDDrawSubMgrLeafScan::~CDDrawSubMgrLeafScan() {

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

RVA_COMPGEN(0x00157700, 0x1e, ??_GCDDrawWorkerCache@@UAEPAXI@Z)

RVA(0x00157720, 0x68)
CDDrawWorkerCache::~CDDrawWorkerCache() {
    Unload();
}

RVA(0x001577a0, 0x16)
i32 CDDrawSubMgrLeaf::IsLoaded() {
    if (m_ownerCtx == NULL) {
        goto fail;
    }
    if (m_id != -1) {
        return 1;
    }

fail:
    return 0;
}

RVA_COMPGEN(0x001577c0, 0x1e, ??_GCDDrawSubMgrLeaf@@UAEPAXI@Z)

RVA(0x001577e0, 0x68)
CDDrawSubMgrLeaf::~CDDrawSubMgrLeaf() {
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
i32 CDDrawSubMgrLeafScan::BindSoundStream(i32 force) {
    CDDrawSurfaceMgr* mgr = OwnerMgr();
    if (mgr == NULL) {
        return 0;
    }
    SoundStream* stream = mgr->m_soundStream;
    if (force == 0) {
        if (stream == NULL) {
            return 0;
        }
        if (stream->m_initialized == 0) {
            return 0;
        }
    }
    if (stream == NULL) {
        m_emitGate = 1;
    } else {
        m_emitGate = 0;
    }
    m_soundStream = stream;
    g_sndCueTag = SND_CUE_NEUTRAL;
    return 1;
}

RVA(0x00157ae0, 0x11)
void CDDrawSubMgrLeafScan::Unload() {
    ClearMap();
    m_soundStream = NULL;
}

// @early-stop
// The CString local and the POSITION local hold each other's stack slots. Not
// steered by: all decl orders, renames, guard shape, scope block, for/while,
// uninit-decl + late assign, or TU-state islands (measured on the 0x152660 twin).
// The exact sibling CDDrawWorkerMapSmall::RemoveByValue (0x165c40) gets the retail
// layout from this same shape; the coin is allocator state outside the body text.
RVA(0x00157b00, 0xb2)
void CDDrawSubMgrLeafScan::RemoveByValue(LeafCue* p) {
    if (p == NULL) {
        return;
    }
    POSITION pos = m_cues.GetStartPosition();
    CString key;
    LeafCue* value = NULL;
    while (pos != static_cast<POSITION>(0)) {
        MapGetNext(m_cues, pos, key, value);
        if (p == value) {
            m_cues.RemoveKey(key);
            delete p;
            break;
        }
    }
}

RVA(0x00157bc0, 0xa2)
void CDDrawSubMgrLeafScan::ClearMap() {
    POSITION pos = m_cues.GetStartPosition();
    CString key;
    LeafCue* val = NULL;
    if (pos != NULL) {
        do {
            MapGetNext(m_cues, pos, key, val);
            if (val != NULL) {
                delete val;
            }
        } while (pos != NULL);
    }
    m_cues.RemoveAll();
}

RVA(0x00157c70, 0xf8)
i32 CDDrawSubMgrLeafScan::RemoveKeysEqual(const char* base, const char* str) {
    CString match(base);
    match += str;
    i32 len = match.GetLength();
    CString key;
    LeafCue* val = NULL;
    POSITION pos = m_cues.GetStartPosition();
    i32 n = 0;
    while (pos != NULL) {
        MapGetNext(m_cues, pos, key, val);
        if (strncmp(key, match, len) == 0) {
            m_cues.RemoveKey(key);
            if (val != NULL) {
                delete val;
            }
            ++n;
        }
    }
    return n;
}

#define ADD_SOUND_CUE_ENTRY(elem, key)                                                             \
    m_cues[key] = elem;                                                                            \
    elem->m_replayDelay = m_replayDelay

RVA(0x00157d70, 0x90)
LeafCue* CDDrawSubMgrLeafScan::CreateEntry(const char* key, CParseSource* src) {
    if (m_emitGate != 0) {
        return NULL;
    }
    LeafCue* e = new LeafCue(CueCount(), m_ownerCtx);
    if (e == NULL) {
        return NULL;
    }
    if (e->Configure(src) == 0) {
        delete e;
        return NULL;
    }
    ADD_SOUND_CUE_ENTRY(e, key);
    return e;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00157e00, 0x90)
LeafCue* CDDrawSubMgrLeafScan::CreateEntry2(const char* key, char* src) {
    if (m_emitGate != 0) {
        return NULL;
    }
    LeafCue* e = new LeafCue(CueCount(), m_ownerCtx);
    if (e == NULL) {
        return NULL;
    }
    if (e->LoadSoundB(src) == 0) {
        delete e;
        return NULL;
    }
    ADD_SOUND_CUE_ENTRY(e, key);
    return e;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00157e90, 0x23)
LeafCue* CDDrawSubMgrLeafScan::AddFromSource(CParseSource* src) {
    if (m_emitGate != 0) {
        return NULL;
    }
    if (src == NULL) {
        return NULL;
    }
    return CreateEntry(src->m_name, src);
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x00157ec0, 0x20)
void CDDrawSubMgrLeafScan::AddEntry(LeafCue* elem, const char* key) {
    ADD_SOUND_CUE_ENTRY(elem, key);
}

RVA(0x00157ee0, 0x1c6)
i32 CDDrawSubMgrLeafScan::ScanTree(CSymTab* tree, const char* prefix, const char* suffix) {
    if (m_emitGate != 0) {
        return 0;
    }
    i32 count = 0;
    char* buf = new char[0x100];
    if (buf == NULL) {
        return 0;
    }
    buf[0] = 0;
    CSymTab* node = static_cast<CSymTab*>(tree->FirstSub());
    while (node != NULL) {
        if (prefix != NULL && *prefix != 0) {
            sprintf(buf, "%s%s%s", prefix, suffix, node->m_name);
        } else {
            strcpy(buf, node->m_name);
        }
        count += ScanTree(node, buf, suffix);
        node = static_cast<CSymTab*>(tree->NextSub(node));
    }

    CSymRec* file = tree->FirstSym();
    if (file != NULL) {
        do {
            CParseSource* fn = tree->NextSym2(file);
            while (fn != NULL) {
                if (fn->GetEntryTag() == PARSETAG_VAW) {
                    if (prefix != NULL && *prefix != 0) {
                        sprintf(buf, "%s%s%s", prefix, suffix, fn->m_name);
                    } else {
                        strcpy(buf, fn->m_name);
                    }
                    LeafCue* val = NULL;
                    MapLookup(m_cues, buf, val);
                    if (val == NULL) {
                        if (CreateEntry(buf, fn) != NULL) {
                            ++count;
                        }
                    }
                }
                fn = tree->NextSym3(fn);
            }
            file = tree->NextSym(file);
        } while (file != NULL);
    }
    delete[] buf;
    return count;
}

RVA(0x001580b0, 0xf6)
i32 CDDrawSubMgrLeafScan::SumField(const char* str) {
    if (m_emitGate != 0) {
        return 0;
    }
    POSITION pos = m_cues.GetStartPosition();
    i32 sum = 0;
    LeafCue* val = NULL;
    CString key;
    while (pos != NULL) {
        val = NULL;
        MapGetNext(m_cues, pos, key, val);
        if (val != NULL) {
            if (str == NULL || *str == 0) {
                sum += val->m_sound->m_sampleCount;
            } else if (strncmp(key, str, strlen(str)) == 0) {
                sum += val->m_sound->m_sampleCount;
            }
        }
    }
    return sum;
}
// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001581b0, 0x5b)
i32 CDDrawSubMgrLeafScan::Fire(const char* key, i32 pos, i32 range1, i32 range2) {
    CGameLevel* lvl = OwnerMgr()->m_level;
    if (lvl != NULL && lvl->m_mainPlane != NULL && m_emitGate == 0) {
        LeafCue* val = NULL;
        MapLookup(m_cues, key, val);
        if (val != NULL) {
            return val->TriggerBlit(pos, -1, range1, range2);
        }
    }
    return 0;
}

RVA(0x00158210, 0xaa)
LeafCue* CDDrawSubMgrLeafScan::GetFirstValue() {
    if (m_emitGate != 0) {
        return NULL;
    }
    POSITION pos = m_cues.GetStartPosition();
    if (pos == NULL) {
        return NULL;
    }
    LeafCue* val = NULL;
    CString key;
    MapGetNext(m_cues, pos, key, val);
    return val;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001582c0, 0xf6)
LeafCue* CDDrawSubMgrLeafScan::NextValueAfter(LeafCue* target) {
    if (target == NULL) {
        return NULL;
    }
    if (m_emitGate != 0) {
        return NULL;
    }
    POSITION pos = m_cues.GetStartPosition();
    if (pos == NULL) {
        return NULL;
    }
    LeafCue* val = NULL;
    CString key;
    while (pos != NULL) {
        MapGetNext(m_cues, pos, key, val);
        if (val == target) {
            if (pos == NULL) {
                return NULL;
            }
            val = NULL;
            MapGetNext(m_cues, pos, key, val);
            return val;
        }
    }
    return NULL;
}

RVA(0x001583c0, 0xdc)
i32 CDDrawSubMgrLeafScan::HasKeyEqual(const char* str) {
    i32 len = strlen(str);
    CString key;
    LeafCue* val = NULL;
    POSITION pos = m_cues.GetStartPosition();
    while (pos != NULL) {
        MapGetNext(m_cues, pos, key, val);
        if (strncmp(key, str, len) == 0) {
            return 1;
        }
    }
    return 0;
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001584a0, 0x43)
i32 CDDrawSubMgrLeafScan::ProbeFirst(i32 arg) {
    if (m_soundStream == NULL) {
        return 0;
    }
    LeafCue* val = GetFirstValue();
    if (val == NULL) {
        return 0;
    }

    if (val->m_sound == NULL) {
        return 0;
    }
    return MatchSub(val, arg) != 0;
}

RVA(0x001584f0, 0x80)
i32 CDDrawSubMgrLeafScan::MatchSub(LeafCue* cue, i32 startPrimary) {
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
CString CDDrawSubMgrLeafScan::FindKeyOfValue(LeafCue* target) {
    CString key;
    if (target == NULL) {
        return key;
    }
    LeafCue* val = NULL;
    POSITION pos = m_cues.GetStartPosition();
    while (pos != NULL) {
        MapGetNext(m_cues, pos, key, val);
        if (val == target) {
            return key;
        }
    }
    key.Empty();
    return key;
}

RVA_COMPGEN(0x00158660, 0x1e, ??_GLeafCue@@UAEPAXI@Z)
RVA(0x00158680, 0x5b)
LeafCue::~LeafCue() {
    Unload();
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x001586e0, 0x34)
i32 LeafCue::LoadSoundA(RiffWaveHeader* riff) {
    SoundDevice* dev = OwnerMgr()->m_soundStream;
    if (!dev) {
        return 0;
    }
    m_sound = dev->LoadSample(riff, 0x100ea, 0);
    return m_sound != NULL;
}

RVA(0x00158720, 0x34)
i32 LeafCue::LoadSoundB(char* src) {
    SoundDevice* dev = OwnerMgr()->m_soundStream;
    if (!dev) {
        return 0;
    }
    m_sound = dev->LoadSampleFile(src, 0x100ea, 0);
    return m_sound != NULL;
}

RVA(0x00158760, 0x59)
i32 LeafCue::Configure(CParseSource* src) {
    char* blob = src->BeginParse();
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
    src->EndParse();
    return ok;
}

RVA(0x001587c0, 0x23)
void LeafCue::Unload() {
    if (m_sound != NULL) {
        SoundDevice* dev = OwnerMgr()->m_soundStream;
        if (dev != NULL) {
            dev->DestroyBuffer(m_sound);
            m_sound = NULL;
        }
    }
}

RVA(0x001587f0, 0xf1)
i32 LeafCue::TriggerBlit(i32 pos, i32 center, i32 range1, i32 range2) {
    if (g_sndEnabled == 0) {
        return 0;
    }
    if (center <= 0) {
        center = OwnerMgr()->m_level->m_mainPlane->m_snappedX;
    }
    if (range1 <= 0) {
        range1 = OwnerMgr()->m_drawTarget->m_frontPair->m_width << 2;
    }
    if (range2 <= 0) {
        range2 = OwnerMgr()->m_drawTarget->m_frontPair->m_width / 3;
    }

    i32 pan = pos - center;
    if (pan >= 0) {

        if (pan >= range1 || pan >= range2) {

            pan = range1 < range2 ? range1 : range2;
        }
    } else {

        i32 ad = abs(pan);
        if (ad >= range1 || ad >= range2) {
            pan = -(range1 < range2 ? range1 : range2);
        }
    }
    i32 vol = (pan * 100) / range2;

    i32 vscale = abs(SND_CUE_NEUTRAL);
    if (g_sndCueTag != SND_CUE_NEUTRAL) {
        vscale = static_cast<i32>(vscale * (g_sndCueTag * g_sndPanScale));
    }
    return m_sound->AcquireAndPlay(vscale, vol, 0, 0);
}
