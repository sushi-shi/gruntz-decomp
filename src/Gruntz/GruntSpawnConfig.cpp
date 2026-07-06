// GruntSpawnConfig.cpp - the grunt spawn/voice configuration manager.
//
// Ten __thiscall methods reconstructed in ascending-RVA order. The dtor (0x85df0)
// lives in the 0x85xxx text region (next to ~CGruntzMapMgr) but is this class's
// teardown; the other nine sit in the spawn-config family at 0x11axxx-0x11cxxx.
// The class is a plain value bag (no vtable): a CDWordArray (m_18) of voice/spawn
// entries plus the percent/priority/voice config seeded from g_buteMgr / g_gameReg.
//
// Field names are placeholders (m_<hexoffset>); only OFFSETS + code bytes are
// load-bearing. Engine callees / globals are reloc-masked (no body). See the
// header for the recovered layout + the conflated-region note.
#include <Gruntz/GruntSpawnConfig.h>
#include <Dsndmgr/StreamFeeder.h>
#include <Gruntz/GameRegistry.h>

#include <Bute/ButeMgr.h> // CButeMgr g_buteMgr (GetIntDef)
#include <rva.h>

// ===========================================================================
// CGruntSpawnConfig::~CGruntSpawnConfig  (0x85df0)
// ===========================================================================
// Clear() the array then run ~CDWordArray on m_18. The destructible m_18 forces
// the /GX EH frame. The empty body folds the inline teardown.
RVA(0x00085df0, 0x4a)
CGruntSpawnConfig::~CGruntSpawnConfig() {
    Clear();
}

// (0x99ca0 ~CSpawnList + 0x9a450 CSpawnList::DeleteAllEntries re-homed to
// src/Gruntz/AreaMgr.cpp - their retail TU band - so the dtor inline-folds into
// ~CAreaMgr exactly as retail. Clear() below keeps the retail extern-call shape.)

// ===========================================================================
// CGruntSpawnConfig::Init  (0x11adc0)
// ===========================================================================
// Bind to an owner and seed the config tree pointer, then build the voice list.
// On a null owner bail with 0. Stash owner (m_00) and owner->m_30 (m_04, the
// config tree), zero the sprite/object pairs (m_08..m_14), seed m_2c = 0x64, and
// run BuildVoiceList(); return its result negated/double-negated (a BOOL).
//
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): structurally
// byte-exact (every offset/immediate/call matches) but retail materializes 0 late
// via `test eax,eax` + a `lea esi,[ecx+8]` store-group, while the recompile pins
// xor edx,edx early (cmp edx,eax) and stores flat - a 1-instr phase shift not
// source-steerable. ~55% on a 68-byte fn; deferred to the final sweep.
RVA(0x0011adc0, 0x44)
BOOL CGruntSpawnConfig::Init(CSpawnOwner* owner) {
    if (owner == 0) {
        return 0;
    }
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_10 = 0;
    m_14 = 0;
    m_00 = owner;
    m_2c = 0x64;
    m_04 = owner->m_30;
    return BuildVoiceList() != 0;
}

// ===========================================================================
// CGruntSpawnConfig::Clear  (0x11ae30)
// ===========================================================================
// Free every entry in m_18 (dtor + RezFree), empty the array, then drop the two
// owned objects (m_10/m_14) from m_04's sub-collection and zero the sprite/object
// pairs. The Remove call shape matches retail byte-exact.
//
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): the loop
// bound (i<GetSize()) and `e!=0`/`p!=0` checks let MSVC pin a callee-saved zero
// (ebp) across the whole body, where retail keeps only ebx and uses `test`/fresh
// zeros + a 3-way pointer-grouped tail zeroing. Logic byte-exact in the middle
// (Remove call, idiv-free); ~68% from the zero-reg phase shift. Deferred.
RVA(0x0011ae30, 0x95)
void CGruntSpawnConfig::Clear() {
    for (i32 i = 0; i < m_18.GetSize(); i++) {
        CSpawnList* e = (CSpawnList*)m_18[i];
        if (e != 0) {
            e->~CSpawnList(); // extern call (dtor defined in AreaMgr.cpp) - retail shape
            RezFree(e);
        }
    }
    m_18.SetSize(0, -1);
    if (m_04 != 0 && m_04->m_20 != 0) {
        void** p = (void**)&m_10;
        for (i32 k = 0; k < 2; k++) {
            if (p[0] != 0) {
                m_04->m_20->Remove(p[0]);
                p[0] = 0;
            }
            p++;
        }
    }
    m_00 = 0;
    m_04 = 0;
    m_08 = 0;
    m_0c = 0;
    m_10 = 0;
    m_14 = 0;
}

