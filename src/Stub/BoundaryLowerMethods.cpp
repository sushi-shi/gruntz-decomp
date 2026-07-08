// BoundaryLowerMethods.cpp - small leaf methods recovered from the engine_boundary
// backlog (lower half, RVA < 0x133370). RTTI cannot attribute these COMDAT-folded
// methods, so the owning class names here are placeholders; only the OFFSETS +
// code bytes are load-bearing. Unmodeled engine callees/globals are declared
// NO-body so their rel32/DIR32 operands reloc-mask. Defined in retail-RVA order.
// The per-use owner/referent views now live in <Gruntz/BoundaryLowerMethodsViews.h>
// (pure code motion); the archive object folds to the canonical CSerialArchive.
#include <Gruntz/GruntzMgr.h> // canonical CGruntzMgr (MFC side; umbrella-first) - the
#include <Bute/ButeTree.h>
#include <DDrawMgr/DDrawSubMgrPages.h>
#include <Gruntz/Brickz.h>
#include <Gruntz/BattlezMapConfig.h>
// 0x8e880/0x915d0/0x91620 owners + CGruntzSoundZ
#include <Ints.h>
#include <rva.h>
#include <Gruntz/GameRegistry.h>              // canonical game-manager singleton (0x24556c) view
#include <Gruntz/BoundaryLowerMethodsViews.h> // owner/referent views for this TU
#include <Globals.h>

// ===========================================================================
// 0x0213a0 - virtual-base field getter: read the field at +0x04 of the virtual
// base whose displacement lives in the vbtable's second slot. __thiscall.
// ===========================================================================
RVA(0x000213a0, 0xa)
i32 C213a0::Get() {
    i32* vb = *(i32**)this;
    i32 disp = vb[1];
    return *(i32*)((char*)this + disp + 4);
}

// ===========================================================================
// 0x0464e0 - type-id -> entry resolver (the projectile/act fast-range + Find +
// grow-on-miss lookup; same shape as TypeKeyColl's TypeResolve). __thiscall(key).
// ===========================================================================
extern void* g_projActCache;      // 0x6bf464 (pinned in CStaticHazard.cpp)
extern void* g_retAddrBreadcrumb; // 0x6bf428 (pinned in CVoiceTrigger.cpp)
extern void* GetRetAddr();        // 0x16d990
// @early-stop
// esi/edi regalloc wall: cl assigns this->esi, key->edi; retail swaps (key->esi,
// this->edi). Full fast-range/Find/grow logic + offsets byte-faithful (same shape as
// TypeKeyColl::TypeResolve); the esi/edi assignment is not source-steerable.
RVA(0x000464e0, 0x74)
void* CTypeColl464::Resolve(i32 key) {
    m_20 = 0;
    if (key >= m_lo && key <= m_hi) {
        return m_buf + (key - m_lo) * m_stride;
    }
    if (Find(key, 0)) {
        return m_buf + (key - m_lo) * m_stride;
    }
    void* item = g_projActCache;
    g_retAddrBreadcrumb = GetRetAddr();
    m_4->Set(this, (i32)item, 0xc);
    return (void*)m_buf2;
}

// ===========================================================================
// 0x050ca0 - dispatch then reset the +0x1a0/+0x1a4 pair. __thiscall(arg).
// ===========================================================================
RVA(0x00050ca0, 0x2b)
void C50ca0::M(i32 arg) {
    Method(arg, 0, 0, 0);
    m_1a0 = -1;
    m_1a4 = 0;
}

// ===========================================================================
// 0x077dc0 - cell setter: m_20[ m_24[idx] + base ] = value. __thiscall(3 args).
// ===========================================================================
RVA(0x00077dc0, 0x1d)
void C77dc0::Set(i32 base, i32 idx, i32 value) {
    m_20[m_24[idx] + base] = value;
}

// ===========================================================================
// 0x08e880 - debug command hook: if the +0x2c sub-object's state slot (vtbl +0x10)
// reports 3, register the DEBUG_SETSKILL command. __thiscall, returns 0.
// ===========================================================================
extern void Lab401947(); // 0x401947 (code address passed as a ptr; reloc-masked)
RVA(0x0008e880, 0x27)
i32 CGruntzMgr::RegisterSetSkillDebugCmd() {
    if (m_curState->Update() == GAMESTATE_PLAY) {
        RegisterDebugCommand("DEBUG_SETSKILL", (void*)&Lab401947, 1);
    }
    return 0;
}

