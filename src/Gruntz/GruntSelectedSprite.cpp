#include <Gruntz/GruntSelectedSprite.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/AniAdvanceCursor.h>
#include <Gruntz/SerialArchive.h> // CSerialArchive (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Wap32/ZVec.h>
#include <Gruntz/Grunt.h> // CGrunt - the registry grunt-table slot (was the CGruntEntry view)
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)
#include <Gruntz/TriggerMgr.h> // CTriggerMgr - m_cmdGrid (its m_grid CGrunt cells)

DATA(0x00244da8)
extern CIndicatorActReg g_selectedActReg; // 0x644da8

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

RVA(0x0007e3e0, 0x178)
CGruntSelectedSprite::CGruntSelectedSprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyName("GAME_GRUNTSELECTEDSPRITE");
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("GAME_GRUNTSELECTEDSPRITE", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    if (m_object->m_sortKey != 0x14) {
        m_object->m_sortKey = 0x14;
        m_object->m_flags |= 0x20000;
    }
}

RVA(0x0007e5e0, 0x15)
void CGruntSelectedSprite::InitActReg() {
    g_selectedActReg.Construct(2000, 2010);
}

RVA(0x0007e660, 0x102)
void CGruntSelectedSprite::FireActivation(i32 id) {
    if ((reinterpret_cast<CSelectedActEntry*>(g_selectedActReg.ResolveEntry(id)))->m_fn != 0) {
        (this->*(reinterpret_cast<CSelectedActEntry*>(g_selectedActReg.ResolveEntry(id)))->m_fn)();
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
        g_buteTree.Insert("A", reinterpret_cast<void*>(id));
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = reinterpret_cast<void**>(g_typeColl.m_alloc);
        while (n-- != 0) {
            if (list != 0) {
                (reinterpret_cast<CString*>(list))->CString::~CString();
            }
            list++;
        }
        (reinterpret_cast<CString*>(slot))->operator=("A");
        g_typeCounter++;
    }
    (reinterpret_cast<CSelectedActEntry*>(g_selectedActReg.ResolveEntry(id)))->m_fn =
        static_cast<i32 (CUserLogic::*)()>(&CGruntSelectedSprite::Update);
}

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
    CGruntzMgr* reg = g_gameReg;
    CGrunt* e = reg->m_cmdGrid->m_grid[m_cellX * 15 + m_cellY];
    if (e != 0 && e->m_arrived != 0) {
        m_38->m_1a0.Advance(g_engineFrameDelta);
        m_object->m_screenX = e->m_object->m_screenX;
        m_object->m_screenY = e->m_object->m_screenY;
    }
    return 0;
}

RVA(0x0007ea70, 0x6f)
i32 CGruntSelectedSprite::SerializeMove(CGruntArchive* arc, i32 mode, i32 a3, i32 a4) {
    CSerialArchive* sa = static_cast<CSerialArchive*>(arc);
    // Retail lays the mode==4 Write block out-of-line (cmp 4; je) with the mode==7
    // Read inline; this (mode != 4 ? maybe-Read : Write) form reproduces that layout.
    if (mode != 4) {
        if (mode == 7) {
            sa->Read(&m_cellX, 8);
        }
    } else {
        sa->Write(&m_cellX, 8);
    }
    if (!CUserLogic::SerializeMove(
            reinterpret_cast<CSerialArchive*>((reinterpret_cast<i32>(arc))),
            mode,
            a3,
            a4
        )) {
        return 0;
    }
    return Chain(sa, mode, a3, reinterpret_cast<CGameObject*>(a4)) ? 1 : 0;
}
