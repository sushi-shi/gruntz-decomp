// GruntSpawnConfig.h - the grunt spawn/voice configuration manager.
//
// A plain (non-CObject) helper that loads grunt-spawn parameters from the bute
// config (g_buteMgr) and the game registry (g_gameReg), then builds + serves a
// CDWordArray of per-grunt CSpawnList voice lists (element dtor @0x99ca0). Recovered
// from the spawn-config method family at 0x11axxx-0x11cxxx: LoadGruntSpawnConfig
// (0x11afb0) reads "GruntPercent"/"GruntPriority"/"GruntVoice", BuildVoiceSoundList
// (0x11c210) reads the "VOICES_%s" bute sections, and the random-weighted picker
// at 0x11bee0 uses the g_gameReg LCG (timeGetTime-seeded). The dtor (0x85df0)
// lives in the 0x85xxx text region but is this class's teardown (Clear() then
// ~CArray on m_18) - trace discovery conflated the two regions under one stub
// label; both are this one class.
//
// Field names are placeholders (m_<hexoffset>); only the OFFSETS + code bytes are
// load-bearing (campaign doctrine). Layout recovered from the ctor/dtor + stores:
//   m_00  owner/parent ptr (->m_100 read by the "ready?" probe @0x11c830)
//   m_04  a config tree ptr (= owner->[0x30]); m_04->m_20 used by Clear()
//   m_08/m_0c  a sprite-pair (set by the GruntVoice loader @0x11af00; cleared @0x11af90)
//   m_10/m_14  a second owned-object pair (freed via m_04->m_20 in Clear())
//   m_18  CDWordArray of entry pointers (m_1c = data, m_20 = count)
//   m_2c  int = 0x64 (a percent/threshold seed)
#ifndef GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H
#define GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H

#include <rva.h>

#include <Ints.h>
#include <Mfc.h> // CPtrArray + <windows.h>

#include <Gruntz/GameRegistry.h>  // WwdGameReg / g_gameReg
#include <Gruntz/SpawnList.h>     // canonical CSpawnList / CSpawnEntry (voice lists)
#include <Gruntz/SpriteFactory.h> // the shared CSpriteFactory (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>     // CGameObject (the created sprite) + CGameObjAux

// Forward decls so the manager's typed slots need no view-casts (defs below / in
// the .cpp). m_04 is ONE config tree (its +0x08 sprite factory + +0x20 collection),
// m_08/m_0c hold voice sprites, m_10/m_14 owned voice streams.
class CGruntVoice; // folded CGruntVoice
struct CSpawnStream;
struct CSpawnTree;

// ---------------------------------------------------------------------------
// The per-grunt voice list (the CGruntSpawnConfig CDWordArray element) is the
// canonical CSpawnList of CSpawnEntry records (<Gruntz/SpawnList.h>). The old
// local "CSpawnEntry" here was a MISNOMER view of the LIST (its CPtrList@+0x00,
// m_1c cursor, m_20 last-picked) and "CVoiceSound" a view of the RECORD - both
// folded onto the canonicals (see SpawnList.h's six-view unification proof).
// The methods live where retail homed them: ~CSpawnList @0x99ca0 /
// DeleteAllEntries @0x9a450 / ClearFlags @0x9a420 in AreaMgr.cpp;
// AddVoiceSound @0x11c560 stays defined in GruntSpawnConfig.cpp; the record
// ctor @0x11c630 in SpawnEntry.cpp. (The old GetAt/FillName decls were stale
// Ghidra copies: 0x9a260 is CSpawnEntry::GetName.)
// ---------------------------------------------------------------------------

// The bute key getter (0x11bba0) is handed a (config, target) pair; it returns a
// pointer to one of `target`'s fields, chosen by config->m_170 (a 0..0x20 switch)
// plus two early specials on config->m_258. `target` is a flat bag of i32 slots
// the switch indexes by byte offset; modeled as a raw byte bag so each case is a
// `lea eax,[target+N]`.
struct CSpawnButeConfig {
    char m_00[0x170];
    i32 m_170; // +0x170  the switch selector
    char m_174[0x234 - 0x174];
    i32 m_234; // +0x234  a "has-slot" flag tested by case fallthrough
    char m_238[0x258 - 0x238];
    i32 m_258; // +0x258  early-special selector (0x39 / 0x3a)
};
struct CSpawnButeTarget {
    char m_data[0x2c0];
};

