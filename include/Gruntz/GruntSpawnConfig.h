#ifndef GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H
#define GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H

#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // CPtrArray + <windows.h>

#include <Gruntz/GameRegistry.h>      // WwdGameReg / g_gameReg
#include <Gruntz/SpawnList.h>         // canonical CSpawnList / CSpawnEntry (voice lists)
#include <DDrawMgr/DDrawChildGroup.h> // the shared CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>         // CGameObject (the created sprite) + AnimWorkerObj

class CGruntVoice;  // folded CGruntVoice
struct StreamVoice; // m_10/m_14 owned voice streams (the real <Dsndmgr/StreamVoice.h>)
struct CSpawnTree;

// The bute key getter (0x11bba0) is handed a (config, target) pair; it returns a
// pointer to one of `target`'s fields, chosen by config->m_170 (a 0..0x20 switch)
// plus two early specials on config->m_258. `target` is a flat bag of i32 slots
// the switch indexes by byte offset; modeled as a raw byte bag so each case is a
// `lea eax,[target+N]`.
// param_1's +0x10 sub-object: LoadGruntSpawnConfig reads +0x188 as the currently-active
// voice id (compared to each voice's m_source; passed to CGruntVoice::Setup).
// @identity-TODO: the exact class of this active-voice holder is unrecovered (candidate
// CGruntHud, whose m_188 sits at the same offset), so it stays a modeled sub-object.
struct CSpawnActiveVoice {
    char m_00[0x188];
    i32 m_188; // +0x188  currently-active voice id
};
SIZE_UNKNOWN();
struct CSpawnButeConfig {
    char m_00[0x10];
    CSpawnActiveVoice* m_10; // +0x10  the active-voice sub-object (was the CSpawnGate view)
    char m_14[0x170 - 0x14];
    i32 m_170; // +0x170  the switch selector
    char m_174[0x234 - 0x174];
    i32 m_234; // +0x234  a "has-slot" flag tested by case fallthrough
    char m_238[0x258 - 0x238];
    i32 m_258; // +0x258  early-special selector (0x39 / 0x3a)
};
SIZE_UNKNOWN();
struct CSpawnButeTarget {
    char m_data[0x2c0];
};
SIZE_UNKNOWN();

struct CSpawnResolver {};
SIZE_UNKNOWN();
struct CSpawnOwner {
    char m_00[0x30];
    CSpawnTree* m_30;     // +0x30  -> the config tree stashed in m_04
    CSpawnResolver* m_34; // +0x34  the 'WAV' resource resolver
    char m_38[0x100 - 0x38];
    i32 m_100; // +0x100 the "ready" flag the @0x11c830 probe tests
};
SIZE_UNKNOWN();

class CGrunt; // CueA/CueSpawn first arg

class CGruntSpawnConfig {
public:
    // --- the on-screen cue receiver face (folded from Grunt.h's CGruntSpawnConfig -
    // the +0x60 registry object is ONE class wearing three names: spawn config ==
    // cue sink == the GruntzMgr "TimerObj" poll face). All declared-only:
    // reloc-masked thunk calls.
    void Cue(i32 a, i32 b, i32 c, i32 d, i32 e); // via thunk 0x33b4
    void Cue1(i32 a);                            // 1-arg cue (thunk_0x1163 -> 0x51c730)
    // 0x39f4: the on-screen event cue the per-tick game-object managers (CObjectTracker::Update)
    // fire on the registry's m_cueSink when the managed object is inside the viewport rect.

    BOOL Init(CSpawnOwner* owner); // 0x11adc0
    void Clear();                  // 0x11ae30
    BOOL LoadGruntVoices();        // 0x11af00
    void ClearSprites();           // 0x11af90 (out-of-line: m_08 = 0; m_0c = 0)
    void* GetButeSlot(CSpawnButeConfig*, CSpawnButeTarget*); // 0x11bba0
    i32 PickWeighted(i32 index, i32 seed);                   // 0x11bee0
    BOOL BuildVoiceList();                                   // 0x11c1a0
    // The percent/priority-gated voice spawn driver (0x11afb0); re-homed from the
    // ApiCaller backlog. param_1=config, param_2=target/index, param_3=pick seed,
    // param_4=priority, param_5=percent.
    BOOL LoadGruntSpawnConfig(i32 param_1, i32 param_2, i32 param_3, i32 param_4, i32 param_5);
    // Two sibling weighted grunt-voice spawn drivers (0x11b3b0 / 0x11b7c0), re-homed
    // from the ApiCaller backlog. Both @early-stop stubs (a full body caps at ~47%
    // on the /GX single-epilogue wall; the return-0 stub scores higher). 0x11b7c0 is
    // a file-scope __stdcall sibling (see the .cpp).
    i32 SpawnVoiceDriver(i32, i32, i32, i32, i32, i32); // 0x11b3b0
    CSpawnList* BuildVoiceSoundList(i32 i);             // 0x11c210 (defined in VoiceSoundList.cpp)
    i32 AnyVoicePlaying();   // 0x11c6c0 (either slot m_08/m_0c has a non-zero m_playFlags)
    i32 VoicePlaying(i32 i); // 0x11c700 (slot i's m_playFlags is non-zero)
    void StopVoice(i32 id);  // 0x11c730 (selective per-id voice teardown)
    void DtorBody();         // 0x11c7b0 (the 2-iter pair teardown; == m_timer->Flush)
    void Stop();             // reloc-masked (per-frame poll stop, via CGruntzMgr::m_timer)
    void ResetPicks();       // 0x11c7f0 (DtorBody + reset entry m_20s)
    BOOL IsReady();          // 0x11c830 (out-of-line: m_00->m_100 != 0)
    ~CGruntSpawnConfig();    // 0x85df0

    // --- fields (placeholders; offsets load-bearing) ---
    CSpawnOwner* m_owner;     // +0x00
    CSpawnTree* m_configTree; // +0x04  = owner->m_30 (config tree)
    CGruntVoice* m_voice0;    // +0x08  voice-sprite pair
    CGruntVoice* m_voice1;    // +0x0c
    StreamVoice* m_stream0;   // +0x10  owned voice-stream pair (the real Dsndmgr StreamVoice)
    StreamVoice* m_stream1;   // +0x14
    // ::CPtrArray, not CDWordArray: retail's ctor/SetSize calls land in [0x1b4f0b,
    // 0x1b527e), whose head stamps ??_7CPtrArray@@6B@ (mfc_class --audit).
    CPtrArray m_voiceLists; // +0x18  (vptr@0x18, m_pData@0x1c, m_nSize@0x20) - 0x14 bytes
    i32 m_voiceVolume;      // +0x2c  = 0x64
};
SIZE_UNKNOWN();

struct CSpawnTree {
    char m_pad00[8];
    CDDrawChildGroup* m_08; // +0x08  the sprite factory (LoadGruntVoices)
    char m_pad0c[0x20 - 0xc];
    class SoundStream* m_20; // +0x20  the sound-stream mgr (DestroyVoice/OpenStream;
                             //        ex the empty CSpawnRemoveColl marker view)
};
SIZE_UNKNOWN();

#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

extern "C" i32 SpawnResolveName(void* resolver, void* nameStr, i32 mode); // FUN_0053bff0

#endif // GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H