// ===========================================================================
// CGruntSpawnConfig::LoadGruntVoices  (0x11af00)
// ===========================================================================
// Build two sprites from the "GruntVoice" descriptor and stash them in the
// sprite-pair (m_08/m_0c). For each of the two iterations: get the source object
// (m_04->m_08), make a sprite via CreateSprite, call its slot-0x10 method, then
// read the sprite's m_7c->m_18 into the pair slot; bail with 0 on a null result.
RVA(0x0011af00, 0x62)
BOOL CGruntSpawnConfig::LoadGruntVoices() {
    ClearSprites();
    i32 i = 0;
    void** slot = (void**)&m_08;
    for (; i < 2; i++, slot++) {
        CGameObject* spr = m_04->m_08->CreateSprite(0, 0, 0, 0xdbba1, "GruntVoice", 0x4040003);
        spr->m_7c->Init(spr);
        void* got = spr->m_7c->m_logic;
        *slot = got;
        if (got == 0) {
            return 0;
        }
    }
    return 1;
}

// ===========================================================================
// CGruntSpawnConfig::ClearSprites  (0x11af90)
// ===========================================================================
// Zero the sprite-pair (m_08/m_0c).
//
// @early-stop
// addressing-mode wall: retail emits `add ecx,8; mov [ecx]; mov [ecx+4]` (a
// this-repositioned store-pair) for these two zeros; every C++ spelling
// (m_08=0;m_0c=0 / *p++=0 / a 2-iter loop) folds to the shorter `mov [ecx+8];
// mov [ecx+c]`. 11-byte fn, <1% unit weight; not source-steerable. Deferred.
RVA(0x0011af90, 0xb)
void CGruntSpawnConfig::ClearSprites() {
    m_08 = 0;
    m_0c = 0;
}

// The bute manager singleton (?g_buteMgr, RVA 0x2453d8); DATA label owned by the
// bute TU, declared extern here so the `ecx=&g_buteMgr; call GetIntDef` reloc-masks.
extern CButeMgr g_buteMgr;

// ===========================================================================
// CGruntSpawnConfig::LoadGruntSpawnConfig  (0x11afb0)
// ===========================================================================
// The percent/priority-gated voice spawn driver. Ensure the voices are loaded and
// the owner is ready, then read the per-section percent/priority from the bute
// config (formatted "SG%i"/"G%i" section names), roll the percent gate, skip if a
// higher-priority voice is already active, pick a weighted entry, duck the
// currently-playing voice's volume, open/configure the chosen stream, and play it.
// /GX EH frame from the two CString temporaries.
//
// @early-stop
// /GX EH-state-numbering wall (docs/patterns/eh-state-numbering-base.md, topic:eh):
// the instruction selection, the volume-duck branch tree, the stream open/configure
// path, and the two CString Format/dtor temporaries are byte-faithful; the residual
// is the trylevel slot threading + the shared scope-exit dtor block the /GX state
// machine emits (the early-out gotos all funnel through one CString teardown, where
// retail's state ids differ). Logic complete; deferred to the final sweep.
// g_gameReg viewed for the LCG rand (__thiscall, ecx = the registry) + the master
// volume the duck halves.
// The bute config gate (param_1): m_10->m_188 is the currently-active voice id.
struct CSpawnGateInner {
    char m_pad00[0x188];
    i32 m_188; // +0x188
};
struct CSpawnGate {
    char m_pad00[0x10];
    CSpawnGateInner* m_10; // +0x10
};
// One owned voice stream (m_10/m_14): a DirectSoundMgr with a +0x6c releasable
// sub-sprite, plus the source/configure/volume setters.
struct CSpawnStream {
    i32 SetSource(i32 src);                    // 0x1374c0
    i32 Configure(i32 a, i32 b, i32 c, i32 d); // 0x137520
    void SetVolumeByIndex(i32 vol);            // 0x1355c0
    char m_pad00[0x6c];
    CSpriteReleasable m_6c; // +0x6c  (Release, 0x137f00)
};
// (OpenStream lives on the unified CSpawnRemoveColl at m_04->m_20; see the header.)

