// CGruntSpawnConfig.h - the grunt spawn/voice configuration manager.
//
// A plain (non-CObject) helper that loads grunt-spawn parameters from the bute
// config (g_buteMgr) and the game registry (g_gameReg), then builds + serves a
// CArray of voice/spawn entries (ClassUnknown_25 / element @0x99ca0). Recovered
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
#include <Mfc.h> // CDWordArray + <windows.h>

#include <Gruntz/CGameRegistry.h> // WwdGameReg / g_gameReg

// ---------------------------------------------------------------------------
// The per-grunt voice/spawn record (the CGruntSpawnConfig CDWordArray element;
// trace-placeholder ClassUnknown_25). It IS a CObList (block size 0xa) of voice-
// sound nodes (CVoiceSound, ClassUnknown_26) plus two trailing ints: m_1c (= 0 at
// ctor, the CObList count overlaps its own m_nCount at +0xc) and m_20 (= -1, the
// last-picked index the random picker reads).
//
//   ~CSpawnEntry  0x99ca0  - empties the node list (EmptyVoiceList) then the
//                            embedded CObList member dtor frees its blocks (the
//                            trailing ~CObList in the /GX frame). Called as an
//                            explicit dtor (e->~CSpawnEntry()) + RezFree(e) by
//                            CGruntSpawnConfig::Clear (a manual `delete e`).
//   EmptyVoiceList 0x9a450  - walks the CObList nodes, `delete (CVoiceSound*)node`
//                            on each held element, then m_list.RemoveAll().
//   AddVoiceSound  0x11c560 - new CVoiceSound(s); m_list.AddTail(node).
//
// GetAt (0x429a30) / FillName (0x49a260) serve entries; external/no-body.
// ---------------------------------------------------------------------------

// A voice-sound node held in the CSpawnEntry CObList. 12 bytes: a CString name +
// an int + a cached char* of the name's data. Its ctor (FUN_0051c630, the
// ClassUnknown_26 placeholder) is reloc-masked (defined in another TU); its
// teardown is `~CString + operator delete` (no vtable - a plain value node).
struct CVoiceSound {
    CVoiceSound(CString s); // 0x11c630 (__thiscall, ret 8)
    CString m_str;          // +0x00
    i32 m_04;               // +0x04  = 0
    char* m_08;             // +0x08  = m_str.m_pchData
};

struct CSpawnEntry {
    CSpawnEntry();
    ~CSpawnEntry();                          // 0x99ca0  (empties the list, then ~CObList member)
    void EmptyVoiceList();                   // 0x9a450  delete every held CVoiceSound, RemoveAll
    void AddVoiceSound(CString s, i32 flag); // 0x11c560

    void* GetAt(void* out);       // FUN_00429a30 (__thiscall)
    void* FillName(void* outStr); // FUN_0049a260 (__thiscall)

    CObList m_list; // +0x00  the voice-sound node list (block size 0xa); count @+0xc
    i32 m_1c;       // +0x1c  = 0
    i32 m_20;       // +0x20  = -1  last-picked index
};

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
struct CSpawnOwner {
    char m_00[0x30];
    void* m_30; // +0x30  -> the config tree stashed in m_04
    char m_34[0x100 - 0x34];
    i32 m_100; // +0x100 the "ready" flag the @0x11c830 probe tests
};

// ---------------------------------------------------------------------------
// CGruntSpawnConfig - the spawn/voice config manager (no vtable; a value bag).
// ---------------------------------------------------------------------------
class CGruntSpawnConfig {
public:
    BOOL Init(CSpawnOwner* owner);                           // 0x11adc0
    void Clear();                                            // 0x11ae30
    BOOL LoadGruntVoices();                                  // 0x11af00
    void ClearSprites();                                     // 0x11af90
    void* GetButeSlot(CSpawnButeConfig*, CSpawnButeTarget*); // 0x11bba0
    i32 PickWeighted(i32 index, i32 seed);                   // 0x11bee0
    BOOL BuildVoiceList();                                   // 0x11c1a0
    // The percent/priority-gated voice spawn driver (0x11afb0); re-homed from the
    // ApiCaller backlog. param_1=config, param_2=target/index, param_3=pick seed,
    // param_4=priority, param_5=percent.
    BOOL LoadGruntSpawnConfig(i32 param_1, i32 param_2, i32 param_3, i32 param_4, i32 param_5);
    void* BuildVoiceSoundList(i32 i); // 0x11c210 (defined in another TU; reloc-masked)
    void StopVoice(i32 id);           // 0x11c730 (selective per-id voice teardown)
    void DtorBody();                  // 0x11c7b0 (the 2-iter pair teardown)
    void ResetPicks();                // 0x11c7f0 (DtorBody + reset entry m_20s)
    BOOL IsReady();                   // 0x11c830
    ~CGruntSpawnConfig();             // 0x85df0