// ===========================================================================
// 0x0915d0 / 0x091620 - guarded dispatch: when +0x48 and +0x14 are live and the
// +0x48 sub's +0x1c probe (0x138f60) succeeds, hand (const, arg) to its handler
// (0x138fd0). __thiscall(arg). The two differ only in the constant (0 vs 0x64).
// ===========================================================================
RVA(0x000915d0, 0x3f)
void CGruntzMgr::MuteMusicIfActive(i32 ms) {
    if (m_sound == 0) {
        return;
    }
    if (m_musicEnabled == 0) {
        return;
    }
    i32 ok;
    if (m_sound->m_pCurrent != 0) {
        ok = m_sound->m_pCurrent->IsBusy();
    } else {
        ok = 0;
    }
    if (ok == 0) {
        return;
    }
    if (m_sound->m_pCurrent == 0) {
        return;
    }
    m_sound->m_pCurrent->SetVolume(0, ms);
}
RVA(0x00091620, 0x3f)
void CGruntzMgr::RestoreMusicVolumeIfActive(i32 ms) {
    if (m_sound == 0) {
        return;
    }
    if (m_musicEnabled == 0) {
        return;
    }
    i32 ok;
    if (m_sound->m_pCurrent != 0) {
        ok = m_sound->m_pCurrent->IsBusy();
    } else {
        ok = 0;
    }
    if (ok == 0) {
        return;
    }
    if (m_sound->m_pCurrent == 0) {
        return;
    }
    m_sound->m_pCurrent->SetVolume(0x64, ms);
}

// (0x099ba0 C99ba0::Ctor re-homed to src/Gruntz/AreaMgr.cpp as the real
// CAreaMgr::CAreaMgr - C99ba0 was a view of CAreaMgr {index, CSpawnList@+4};
// the 0x1b4867 sub Init is the embedded CObList block-size-10 ctor.)

// (0x09a420 C9a420::Clear re-homed to src/Gruntz/AreaMgr.cpp as the real
// CSpawnList::ClearFlags - the walked chain is the CSpawnList node list and the
// cleared back-pointer word is CSpawnEntry::m_flag. See <Gruntz/SpawnList.h>
// for the six-view unification proof.)

// ===========================================================================
// 0x09cab0 - out-param wrapper: call the +0x10 sub's method (0x1b8008) with a
// zeroed local and return the filled local. __thiscall(arg).
// ===========================================================================
RVA(0x0009cab0, 0x23)
i32 C9cab0::M(i32 arg) {
    i32 local = 0;
    ((CMapStringToPtr*)&m_10)->Lookup((const char*)arg, (void*&)local);
    return local;
}

// ===========================================================================
// 0x0b4c40 - dispatch a 4-arg action (0x3035); on success, when arg2 == 8, arm the
// +0x10 sub-object (+0x58 = 1, +0x50 = arg2, +0x54 = 0x80). __thiscall, ret 0x10.
// ===========================================================================
RVA(0x000b4c40, 0x4b)
i32 C0b4c40::Handle(i32 a1, i32 a2, i32 a3, i32 a4) {
    if (!Dispatch3035(a1, a2, a3, a4)) {
        return 0;
    }
    if (a2 == 8) {
        CSubB4* s = m_10;
        s->m_58 = 1;
        s->m_50 = a2;
        s->m_54 = 0x80;
    }
    return 1;
}

// ===========================================================================
// 0x0bd450 - init: run the base ctor (0x3625) then open the "c:\gruntz.log" log
// (0x1983). __thiscall.
// ===========================================================================
RVA(0x000bd450, 0x16)
void Cbd450::Init() {
    Base3625();
    OpenLog1983("c:\\gruntz.log");
}

// ===========================================================================
// 0x0cef50 - teardown: flush the +0x04 owner's +0xc8 sub (0x1b9c69); when +0x1c0
// is live, run the +0x0c->+0x04 close (0x158d20 -> 0x158e40) and dispatch +0x04's
// 0x201d(3). __thiscall, returns 1.
// ===========================================================================
RVA(0x000cef50, 0x46)
i32 Ccef50::M() {
    ((CObList*)(m_4 + 0xc8))->~CObList();
    if (m_1c0 != 0) {
        if (((CDDrawSubMgrPages*)m_c->m_4)->Method_158d20() != 0) {
            ((CDDrawSubMgrPages*)m_c->m_4)->Method_158e40();
        }
        ((CGruntzMgr*)m_4)->ChangeState_8fab0(3);
    }
    return 1;
}