// The game registry pointer at *0x64556c (reloc-masked DATA; DATA label owned by
// another TU, but a fresh decl here is byte-neutral - the reference is by address).
DATA(0x0024556c)
extern CGameRegistry* g_gameReg;

RVA(0x0011afb0, 0x321)
BOOL CGruntSpawnConfig::LoadGruntSpawnConfig(
    i32 param_1,
    i32 param_2,
    i32 param_3,
    i32 param_4,
    i32 param_5
) {
    if (m_08 == 0 && !LoadGruntVoices()) {
        return 0;
    }
    if (param_1 == 0) {
        return 0;
    }
    if (!IsReady()) {
        return 0;
    }
    void* index = GetButeSlot((CSpawnButeConfig*)param_1, (CSpawnButeTarget*)param_2);
    CString local_10;
    CString local_14;
    local_14.Format("SG%i", (int)index);
    local_10.Format("G%i", param_2);
    if (param_5 == -1) {
        param_5 = g_buteMgr.GetIntDef((LPCTSTR)local_14, "Per", -1);
        if (param_5 == -1) {
            param_5 = g_buteMgr.GetIntDef("GruntPercent", (LPCTSTR)local_10, 0);
        }
    }
    if (param_5 < 100 && param_5 < g_gameReg->Rand() % 0x65) {
        return 0;
    }
    if (param_4 == -1) {
        param_4 = g_buteMgr.GetIntDef((LPCTSTR)local_14, "Pri", -1);
        if (param_4 == -1) {
            param_4 = g_buteMgr.GetIntDef("GruntPriority", (LPCTSTR)local_10, 1);
        }
    }
    CSpawnVoice** voices = &m_08;
    for (i32 i = 0; i < 2; i++) {
        if (param_4 <= voices[i]->m_6c) {
            return 0;
        }
    }
    i32 src = PickWeighted((i32)index, param_3);
    if (src == 0 || m_04->m_20 == 0) {
        return 0;
    }
    CSpawnVoice* v8 = m_08;
    CSpawnVoice* v0c = m_0c;
    i32 a = v8->m_6c;
    i32 b = v0c->m_6c;
    i32 c = v8->m_68;
    i32 d = v0c->m_68;
    CSpawnStream** streams = &m_10;
    CSpawnGate* gate = (CSpawnGate*)param_1;
    i32 chosen;
    if (b < a) {
        chosen = 1;
        if (c == gate->m_10->m_188) {
            chosen = 0;
            if (b != 0 && streams[1] != 0) {
                streams[1]->SetVolumeByIndex(g_gameReg->m_inputStateVal / 2);
            }
        } else if (a != 0 && streams[0] != 0) {
            streams[0]->SetVolumeByIndex(g_gameReg->m_inputStateVal / 2);
        }
    } else {
        chosen = 0;
        if (d == gate->m_10->m_188) {
            chosen = 1;
            if (a != 0 && streams[0] != 0) {
                streams[0]->SetVolumeByIndex(g_gameReg->m_inputStateVal / 2);
            }
        } else if (b != 0 && streams[1] != 0) {
            streams[1]->SetVolumeByIndex(g_gameReg->m_inputStateVal / 2);
        }
    }
    if (streams[chosen] == 0) {
        streams[chosen] = m_04->m_20->OpenStream(src, 0x5000, 0x1400, 0x100e0, 0, 0);
        if (streams[chosen] == 0) {
            return 0;
        }
    }
    CSpawnStream* stream = streams[chosen];
    i32 vol = m_2c;
    ((StreamFeeder*)&stream->m_6c)->Pause();
    if (stream->SetSource(src) != 0) {
        stream->Configure(vol, 0, 0, 0);
    }
    CSpawnVoice* voice = voices[chosen];
    return voice->Setup(gate->m_10->m_188, (i32)stream, param_4, 0) != 0;
}