    // --- fields (placeholders; offsets load-bearing) ---
    CSpawnOwner* m_00; // +0x00
    void* m_04;        // +0x04  = owner->m_30 (config tree)
    void* m_08;        // +0x08  sprite-pair
    void* m_0c;        // +0x0c
    void* m_10;        // +0x10  owned-object pair
    void* m_14;        // +0x14
    CDWordArray m_18;  // +0x18  (vptr@0x18, m_pData@0x1c, m_nSize@0x20) - 0x14 bytes
    i32 m_2c;          // +0x2c  = 0x64
};

// --- the per-method helper externs (reloc-masked; no body) ---

// LoadGruntVoices builds a play request through the sprite factory. The factory
// (m_04->m_08, a CSpriteFactory) is invoked __thiscall; CreateSprite is modeled
// as a method so `mov ecx,factory; call` falls out with no stack cleanup mismatch.
struct CSpriteHandleSub {
    char m_00[0x10];
    void (*Activate)(void* self); // [m_7c + 0x10]  the slot-0x10 method
    char m_14[0x18 - 0x14];
    void* m_18; // +0x18  the result handle stored into the m_08/m_0c pair
};
struct CSpriteHandle {
    char m_00[0x7c];
    CSpriteHandleSub* m_7c; // +0x7c  vtable-ish slot bag (Activate + result)
};
struct CSpriteFactory08 {
    // __thiscall on the factory: (0,0,0, id, desc, flags). Returns the sprite.
    CSpriteHandle* CreateSprite(i32 a0, i32 a1, i32 a2, i32 id, char* desc, i32 flags); // 0x1597b0
};
struct CSpawnSpriteSource {
    char m_00[8];
    CSpriteFactory08* m_08; // +0x08  the factory
};

// The voice-sprite stored in the m_08/m_0c pair. The teardown (0x11c7b0) calls
// its Reset (0x11a870, __thiscall); reloc-masked. m_68 holds the voice id the
// selective teardown (0x11c730) matches against.
struct CSpawnVoice {
    void Reset();                               // 0x11a870
    i32 Setup(i32 a, i32 stream, i32 c, i32 d); // 0x11a7b7 (LoadGruntSpawnConfig play start)
    char m_pad00[0x68];
    i32 m_68; // +0x68  voice id
    i32 m_6c; // +0x6c  priority/rank (LoadGruntSpawnConfig gates on it)
};

// The "GruntVoice" sound descriptor blob the loader pushes (s_GruntVoice_0060a638).
DATA(0x0020a638)
extern char s_GruntVoice[];

// The owned-object free path in Clear() (m_04->m_20 sub-object's remove).
// The collection Clear() removes the m_08/m_0c entries from (reached as
// m_04->m_20). Remove is __thiscall on that collection (ecx) with the item as
// the one stack arg; modeled as a method so the call shape falls out.
struct CSpawnRemoveColl {
    void Remove(void* item); // 0x1379d0
};
struct CSpawnTree {
    char m_00[0x20];
    CSpawnRemoveColl* m_20; // +0x20  the collection Clear() removes from
};

// The sprite-release helper (FUN_00537f00) is __thiscall on the sub-object at
// (object + 0x6c); modeled as a method so `lea ecx,[obj+0x6c]; call` falls out.
struct CSpriteReleasable {
    void Release(); // 0x137f00
};

// The Rez heap free (0x1b9b82 _RezFree) the array-entry teardown runs after the
// element destructor. Reloc-masked.
extern "C" void RezFree(void* p); // _RezFree @0x1b9b82

// The picker tail builds a CString from the chosen entry then constructs a
// result through owner->m_00->m_34 (a name->id resolver). Modeled opaque.
extern "C" i32 SpawnResolveName(void* resolver, void* nameStr, i32 mode); // FUN_0053bff0

#endif // GRUNTZ_GRUNTZ_CGRUNTSPAWNCONFIG_H
