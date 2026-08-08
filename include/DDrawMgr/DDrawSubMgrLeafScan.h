#ifndef GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H
#define GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/LeafCue.h>
#include <Gruntz/Loadable.h>
#include <Gruntz/SoundState.h>
#include <Ints.h>

struct SoundStream;
class CSymTab;
struct CParseSource;

class CDDrawSubMgrLeafScan : public CLoadable {
public:
    CDDrawSubMgrLeafScan(CDDrawSurfaceMgr* owner) : CLoadable(owner, 0, 0, CLoadable::NO_SEED) {
        m_soundStream = NULL;
        m_replayDelay = 0;
    }

    RVA(0x00157530, 0x17)
    virtual i32 IsLoaded() OVERRIDE {

        if (m_soundStream == NULL && m_emitGate == 0) {
            return 0;
        }
        return 1;
    }

    virtual void Unload() OVERRIDE;

    i32 RefreshAsset(const char* key);

    // The INLINE twin of RefreshAsset. Both exist in retail: the out-of-line
    // body at 0x114120 expands LeafCue::PlayIfElapsed into itself, while call
    // sites like CTriggerMgr::LoadTileArrivalFx expand THIS shape - the map
    // lookup inline and PlayIfElapsed as a call.
    void PlayCue(const char* key) {
        if (m_emitGate == 0) {
            void* found = NULL;
            m_cues.Lookup(key, found);
            if (found != NULL) {
                static_cast<LeafCue*>(found)->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
            }
        }
    }

    LeafCue* CreateEntry(const char* key, void* src);
    LeafCue* CreateEntry2(const char* key, void* src);
    LeafCue* AddFromSource(CParseSource* src);
    void AddEntry(LeafCue* elem, const char* key);

    i32 ScanTree(CSymTab* tree, const char* prefix, const char* suffix);

    CObject* Lookup(const char* key);
    i32 RemoveKeysEqual(const char* base, const char* str);
    i32 SumField(const char* str);
    LeafCue* GetFirstValue();
    LeafCue* NextValueAfter(LeafCue* target);
    i32 ProbeFirst(i32 arg);
    i32 HasKeyEqual(const char* str);
    CString FindKeyOfValue(LeafCue* target);
    i32 MatchSub(LeafCue* cue, i32 startPrimary);

    // The registered-cue count; retail expands it at every use.
    i32 CueCount() const {
        return m_cues.GetCount();
    }

    void ClearMap();

    void RemoveByValue(struct LeafCue* p);

    virtual ~CDDrawSubMgrLeafScan() OVERRIDE;

    i32 Fire(const char* key, i32 pos, i32 range1, i32 range2);

    i32 BindSoundStream(i32 force);

    CMapStringToPtr m_cues;
    SoundStream* m_soundStream;

    i32 m_emitGate;
    i32 m_replayDelay;
};
SIZE(0x38);
SIZE_UNKNOWN();

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H