// ===========================================================================
// CGruntSpawnConfig::SpawnVoiceDriver (0x11b3b0) + SpawnVoiceDriverStd (0x11b7c0)
// ===========================================================================
// The two sibling weighted grunt-voice spawn drivers (percent LCG gate @0x6c1288,
// priority reject, weighted pick, lazy sprite create, CSpawnVoice::Setup tail).
// Re-homed from the ApiCaller backlog by RVA proximity (dead-centre of the
// 0x11axxx-0x11cxxx CGruntSpawnConfig family).
//
// @early-stop
// /GX EH single-epilogue wall: the complete weighted-spawn body was reconstructed
// and builds, but caps at ~47% because cl duplicates the frame-teardown per
// return-site while retail funnels to one shared `jmp` epilogue. The return-0 stub
// scores 73-83% via the smaller-fn normalization artifact, so the highest-% version
// (this stub) is kept per the REVERT rule. Final-sweep candidates.
RVA(0x0011b3b0, 0x338)
i32 CGruntSpawnConfig::SpawnVoiceDriver(i32, i32, i32, i32, i32, i32) {
    return 0;
}

// @early-stop
// twin of 0x11b3b0: same /GX EH single-epilogue wall; file-scope __stdcall sibling,
// stub kept as the highest-% version (full body ~47% vs stub-artifact 73-83%).
RVA(0x0011b7c0, 0x304)
i32 __stdcall SpawnVoiceDriverStd(i32, i32, i32, i32, i32) {
    return 0;
}

