// CGruntSpawnConfig.cpp - the grunt spawn/voice configuration manager.
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
#include <Gruntz/CGruntSpawnConfig.h>

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

// ===========================================================================
// CSpawnEntry::~CSpawnEntry  (0x99ca0)
// ===========================================================================
// Empty the voice-sound node list (EmptyVoiceList), then the embedded CObList
// member dtor frees its blocks (the trailing ~CObList in the /GX frame). The
// destructible m_list forces the EH frame; the trylevel 0->-1 wraps EmptyVoiceList
// so the member ~CObList still runs on unwind.
//
// @early-stop
// /GX EH-state wall (docs/patterns/eh-dtor-model-members-as-destructible.md,
// topic:eh/topic:wall): EmptyVoiceList + the member ~CObList sequence and the
// frame are byte-exact; the residual is the trylevel-slot threading the /GX
// state machine emits. Logic complete; deferred to the final sweep.
RVA(0x00099ca0, 0x49)
CSpawnEntry::~CSpawnEntry() {
    EmptyVoiceList();
}

// ===========================================================================
// CSpawnEntry::EmptyVoiceList  (0x9a450)
// ===========================================================================
// Walk the CObList node chain (head @ m_list+0x4, each CNode = {next,prev,data});
// `delete (CVoiceSound*)node->data` on each held element (~CString + the engine
// operator delete = RezFree), then m_list.RemoveAll(). No destructible local, so
// no /GX frame even under eh flags.
RVA(0x0009a450, 0x36)
void CSpawnEntry::EmptyVoiceList() {
    struct CNode {
        CNode* m_next; // +0x00
        void* m_prev;  // +0x04
        void* m_data;  // +0x08
    };
    struct Layout {
        void* m_vptr;  // +0x00
        CNode* m_head; // +0x04  CObList::m_pNodeHead
    };
    CNode* node = ((Layout*)&m_list)->m_head;
    while (node != 0) {
        CNode* cur = node;
        node = node->m_next;
        CVoiceSound* v = (CVoiceSound*)cur->m_data;
        if (v != 0) {
            delete v;
        }
    }
    m_list.RemoveAll();
}

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
        CSpawnEntry* e = (CSpawnEntry*)m_18[i];
        if (e != 0) {
            e->~CSpawnEntry();
            RezFree(e);
        }
    }
    m_18.SetSize(0, -1);
    if (m_04 != 0 && ((CSpawnTree*)m_04)->m_20 != 0) {
        void** p = &m_10;
        for (i32 k = 0; k < 2; k++) {
            if (p[0] != 0) {
                ((CSpawnTree*)m_04)->m_20->Remove(p[0]);
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
// Byte-exact apart from the 3 reloc-masked operands (CreateSprite, s_GruntVoice,
// ClearSprites) - the reloc-mask plateau, i.e. matched (95.3% is the entropy tail).
RVA(0x0011af00, 0x62)
BOOL CGruntSpawnConfig::LoadGruntVoices() {
    ClearSprites();
    void** slot = &m_08;
    for (i32 i = 0; i < 2; i++, slot++) {
        CSpriteHandle* spr = ((CSpawnSpriteSource*)m_04)
                                 ->m_08->CreateSprite(0, 0, 0, 0xdbba1, s_GruntVoice, 0x4040003);
        spr->m_7c->Activate(spr);
        void* got = spr->m_7c->m_18;
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
// CSpawnEntry::AddVoiceSound  (0x11c560)
// ===========================================================================
// Allocate a voice-sound node (operator new 0xc), construct it from the (by-value)
// name CString, and append it to the list (AddTail). The trailing `flag` arg is
// unused. The by-value CString param + the inner copy construct the /GX frame.
//
// @early-stop
// /GX frame-size wall (docs/patterns/gx-scoped-local-eh-frame-size.md, topic:eh):
// instruction selection is byte-identical, but retail reserves the CString temp
// in ONE dword (`push ecx`) while the recompile reserves two (`sub esp,8`) - the
// by-value param slot is reused as the temp in retail, allocated fresh here - so
// every [esp+N] is shifted +4. ~92.5%; not source-steerable. Logic complete.
RVA(0x0011c560, 0x91)
void CSpawnEntry::AddVoiceSound(CString s, i32 flag) {
    (void)flag;
    CVoiceSound* node = new CVoiceSound(s);
    if (node != 0) {
        m_list.AddTail((CObject*)node);
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
    void** p = &m_08;
    for (i32 k = 0; k < 2; k++) {
        if (p[2] != 0) {
            ((CSpriteReleasable*)((char*)p[2] + 0x6c))->Release();
        }
        if (p[0] != 0) {
            ((CSpawnVoice*)p[0])->Reset();
        }
        p++;
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
