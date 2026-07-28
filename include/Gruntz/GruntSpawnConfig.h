#ifndef GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H
#define GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H

#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // CPtrArray + <windows.h>

#include <Gruntz/GameRegistry.h>      // WwdGameReg / g_gameReg
#include <Gruntz/SpawnList.h>         // canonical CSpawnList / CSpawnEntry (voice lists)
#include <DDrawMgr/DDrawChildGroup.h> // the shared CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>         // CGameObject (the created sprite) + AnimWorkerObj
#include <Rez/RezAlloc.h>             // RezAlloc/RezFree (the global allocator pair)

class CGruntVoice;  // folded CGruntVoice
struct StreamVoice; // m_10/m_14 owned voice streams (the real <Dsndmgr/StreamVoice.h>)
// The owner/config-tree pair. The ex `CSpawnOwner` / `CSpawnTree` pad-structs were fake
// views of these two REAL classes; dissolved 2026-07-27 (see the m_owner note below).
class CGruntzMgr;
class CDDrawSurfaceMgr;

// A voice-list id is a dense (band, cue) pair: `id = VOICE_CUES_PER_BAND * band + cue`.
// The band comes from the grunt's own state (CGrunt::m_entranceReason, plus two early
// specials on m_gruntKind); the cue is what the caller announces (arrival, death, ...).
//
// PROVEN, not inferred:
//   - all 36 arms of GetButeSlot's switch are exact multiples of 20 decimal, covering
//     bands 0..35 with no gap and no duplicate (a permutation, so the switch really is
//     a band lookup table);
//   - BuildVoiceList (0x11c1a0) fills exactly 0x4b0 = 60 * 20 entries;
//   - every retail caller of LoadGruntSpawnConfig passes a small literal cue
//     (1, 2, 3, 8, 0xa, 0xb, 0xc) into `m_cueSink`, all well inside [0, 20).
// The individual band MEANINGS are not recovered, so the arms carry their band number
// and no invented name.
enum {
    VOICE_CUES_PER_BAND = 20
};

// The bute key getter (0x11bba0) is handed (config, cue) and returns that i32 id. It is
// integer arithmetic, NOT a pointer: retail's sole caller (LoadGruntSpawnConfig 0x11afb0)
// pushes the result straight into `CString::Format("SG%i", it)`, and PickWeighted
// bounds-checks it `< 0` / `== 0` / `>= m_voiceLists.GetSize()` before using it as
// `m_voiceLists[it]`. The ex-`CSpawnButeTarget` 0x2c0-byte "raw byte bag" was a fake
// view of this integer (its `m_data + N` spelled the band constants as struct
// offsets); dissolved 2026-07-27.
// The band selectors are read off the CGrunt itself, not a view. The ex
// `CSpawnButeConfig` pad-struct's +0x10 / +0x170 / +0x234 / +0x258 are CGrunt's
// m_object / m_entranceReason / m_coordToggle / m_gruntKind at exactly those offsets
// (m_gruntKind == the 0x39/0x3a early-special selector is the semantic clincher:
// GetButeSlot's two pre-switch specials ARE grunt kinds), and the ex
// `CSpawnActiveVoice` at +0x10 was the bound CGameObject, whose +0x188 is the object id
// every CGruntVoice caches as m_source - CWarlord already passes that same
// m_object->m_188 into the sibling driver. Both dissolved 2026-07-27.
class CGrunt; // the voice-cue subject (GetButeSlot / LoadGruntSpawnConfig arg 1)

