#ifndef GRUNTZ_SOUNDCUEREGISTRY_H
#define GRUNTZ_SOUNDCUEREGISTRY_H

#include <map>
#include <string>


#include <Ints.h>

#include <Gruntz/SoundCue.h>
#include <Gruntz/SoundState.h>
#include <Ints.h>
#include <Utils/MapTyped.h>
#include <Wap32/WapObj.h>

class SoundStream;
class CRezDir;
struct CRezItm;

class SoundCueRegistry : public CWapObj {
public:
    inline void TickVolumeRamps();
    SoundCueRegistry(CDDrawSurfaceMgr* owner) : CWapObj(owner, 0, 0, CWapObj::NO_SEED) {
        m_soundStream = NULL;
        m_defaultReplayDelayMs = 0;
    }

    virtual i32 IsLoaded()   {

        if (m_soundStream == NULL && m_silentMode == false) {
            return 0;
        }
        return 1;
    }

    virtual void Unload()  ;

    i32 PlayCueIfElapsed(const std::string& key);

    SoundCue* FindCue(const std::string& key) {
        SoundCue* found = NULL;
        MapLookup(m_cues, key, found);
        return found;
    }

    void PlayCue(const std::string& key) {
        if (m_silentMode == false) {
            SoundCue* found = NULL;
            MapLookup(m_cues, key, found);
            if (found != NULL) {
                found->PlayIfElapsed(g_soundVolumePercent, 0, 0, false);
            }
        }
    }

    SoundCue* LoadCueFromSource(const std::string& key, CRezItm* source);
    SoundCue* LoadCueFromFile(const std::string& key, char* path);
    SoundCue* LoadNamedCue(CRezItm* source);

    i32 LoadFromTree(CRezDir* tree, const std::string& prefix, const std::string& separator);

    SoundCue* Lookup(const std::string& key);
    i32 RemoveWithPrefix(const std::string& prefix, const std::string& separator);
    i32 SumAudioBytes(const std::string& prefix);
    SoundCue* GetFirstCue();
    SoundCue* GetNextCueAfter(SoundCue* target);
    i32 ConfigurePrimaryFromFirstCue(i32 startPrimary);
    i32 HasWithPrefix(const std::string& prefix);
    std::string FindCueKey(SoundCue* target);
    i32 ConfigurePrimaryFromCue(SoundCue* cue, i32 startPrimary);

    i32 CueCount() const {
        return static_cast<i32>(m_cues.size());
    }

    void ClearCues();

    void RemoveCue(struct SoundCue* cue);

    virtual ~SoundCueRegistry()  ;

    i32 PlaySpatializedCue(const std::string& key, i32 sourceX, i32 maxPanOffsetPx, i32 fullPanOffsetPx);

    i32 BindSoundStream(b32 allowUnavailable);

    const std::map<std::string, SoundCue*>& Entries() const { return m_cues; }
    SoundStream* m_soundStream;

    b32 m_silentMode;
    i32 m_defaultReplayDelayMs;

private:
    std::map<std::string, SoundCue*> m_cues;
    void RegisterCue(SoundCue* cue, const std::string& key);
};

#endif
