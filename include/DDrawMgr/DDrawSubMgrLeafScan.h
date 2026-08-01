#ifndef GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H
#define GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>
#include <Gruntz/Loadable.h>

struct SoundStream;
struct LeafCue;
class CSymTab;
struct CParseSource;

class CDDrawSubMgrLeafScan : public CLoadable {
public:
    CDDrawSubMgrLeafScan(CDDrawSurfaceMgr* owner) : CLoadable(owner) {
        m_soundStream = 0;
        m_34 = 0;
    }

    RVA(0x00157530, 0x17)
    virtual i32 IsLoaded() OVERRIDE {

        if (m_soundStream == 0 && m_emitGate == 0) {
            return 0;
        }
        return 1;
    }

    virtual void Unload() OVERRIDE;

    i32 RefreshAsset(const char* key);

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

    void ClearMap();

    void RemoveByValue(struct LeafCue* p);

    virtual ~CDDrawSubMgrLeafScan() OVERRIDE;

    i32 Fire(const char* key, i32 pos, i32 range1, i32 range2);

    i32 BindSoundStream(i32 force);

    CMapStringToPtr m_10;
    SoundStream* m_soundStream;

    i32 m_emitGate;
    i32 m_34;
};
SIZE_UNKNOWN();
SIZE_UNKNOWN();

#endif // GRUNTZ_DDRAWMGR_CDDRAWSUBMGRLEAFSCAN_H
