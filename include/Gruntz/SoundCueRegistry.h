#ifndef GRUNTZ_SOUNDCUEREGISTRY_H
#define GRUNTZ_SOUNDCUEREGISTRY_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Ints.h>
#include <Utils/MapTyped.h>
#include <Wap32/WapObj.h>

struct SoundStream;
class CRezArchiveDir;
struct CRezArchiveEntry;

class SoundCueRegistry : public CWapObj {
public:
    SoundCueRegistry(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {
        m_soundStream = NULL;
        m_defaultReplayDelayMs = 0;
    }

    RVA(0x00157530, 0x17)
    virtual i32 IsLoaded() OVERRIDE {

        if (m_soundStream == NULL && m_silentMode == 0) {
            return 0;
        }
        return 1;
    }

    virtual void Unload() OVERRIDE;

    i32 PlayCueIfElapsed(const char* key);

    SoundCue* FindCue(const char* key) {
        SoundCue* found = NULL;
        MapLookup(m_cues, key, found);
        return found;
    }

    void PlayCue(const char* key) {
        if (m_silentMode == 0) {
            SoundCue* found = NULL;
            MapLookup(m_cues, key, found);
            if (found != NULL) {
                found->PlayIfElapsed(g_soundVolumePercent, 0, 0, 0);
            }
        }
    }

    SoundCue* LoadCueFromSource(const char* key, CRezArchiveEntry* source);
    SoundCue* LoadCueFromFile(const char* key, char* path);
    SoundCue* LoadNamedCue(CRezArchiveEntry* source);
    void AddCue(SoundCue* cue, const char* key);

    i32 LoadFromTree(CRezArchiveDir* tree, const char* prefix, const char* separator);

    CObject* Lookup(const char* key);
    i32 RemoveWithPrefix(const char* prefix, const char* separator);
    i32 SumAudioBytes(const char* prefix);
    SoundCue* GetFirstCue();
    SoundCue* GetNextCueAfter(SoundCue* target);
    i32 ConfigurePrimaryFromFirstCue(i32 startPrimary);
    i32 HasWithPrefix(const char* prefix);
    CString FindCueKey(SoundCue* target);
    i32 ConfigurePrimaryFromCue(SoundCue* cue, i32 startPrimary);

    // The registered-cue count; retail expands it at every use.
    i32 CueCount() const {
        return m_cues.GetCount();
    }

    void ClearCues();

    void RemoveCue(struct SoundCue* cue);

    virtual ~SoundCueRegistry() OVERRIDE;

    i32 PlaySpatializedCue(const char* key, i32 sourceX, i32 maxPanOffsetPx, i32 fullPanOffsetPx);

    i32 BindSoundStream(i32 allowUnavailable);

    CMapStringToPtr m_cues;
    SoundStream* m_soundStream;

    i32 m_silentMode;
    i32 m_defaultReplayDelayMs;
};

#endif // GRUNTZ_SOUNDCUEREGISTRY_H