// ===========================================================================
// CGruntSpawnConfig::GetButeSlot  (0x11bba0)
// ===========================================================================
// Return a pointer to one of `target`'s fields, chosen by config->m_170 (a switch
// over 0..0x20), plus two early specials on config->m_258 (0x3a -> +0x17c,
// 0x39 -> +0x104). A null config or out-of-range selector returns 0.
//
// @early-stop
// jump-table scoring-artifact wall (docs/patterns/jumptable-data-overlap.md): the
// dispatch, the index/jump table, and every case body are byte-IDENTICAL to retail
// (verified by raw byte-compare of the head + case blocks in source-case order).
// objdiff scores ~73% only because the inline `.text` jump-table region carries a
// base reloc against a $L label vs the target's switchdataD self-reloc. The code
// IS matched; the % undercounts it. No source change applies - stop chasing.
RVA(0x0011bba0, 0x1f4)
void* CGruntSpawnConfig::GetButeSlot(CSpawnButeConfig* config, CSpawnButeTarget* target) {
    if (config == 0) {
        return 0;
    }
    if (config->m_258 == 0x3a) {
        return target->m_data + 0x17c;
    }
    if (config->m_258 == 0x39) {
        return target->m_data + 0x104;
    }
    switch ((u32)config->m_170) {
        case 0:
            return target->m_data + 0x154;
        case 1:
            return target->m_data + 0x3c;
        case 2:
            return target->m_data + 0x50;
        case 3:
            return target->m_data + 0x64;
        case 4:
            return target->m_data + 0x78;
        case 5:
            return target->m_data + 0x8c;
        case 6:
            return target->m_data + 0xa0;
        case 7:
            return target->m_data + 0xc8;
        case 8:
            return target->m_data + 0xdc;
        case 9:
            return target->m_data + 0xf0;
        case 10:
            return target->m_data + 0x140;
        case 11:
            return target->m_data + 0x190;
        case 12:
            return target->m_data + 0x1b8;
        case 13:
            return target->m_data + 0x1cc;
        case 14:
            return target->m_data + 0x1e0;
        case 15:
            return target->m_data + 0x1f4;
        case 16:
            return target->m_data + 0x21c;
        case 17:
            return target->m_data + 0x230;
        case 18:
            if (config->m_234 != 0) {
                return target->m_data + 0x258;
            }
            return target->m_data + 0x244;
        case 19:
            return target->m_data + 0x26c;
        case 20:
            return target->m_data + 0x280;
        case 21:
            return target->m_data + 0x294;
        case 22:
            return target->m_data + 0x2a8;
        case 23:
            return target->m_data + 0x0;
        case 24:
            return target->m_data + 0x14;
        case 25:
            return target->m_data + 0x28;
        case 26:
            return target->m_data + 0xb4;
        case 27:
            return target->m_data + 0x118;
        case 28:
            return target->m_data + 0x12c;
        case 29:
            return target->m_data + 0x168;
        case 30:
            return target->m_data + 0x1a4;
        case 31:
            return target->m_data + 0x208;
        case 32:
            return target->m_data + 0x2bc;
        default:
            return 0;
    }
}

// @early-stop
// rand()-inline wall + >512B: the weighted-random entry picker. MSVC inlines the
// MSVC LCG rand() (x=x*214013+2531011, the 0xcd00 body) THREE times - once with
// the seeded-flag check (mov al,[0x6c127d]; test al,1; or al,1; call ebp) and
// twice in the inner re-roll loop - plus an idiv-by-count reduction and a CString
// tail under the /GX EH frame. The seed/seeded globals (0x6c1288/0x6c127d) and
// the timeGetTime ptr (0x6c4650) are reloc-masked DATA. Reproducing the
// inline-vs-call rand split + the idiv scheduling from a single source spelling
// is a known wall; left as a documented stub for the final sweep / a leaf-first
// redo (per matcher.md: a >512B non-converging fn is left stubbed, not half-done).
RVA(0x0011bee0, 0x230)
i32 CGruntSpawnConfig::PickWeighted(i32 index, i32 seed) {
    (void)index;
    (void)seed;
    return 0;
}

// ===========================================================================
// CGruntSpawnConfig::BuildVoiceList  (0x11c1a0)
// ===========================================================================
// Size the array empty, then fill 0x4af entries: for i in [1, 0x4b0) grow the
// array with BuildVoiceSoundList(i). Returns 1.
RVA(0x0011c1a0, 0x46)
BOOL CGruntSpawnConfig::BuildVoiceList() {
    m_18.SetSize(0, -1);
    m_18.SetAtGrow(0, 0);
    for (i32 i = 1; i < 0x4b0; i++) {
        m_18.SetAtGrow(i, (DWORD)BuildVoiceSoundList(i));
    }
    return 1;
}

// ===========================================================================
// CSpawnList::AddVoiceSound  (0x11c560)
// ===========================================================================
// Allocate a record (operator new 0xc), construct it from the (by-value) name
// CString AND the `flag` arg (CSpawnEntry's 2nd ctor param, stored at m_data),
// and append it to the list (AddTail). The by-value CString param + the inner
// copy construct the /GX frame. EXACT once the flag is passed through (the
// earlier "frame-size wall" was really the dropped 2nd ctor arg - a `push flag`).
// The (CObject*) cast is language-forced: CObList stores raw non-CObject records
// here exactly as retail does.
RVA(0x0011c560, 0x91)
void CSpawnList::AddVoiceSound(CString s, i32 flag) {
    CSpawnEntry* node = new CSpawnEntry(s, flag);
    if (node != 0) {
        m_list.AddTail((CObject*)node);
    }
}