// ===========================================================================
// 0x0d5e20 - forward an arg through two virtuals (vtbl +0x3c then +0x40).
// __thiscall(arg).
// ===========================================================================
RVA(0x000d5e20, 0x1b)
void Cd5e20::M(void* arg) {
    v15(arg);
    v16(arg);
}

// ===========================================================================
// 0x0db200 - swap the +0x08 holder to `arg`: no-op when already equal, else
// validate (0x11f9), toggle old off / new on (0x3bbb) and store. __thiscall.
// ===========================================================================
extern "C" i32 Check11f9(void* p);          // 0x11f9
extern "C" void Toggle3bbb(void* p, i32 f); // 0x3bbb
RVA(0x000db200, 0x51)
i32 Cdb200::M(void* arg) {
    if (m_8 == arg) {
        return 1;
    }
    if (Check11f9(arg)) {
        Toggle3bbb(m_8, 1);
        Toggle3bbb(arg, 0);
        m_8 = arg;
        return 1;
    }
    return 0;
}

// ===========================================================================
// 0x0db2f0 - finalize: when +0x20 is live, run the +0x38 teardown (0x40c5) iff
// +0x14 is clear, then reset +0x20. __thiscall, returns 1/0.
// ===========================================================================
RVA(0x000db2f0, 0x2b)
i32 Cdb2f0::M() {
    if (m_20 == 0) {
        return 0;
    }
    if (m_14 == 0) {
        ((CBattlezMapConfig*)&m_38)->Clear_02ade0();
    }
    m_20 = 0;
    return 1;
}

// ===========================================================================
// 0x0db750 - "LEVEL" config sync: on first call (arg==0) probe the +0x0c owner's
// +0x2c config for "LEVEL" (0x152c50); set it (0x1527d0), then resolve the +0x28
// parser entry (0x13bae0) and, if found, bind it back (0x152ad0). __thiscall.
// ===========================================================================
RVA(0x000db750, 0x70)
i32 Cdb750::M(void* arg) {
    if (m_c == 0) {
        return 0;
    }
    if (arg == 0) {
        if (m_c->m_2c->HasKeyPrefix_152c50("LEVEL") != 0) {
            return 1;
        }
    }
    m_c->m_2c->RemoveKeysEqual_1527d0("LEVEL", &g_dat60b588);
    void* e = (void*)m_28->ResolvePath(&g_dat613054);
    if (e == 0) {
        return 0;
    }
    m_c->m_2c->ScanTree_152ad0(e, "LEVEL", &g_dat60b588);
    return 1;
}

// ===========================================================================
// 0x0ea170 - 2-bit selector over a +0x38 virtual: pick one of four fixed arg
// tuples by (arg1!=0, arg2!=0). __thiscall(arg1, arg2).
// ===========================================================================
RVA(0x000ea170, 0x5c)
void Cea170::M(i32 a1, i32 a2) {
    if (a1 == 0) {
        if (a2 == 0) {
            Dispatch(1, -1, 0, 0, -1);
        } else {
            Dispatch(-1, -1, -1, 0, -1);
        }
    } else {
        if (a2 == 0) {
            Dispatch(4, -1, 0, 0, -1);
        } else {
            Dispatch(-1, -1, 1, 0, -1);
        }
    }
}

// ===========================================================================
// 0x0eb970 - Serialize: when the manager is live, transfer +0x3c via arg1's
// vtbl +0x30 (mode 4) or +0x2c (mode 7), then chain the base serializer (0x3ca1)
// normalized to a bool. __thiscall(ar, mode, a3, a4). ret 0x10.
// ===========================================================================
extern "C" CGameRegistry* g_mgrSettings;
// @early-stop
// block-layout wall: the mode==4 Write branch lands inline (jne-skip) but retail
// floats it to the tail (forward je). All transfers, the base-chain call and the
// neg/sbb/neg bool are byte-faithful.
RVA(0x000eb970, 0x72)
i32 Ceb970::Serialize(CSerialArchive* ar, i32 mode, i32 a3, i32 a4) {
    if (ar == 0) {
        return 0;
    }
    if (g_mgrSettings->m_world == 0) {
        return 0;
    }
    if (mode == 4) {
        ar->Write(&m_3c, 4);
    } else if (mode == 7) {
        ar->Read(&m_3c, 4);
    }
    return Base3ca1(ar, mode, a3, a4) != 0;
}