// The owner the spawn loader hangs off (m_00). Only its m_100 (a "ready" flag)
// and m_30 (the config tree m_04) are touched by this class.
// The 'WAV' resource resolver reached through owner->m_34 (0x13bff0 __thiscall;
// the voice builder's name filter + the weighted picker's tail both call it).
struct CSpawnResolver {};
SIZE_UNKNOWN(CSpawnResolver);
struct CSpawnOwner {
    char m_00[0x30];
    CSpawnTree* m_30;     // +0x30  -> the config tree stashed in m_04
    CSpawnResolver* m_34; // +0x34  the 'WAV' resource resolver
    char m_38[0x100 - 0x38];
    i32 m_100; // +0x100 the "ready" flag the @0x11c830 probe tests
};

// ---------------------------------------------------------------------------
// CGruntSpawnConfig - the spawn/voice config manager (no vtable; a value bag).
// ---------------------------------------------------------------------------
class CGruntSpawnConfig {
public:
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
    void Tick();             // reloc-masked (BroadcastCmd 4/7)
    void Teardown();         // reloc-masked (Close)
    void ResetPicks();       // 0x11c7f0 (DtorBody + reset entry m_20s)
    BOOL IsReady();          // 0x11c830 (out-of-line: m_00->m_100 != 0)
    ~CGruntSpawnConfig();    // 0x85df0

    // --- fields (placeholders; offsets load-bearing) ---
    CSpawnOwner* m_00;  // +0x00
    CSpawnTree* m_04;   // +0x04  = owner->m_30 (config tree)
    CGruntVoice* m_08;  // +0x08  voice-sprite pair
    CGruntVoice* m_0c;  // +0x0c
    CSpawnStream* m_10; // +0x10  owned voice-stream pair
    CSpawnStream* m_14; // +0x14
    // ::CPtrArray, not CDWordArray: retail's ctor/SetSize calls land in [0x1b4f0b,
    // 0x1b527e), whose head stamps ??_7CPtrArray@@6B@ (mfc_class --audit).
    CPtrArray m_18;     // +0x18  (vptr@0x18, m_pData@0x1c, m_nSize@0x20) - 0x14 bytes
    i32 m_2c;           // +0x2c  = 0x64
};

// --- the per-method helper externs (reloc-masked; no body) ---

// LoadGruntVoices builds a play request through the shared sprite factory
// (m_04->m_08, the CSpriteFactory whose CreateSprite lives at 0x1597b0; see
// <Gruntz/SpriteFactory.h>). The created instance is the shared CGameObject
// (<Gruntz/UserLogic.h>); this loader only touches its +0x7c CGameObjAux control
// block (the Init driver @+0x10, the m_18 setup slot it stashes as the voice
// stream handle).

// The voice-sprite stored in the m_08/m_0c pair. The teardown (0x11c7b0) calls
// its Reset (0x11a870, __thiscall); reloc-masked. m_68 holds the voice id the
// selective teardown (0x11c730) matches against.
// The owned-object free path in Clear() (m_04->m_20 sub-object's remove).
// The collection Clear() removes the m_08/m_0c entries from (reached as
// m_04->m_20). Remove is __thiscall on that collection (ecx) with the item as
// the one stack arg; modeled as a method so the call shape falls out.
// The config-tree collection at m_04+0x20: Clear() removes owned objects from it
// (Remove), and the spawn driver opens a voice stream through it (OpenStream). One
// collection, both methods (was split across CSpawnRemoveColl/CSpawnStreamFactory).
struct CSpawnRemoveColl {
    // Remove @0x1379d0 = SoundStream::DestroyVoice, OpenStream @0x137900 = SoundStream::OpenStream; cast at calls.
};
// The one config tree stashed in m_04 (= owner->m_30): its +0x08 is the sprite
// factory LoadGruntVoices builds voices through, its +0x20 the remove/stream
// collection (was three overlapping views: CSpawnTree/CSpawnConfigTree/CSpawnSpriteSource).
struct CSpawnTree {
    char m_pad00[8];
    CSpriteFactory* m_08; // +0x08  the sprite factory (LoadGruntVoices)
    char m_pad0c[0x20 - 0xc];
    CSpawnRemoveColl* m_20; // +0x20  the remove/stream collection
};

// The sprite-release helper (FUN_00537f00) is __thiscall on the sub-object at
// (object + 0x6c); modeled as a method so `lea ecx,[obj+0x6c]; call` falls out.
struct CSpriteReleasable {};

// The Rez heap free (0x1b9b82 _RezFree) the array-entry teardown runs after the
// element destructor. Reloc-masked.
extern "C" void RezFree(void* p); // _RezFree @0x1b9b82

// The picker tail builds a CString from the chosen entry then constructs a
// result through owner->m_00->m_34 (a name->id resolver). Modeled opaque.
extern "C" i32 SpawnResolveName(void* resolver, void* nameStr, i32 mode); // FUN_0053bff0

#endif // GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H
