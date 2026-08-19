#ifndef GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H
#define GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/LeafCue.h>
#include <Gruntz/SoundState.h>
#include <Ints.h>
#include <Utils/MapTyped.h>
#include <Wap32/WapObj.h>

struct SoundStream;
class CSymTab;
struct CParseSource;

class CDDrawSubMgrLeafScan : public CWapObj {
public:
    CDDrawSubMgrLeafScan(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {
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

    void PlayCue(const char* key) {
        if (m_emitGate == 0) {
            LeafCue* found = NULL;
            MapLookup(m_cues, key, found);
            if (found != NULL) {
                found->PlayIfElapsed(g_sndCueTag, 0, 0, 0);
            }
        }
    }

    LeafCue* CreateEntry(const char* key, CParseSource* src);
    LeafCue* CreateEntry2(const char* key, char* src);
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

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H