class CGruntSpawnConfig {
public:
    BOOL Init(CGruntzMgr* owner);          // 0x11adc0
    void Clear();                          // 0x11ae30
    BOOL LoadGruntVoices();                // 0x11af00
    void ClearSprites();                   // 0x11af90 (out-of-line: m_08 = 0; m_0c = 0)
    i32 GetButeSlot(CGrunt* who, i32 cue); // 0x11bba0
    // The weighted voice-line picker (0x11bee0): resolve m_voiceLists[voiceId] to a
    // .WAV parse record, re-rolling up to 5 times to avoid repeating the list's last
    // pick. `which` selects an explicit entry; -1 (or out of range) means roll.
    // Returns the CSymParser record - proven by the tail `mov esi,eax` over
    // ?ResolveQualified@CSymParser@@QAEPAUCParseSource@@PBDI@Z, returned unchanged.
    struct CParseSource* PickWeighted(i32 voiceId, i32 which); // 0x11bee0
    BOOL BuildVoiceList();                                     // 0x11c1a0
    // The percent/priority-gated voice spawn driver (0x11afb0); re-homed from the
    // ApiCaller backlog. `cue` picks the voice band slot (see VOICE_CUES_PER_BAND);
    // `which` selects an explicit list entry (-1 = roll one). `priority`/`percent`
    // default from the bute "Pri"/"Per" keys - which is what names them.
    BOOL LoadGruntSpawnConfig(class CGrunt* who, i32 cue, i32 which, i32 priority, i32 percent);
    // Two overloaded weighted grunt-voice spawn drivers (0x11b3b0 / 0x11b7c0).
    // Both consume this in ecx and return with callee-cleaned stack arguments; the
    // five-argument overload was formerly mis-modeled as a free __stdcall sibling.
    // The subject is a CGrunt, PROVEN by the body: it reads `who->m_object->m_188`
    // (+0x10 then +0x188) to derive the ducking object id, and all 75 call sites pass
    // a CGrunt `this` or 0. (Was `void* spawner` - the mangled PAX is retail's, but
    // the RVA() binding is by address, so the real type costs nothing.)
    i32 SpawnVoiceDriver(
        CGrunt* who,
        i32 voiceId,
        i32 which,
        i32 objId,
        i32 priority,
        i32 percent
    ); // 0x11b3b0
    // The five-argument twin takes the ducking object id DIRECTLY (settled by the
    // 0x11b7c0 body: `cmp ecx,[esp+0x24]` against each voice's m_source), which is
    // what CWarlord passes as m_object->m_188.
    i32 SpawnVoiceDriver(i32 objId, i32 voiceId, i32 which, i32 priority, i32 percent); // 0x11b7c0
    CSpawnList* BuildVoiceSoundList(i32 i); // 0x11c210 (defined in VoiceSoundList.cpp)
    i32 AnyVoicePlaying();   // 0x11c6c0 (either slot m_08/m_0c has a non-zero m_playFlags)
    i32 VoicePlaying(i32 i); // 0x11c700 (slot i's m_playFlags is non-zero)
    void StopVoice(i32 id);  // 0x11c730 (selective per-id voice teardown)
    void PauseAllVoices();   // 0x11c7b0 (the 2-iter pair teardown; == m_timer->Flush)
    void Stop();             // reloc-masked (per-frame poll stop, via CGruntzMgr::m_timer)
    void ResetPicks();       // 0x11c7f0 (PauseAllVoices + reset entry m_20s)
    BOOL IsReady();          // 0x11c830 (out-of-line: m_owner->m_isVoiceEnabled != 0)
    ~CGruntSpawnConfig();    // 0x85df0

    // --- fields (placeholders; offsets load-bearing) ---
    // +0x00 the owning CGruntzMgr. PROVEN, not inferred: Init (0x11adc0) has exactly ONE
    // retail caller - CGruntzMgr::Run @0x84018 passes its own `this` - and every field the
    // ex-CSpawnOwner view spelled lands on a real CGruntzMgr member at the same offset:
    // +0x30 m_world (CDDrawSurfaceMgr*), +0x34 m_symParser (CSymParser*, the 'WAV' resolver
    // that both readers reach via ?ResolveQualified@CSymParser@@...), +0x100
    // m_isVoiceEnabled (exactly what IsReady @0x11c830 probes).
    CGruntzMgr* m_owner;
    // +0x04 = owner->m_world. The ex-CSpawnTree view's two fields ARE CDDrawSurfaceMgr's:
    // +0x08 m_childGroup (CDDrawChildGroup::CreateSprite) and +0x20 m_soundStream
    // (SoundStream::DestroyVoice/OpenStream).
    CDDrawSurfaceMgr* m_configTree;
    CGruntVoice* m_voices[2]; // +0x08/+0x0c  voice-sprite pair (indexed everywhere)
    // +0x10/+0x14: the owned voice-stream PAIR (the real Dsndmgr StreamVoice). It is
    // an array, not two scalars - Clear/PickVoice/PauseAllVoices all walk it with one cursor.
    StreamVoice* m_streams[2];
    // ::CPtrArray, not CDWordArray: retail's ctor/SetSize calls land in [0x1b4f0b,
    // 0x1b527e), whose head stamps ??_7CPtrArray@@6B@ (mfc_class --audit).
    CPtrArray m_voiceLists; // +0x18  (vptr@0x18, m_pData@0x1c, m_nSize@0x20) - 0x14 bytes
    i32 m_voiceVolume;      // +0x2c  = 0x64
};
SIZE_UNKNOWN();

extern "C" i32 SpawnResolveName(void* resolver, void* nameStr, i32 mode); // FUN_0053bff0

#endif // GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H
