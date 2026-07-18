// GruntSelectedSprite.cpp - the "grunt is selected" indicator sprite
// (C:\Proj\Gruntz). A CUserLogic-derived game object; methods in ascending
// retail-RVA order:
//   ~CGruntSelectedSprite  @0x011e80 - the /GX leaf dtor (CUserLogic teardown).
//   SetCell                @0x07e9c0 - stash the (x,y) grunt cell, return 1.
//   Update                 @0x07e9f0 - track the selected grunt's screen pos.
//
// The 0x44 is a DESTRUCTOR (it stamps the CUserLogic 0x5e705c then CUserBase
// 0x5e70b4 vptr and tears down the +0x18 link via ~EngStr @0x16d2a0), NOT a
// ctor - identical in shape to ~CTimeBomb @0x012a70 / ~CInGameIcon @0x011d00.
#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GameRegPtr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/Grunt.h> // CGrunt - the registry grunt-table slot (was the CGruntEntry view)
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)

// DATA-bind the class registry singleton in the main_file .cpp (labels.py scans
// DATA() only in TU source, not headers).
DATA(0x00244da8)
CIndicatorActReg g_selectedActReg; // 0x644da8

// ~CGruntSelectedSprite @0x011e80 - the leaf adds no destructible members beyond
// CUserLogic, so its dtor folds the bare CUserLogic teardown: store the
// CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link (the embedded
// ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The destructible
// link forces the /GX EH frame; the empty body is enough for cl.
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntSelectedSprite() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CGruntSelectedSprite@@UAE@XZ 0x00011e80 0x44

// --- CGruntSelectedSprite (0x07e3e0), vptr 0x5e7bfc --- the ctor anchors the
// ??_7CGruntSelectedSprite vtable in this TU. Folds the inline CUserLogic(obj) base
// + the sprite name/geometry tail.
RVA(0x0007e3e0, 0x178)
CGruntSelectedSprite::CGruntSelectedSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("GAME_GRUNTSELECTEDSPRITE");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_GRUNTSELECTEDSPRITE", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_object->m_latchedAnimId != 0x14) {
        m_object->m_latchedAnimId = 0x14;
        m_object->m_flags |= 0x20000;
    }
}

// CGruntSelectedSprite::InitActReg @0x07e5e0 - construct the class's activation-
// coordinate registry singleton (g_selectedActReg @0x644da8) over [2000, 2010]
// via the shared registry ctor (FUN_00408710). Free init thunk; reloc-masked.
RVA(0x0007e5e0, 0x15)
void CGruntSelectedSprite::InitActReg() {
    ((CZDArrayDerived*)&g_selectedActReg)->Construct(2000, 2010);
}

// CGruntSelectedSprite::RunAct @0x07e660 - resolve the coordinate-registry entry for
// `id` (the inline CActReg::ResolveEntry fast [lo,hi] range + slow Find/Insert rebuild),
// and if it holds a registered handler PMF, re-resolve the entry and dispatch the PMF on
// `this`. ResolveEntry has side effects (m_scratch=0, may grow) so cl re-evaluates it for
// the guarded call rather than CSE-ing - hence the two inline expansions.
RVA(0x0007e660, 0x102)
void CGruntSelectedSprite::FireActivation(i32 id) {
    if (((CSelectedActEntry*)g_selectedActReg.ResolveEntry(id))->m_fn != 0) {
        (this->*((CSelectedActEntry*)g_selectedActReg.ResolveEntry(id))->m_fn)();
    }
}

// CGruntSelectedSprite::RegisterActs @0x07e7c0 - bind the class's per-frame handler
// (Update @0x07e9f0) to the activation key "A" (the SAME activation-name-intern
// archetype as CGruntHealthSprite::RegisterActs; see that TU for the full notes).
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// Update` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0007e7c0, 0x18d)
void CGruntSelectedSprite::RegisterActs() {
    i32 id = reinterpret_cast<i32>(g_buteTree.Find("A"));
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert("A", (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = (void**)g_typeColl.m_alloc;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=("A");
        g_typeCounter++;
    }
    ((CSelectedActEntry*)g_selectedActReg.ResolveEntry(id))->m_fn = (i32 (CUserLogic::*)())&CGruntSelectedSprite::Update;
}

// SetCell @0x07e9c0 - stash the (x,y) grunt cell, return 1.
RVA(0x0007e9c0, 0x16)
i32 CGruntSelectedSprite::SetCell(i32 x, i32 y) {
    m_cellX = x;
    m_cellY = y;
    return 1;
}

// Update @0x07e9f0 - resolve the grunt for cell (m_cellX,m_cellY) from the registry's
// grunt table; if that grunt is drawn (entry->m_drawn), sync the +0x38 object's
// helper and copy the grunt's screen position into the bound renderable so the
// "selected" ring tracks the grunt. Returns 0.
//
// @early-stop
// regalloc/scheduling wall (zero-register-pinning class): the logic is byte-exact
// but cl pins g_gameReg in a different register than retail (ecx vs edx) and emits
// the reg->m_68 load before the index lea-chain where retail defers it - the
// `m_drawn` second condition shifts the register pressure so the deref ordering is
// not source-steerable (the sibling Toy::Update, no m_1d8 check, reaches 99.3%).
// Every instruction matches modulo register names. Deferred to the final sweep.
// (wave5-R7: this zero-reg-pin wall is ultra-TU-sensitive; SerializeMove's byte-neutral
// chain rebinding to CMovingLogicBase::Serialize nudged its regalloc 99.2->84.8%. The
// LOGIC is unchanged - only more register-name divergence in the same wall.)
RVA(0x0007e9f0, 0x5f)
i32 CGruntSelectedSprite::Update() {
    CGameRegistry* reg = g_gameReg;
    CGrunt* e = ((CGrunt**)((char*)reg->m_cmdGrid + 0x1c))[m_cellX * 15 + m_cellY];
    if (e != 0 && e->m_arrived != 0) {
        m_38->m_1a0.Advance(g_engineFrameDelta);
        m_object->m_screenX = e->m_object->m_screenX;
        m_object->m_screenY = e->m_object->m_screenY;
    }
    return 0;
}

// SerializeMove @0x07ea70 (vtable slot 1) - round-trip the {m_cellX,m_cellY} 8-byte
// grunt-cell pair through the archive stream (mode 4 = Write @+0x30, mode 7 = Read
// @+0x2c), then chain the shared CUserLogic serialize helper (SerializeMove, 0x16e7f0)
// and the +0x34 CSerialObjRef sub-object's Chain (0x8c00). The two-chain archetype
// (CTimeBomb::SerializeMove / CGruntPuddle::Serialize).
RVA(0x0007ea70, 0x6f)
i32 CGruntSelectedSprite::SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) {
    CSerialArchive* sa = (CSerialArchive*)arc;
    // Retail lays the mode==4 Write block out-of-line (cmp 4; je) with the mode==7
    // Read inline; this (mode != 4 ? maybe-Read : Write) form reproduces that layout.
    if (mode != 4) {
        if (mode == 7) {
            sa->Read(&m_cellX, 8);
        }
    } else {
        sa->Write(&m_cellX, 8);
    }
    if (!CUserLogic::SerializeMove((CSerialArchive*)((i32)arc), mode, a3, a4)) {
        return 0;
    }
    return Chain(sa, mode, a3, (CGameObject*)a4) ? 1 : 0;
}