// ===========================================================================
// 0x0fa150 - release the four owned blits (+0x160/+0x164/+0x14/+0x18) through the
// +0x0c owner's +0x1c allocator (0x142160), then clear +0x3c. __thiscall.
// ===========================================================================
// @early-stop
// cmp-operand-order wall: retail emits cmp val,edi (val vs the zeroed edi); cl emits
// cmp edi,val. Same semantics, 1 byte per guard. All four frees + offsets byte-faithful.
RVA(0x000fa150, 0x74)
void Cfa150::Cleanup() {
    if (m_c != 0) {
        if (m_160 != 0) {
            m_c->m_1c->Free(m_160);
            m_160 = 0;
        }
        if (m_164 != 0) {
            m_c->m_1c->Free(m_164);
            m_164 = 0;
        }
        if (m_14 != 0) {
            m_c->m_1c->Free(m_14);
            m_14 = 0;
        }
        if (m_18 != 0) {
            m_c->m_1c->Free(m_18);
            m_18 = 0;
        }
    }
    m_3c = 0;
}

// ===========================================================================
// 0x104c80 - release the +0x34 blit through the +0x24 owner's +0x1c allocator
// (0x142160) and clear it. __thiscall.
// ===========================================================================
RVA(0x00104c80, 0x1f)
void C104c80::Free() {
    if (m_34 != 0) {
        m_24->m_1c->Free(m_34);
        m_34 = 0;
    }
}

// ===========================================================================
// 0x104dd0 - lazy-create the StatusBarSprite: clamp +0x24/+0x28 to the manager's
// screen bounds, then build it via the +0x0c factory (0x1597b0). __thiscall.
// ===========================================================================
// @early-stop
// scheduling wall: retail computes m_8c-0x22 via lea eax,[ecx-0x22] and loads m_x
// late; cl uses sub + an earlier m_x load. Clamp logic, the factory call and the
// StatusBarSprite literal are byte-faithful.
RVA(0x00104dd0, 0x6b)
i32 C104dd0::Create() {
    if (m_sprite != 0) {
        return 0;
    }
    i32 a = g_mgrSettings->m_modeW - 0x22;
    i32 d = g_mgrSettings->m_modeH;
    if (m_x > a) {
        m_x = a;
    }
    if (m_y > d - 9) {
        m_y = d - 0x22;
    }
    m_sprite = m_factoryHolder->m_8->CreateSprite(0, m_x, m_y, 0xf4240, "StatusBarSprite", 1);
    return m_sprite != 0;
}

// ===========================================================================
// 0x10bbe0 - getter: return +0x4cc when +0x528 is clear; else the active cell
// (+0x534[+0x52c]) when the +0x538 count exceeds the index, else 0. __thiscall.
// ===========================================================================
RVA(0x0010bbe0, 0x34)
i32 C10bbe0::M() {
    if (m_528 == 0) {
        return m_fallback;
    }
    if (m_count > 0 && m_count > m_index) {
        return *m_entries[m_index];
    }
    return 0;
}

// ===========================================================================
// 0x112bf0 - decrement the active grid cell (manager-owned plane) and re-publish
// it through the manager's +0x70 notifier (0x33f0). __thiscall, returns 1.
// ===========================================================================
// @early-stop
// strength-reduction wall: cl materializes m_row<<2 (shl ecx,2) and reuses scale-1
// addressing; retail keeps m_row in ecx and uses *4 scaled addressing in both cell
// stores. Logic/offsets byte-faithful; the shift vs scaled-index pick is not steerable.
RVA(0x00112bf0, 0x5e)
i32 C112bf0::M() {
    CGameRegistry* mg = g_mgrSettings;
    CGridData* g = ((CGridOuter*)mg->m_world)->m_24->m_5c;
    i32 v = g->cells[g->rows[m_row] + m_col] - 1;
    CGridData* g2 = ((CGridOuter*)mg->m_world)->m_24->m_5c;
    g2->cells[g2->rows[m_row] + m_col] = v;
    ((CBrickzGrid*)mg->m_tileGrid)->ComputeCellFlags(m_col, m_row, v);
    m_14 = 0;
    return 1;
}