// ===========================================================================
// CGruntSpawnConfig::StopVoice  (0x11c730)
// ===========================================================================
// Selective per-id teardown: of the two voice slots (m_08, m_0c), find the one
// whose voice id (m_68) matches `id`. For that slot: if its paired object
// (m_10/m_14) is set, release the sub-sprite at +0x6c; then if the voice itself
// (m_08/m_0c) is set, reset it. Both ->m_68 are read eagerly (no null guard).
RVA(0x0011c730, 0x5c)
void CGruntSpawnConfig::StopVoice(i32 id) {
    i32 tag08 = m_08->m_68;
    i32 tag0c = m_0c->m_68;
    if (tag08 == id) {
        if (m_10 != 0) {
            ((StreamFeeder*)&m_10->m_6c)->Pause();
        }
        if (m_08 != 0) {
            m_08->Reset();
        }
    } else if (tag0c == id) {
        if (m_14 != 0) {
            ((StreamFeeder*)&m_14->m_6c)->Pause();
        }
        if (m_0c != 0) {
            m_0c->Reset();
        }
    }
}

// ===========================================================================
// CGruntSpawnConfig::DtorBody  (0x11c7b0)
// ===========================================================================
// The 2-iteration sprite-pair teardown over (m_08,m_10) then (m_0c,m_14): if the
// paired object (m_10/m_14) is set, release its sub-sprite (+0x6c); if the sprite
// (m_08/m_0c) is set, reset it (CGruntVoice::Reset @0x11a870).
RVA(0x0011c7b0, 0x2d)
void CGruntSpawnConfig::DtorBody() {
    void** p = (void**)&m_08;
    for (i32 k = 0; k < 2; k++) {
        if (p[2] != 0) {
            ((StreamFeeder*)&((CSpawnStream*)p[2])->m_6c)->Pause();
        }
        if (p[0] != 0) {
            ((CSpawnVoice*)p[0])->Reset();
        }
        p++;
    }
}

// ===========================================================================
// CGruntSpawnConfig::ResetPicks  (0x11c7f0)
// ===========================================================================
// Run the sprite-pair teardown (DtorBody @0x11c7b0), then walk the m_18 voice-list
// array and reset every non-null list's last-picked index (+0x20) to -1.
RVA(0x0011c7f0, 0x2b)
void CGruntSpawnConfig::ResetPicks() {
    DtorBody();
    for (i32 i = 0; i < m_18.GetSize(); i++) {
        CSpawnList* e = (CSpawnList*)m_18[i];
        if (e != 0) {
            e->m_lastPicked = -1;
        }
    }
}

// ===========================================================================
// CGruntSpawnConfig::IsReady  (0x11c830)
// ===========================================================================
// Return whether the owner is ready (owner->m_100 != 0).
RVA(0x0011c830, 0x12)
BOOL CGruntSpawnConfig::IsReady() {
    return m_00->m_100 != 0;
}

SIZE_UNKNOWN(CGruntSpawnConfig);
SIZE_UNKNOWN(CSpawnButeConfig);
SIZE_UNKNOWN(CSpawnButeTarget);
SIZE_UNKNOWN(CSpawnGate);
SIZE_UNKNOWN(CSpawnGateInner);
SIZE_UNKNOWN(CSpawnOwner);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CSpawnRemoveColl);
SIZE_UNKNOWN(CSpawnStream);
SIZE_UNKNOWN(CSpawnTree);
SIZE_UNKNOWN(CSpawnVoice);
SIZE_UNKNOWN(CSpriteReleasable);