// ===========================================================================
// 0x113860 - mode gate: validate `obj` against mode (4 -> 0x4499, 7 -> 0x1893),
// passing through otherwise. __stdcall(obj, mode, a3, a4) ret 0x10.
// ===========================================================================
extern i32 __stdcall Func1893(void* p); // 0x1893
extern i32 __stdcall Func4499(void* p); // 0x4499
// @early-stop
// regalloc wall: retail keeps obj in eax (so the obj==0 return 0 is free); cl pins
// it in ecx and adds xor eax. switch(mode) recovers the case layout (93%); the eax
// vs ecx pick is not steerable.
RVA(0x00113860, 0x3b)
i32 __stdcall Gate113860(void* obj, i32 mode, i32 a3, i32 a4) {
    if (obj == 0) {
        return 0;
    }
    switch (mode) {
        case 4:
            if (!Func4499(obj)) {
                return 0;
            }
            break;
        case 7:
            if (!Func1893(obj)) {
                return 0;
            }
            break;
    }
    return 1;
}

// ===========================================================================
// 0x113e70 - Serialize: when the manager is live, transfer +0xc0/+0xc4 and the
// nine +0x9c.. dwords (3x3) through arg1's vtbl +0x2c. __thiscall(ar) ret 4.
// ===========================================================================
// @early-stop
// esi/edi regalloc wall: cl assigns ar->esi, this->edi; retail swaps (this->esi,
// ar->edi). The two header transfers + the 3x3 nested-loop transfer are byte-faithful.
RVA(0x00113e70, 0x7b)
i32 C113e70::Serialize(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }
    if (g_mgrSettings->m_world == 0) {
        return 0;
    }
    ar->Read(&m_c0, 4);
    ar->Read(&m_c4, 4);
    i32* p = m_9c;
    i32 i = 3;
    do {
        i32 j = 3;
        do {
            ar->Read(p, 4);
            p++;
        } while (--j);
    } while (--i);
    return 1;
}

// ===========================================================================
// 0x114ec0 - straight 6-arg forwarder to 0x21c1. __cdecl.
// ===========================================================================
extern "C" void Func21c1(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x21c1
RVA(0x00114ec0, 0x27)
void Fwd114ec0(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    Func21c1(a1, a2, a3, a4, a5, a6);
}

// ===========================================================================
// 0x114f00 - guarded forwarder: resolve a2->m_30->m_4->m_10->m_2c and, when live,
// forward it plus the six args to 0x267b. __cdecl(6 args).
// ===========================================================================
extern "C" void Func267b(void* v, i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6); // 0x267b
// @early-stop
// identical-return-epilogue tail-merge wall (docs/patterns): cl shares one pop;ret
// tail across the two null guards; retail emits the inline ret at each site. Deref
// chain + 6-arg re-push forward are byte-faithful.
RVA(0x00114f00, 0x3e)
void Fwd114f00(i32 a1, i32 a2, i32 a3, i32 a4, i32 a5, i32 a6) {
    CObj114f* obj = ((CArg114f*)a2)->m_30->m_4->m_10;
    if (obj == 0) {
        return;
    }
    if (obj->m_2c == 0) {
        return;
    }
    Func267b(obj->m_2c, a1, a2, a3, a4, a5, a6);
}

// ===========================================================================
// 0x1181d0 - bounds-grow: reject when the new (+0x04,+0x08) pair does not exceed
// the +0xb8 box; else store it, notify (0x3661) and stash +0xd4. __thiscall(3).
// ===========================================================================
extern "C" void Func3661(CBox118* p); // 0x3661
RVA(0x001181d0, 0x70)
i32 C1181d0::Update(i32 a1, i32 a2, i32 a3) {
    if (a1 == 0) {
        return 0;
    }
    if (a2 == 0) {
        return 0;
    }
    CBox118* b = &m_bounds;
    if (b == 0) {
        return 0;
    }
    if (b->m_4 > a1) {
        return 0;
    }
    if (b->m_4 == a1 && b->m_8 < a2) {
        return 0;
    }
    b->m_4 = a1;
    b->m_8 = a2;
    Func3661(b);
    m_d4 = a3;
    return 1;
}

// ===========================================================================
// 0x118260 - copy-if-grow: reject when the source box does not exceed +0xb8;
// else copy the 7-dword box in and stash +0xd4. __thiscall(src, arg2) ret 8.
// ===========================================================================
RVA(0x00118260, 0x63)
i32 C118260::Update(CRect118* src, i32 arg2) {
    if (src == 0) {
        return 0;
    }
    CRect118* dst = &m_bounds;
    if (dst == 0) {
        return 0;
    }
    if (dst->m_4 > src->m_4) {
        return 0;
    }
    if (dst->m_4 == src->m_4 && dst->m_8 < src->m_8) {
        return 0;
    }
    *dst = *src;
    m_d4 = arg2;
    return 1;
}
