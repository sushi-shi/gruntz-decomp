// GruntToySprite.cpp - the "grunt has a toy" indicator sprite (C:\Proj\Gruntz).
// A CUserLogic-derived game object; methods in ascending retail-RVA order:
//   ~CGruntToySprite  @0x0122b0 - the /GX leaf dtor (CUserLogic teardown).
//   SetCell           @0x07f920 - stash the (x,y) grunt cell, clear m_38 bit 0.
//   Update            @0x07f960 - track the grunt's screen pos + layer.
//
// The 0x44 is a DESTRUCTOR (stamps CUserLogic 0x5e705c then CUserBase 0x5e70b4,
// tears down the +0x18 link via ~EngStr @0x16d2a0), NOT a ctor - identical in
// shape to ~CTimeBomb @0x012a70.
#include <Gruntz/Sprite.h> // CSprite - the bound object's +0x194 cached sprite (ex CGruntLayerHolder)
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/GameRegPtr.h>
#include <Io/FileMem.h>          // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/Grunt.h> // CGrunt - the registry grunt-table slot (was the CGruntEntry view)
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)

// DATA-bind the class registry singleton in the main_file .cpp (labels.py scans
// DATA() only in TU source, not headers).
DATA(0x00244d58)
CIndicatorActReg g_toyActReg; // 0x644d58

// ~CGruntToySprite @0x0122b0 - the CUserLogic-folded /GX leaf dtor (see header).
// IMPLICIT dtor (retail is COMPILER-GENERATED - eh-dtor-vptr-restamp CAUSE B):
// a user-declared `~CGruntToySprite() {}` emits the leaf-vptr restamp, and the CWapX
// base EH state blocks the dead-store elision that used to hide it. The ??_G
// in the vtable-emitting TU forces the implicit ??1 COMDAT; pinned by name.
// @rva-symbol: ??1CGruntToySprite@@UAE@XZ 0x000122b0 0x44

// --- CGruntToySprite (0x07f350), vptr 0x5e7b4c --- the ctor anchors the
// ??_7CGruntToySprite vtable in this TU. Folds the inline CUserLogic(obj) base.
RVA(0x0007f350, 0x16a)
CGruntToySprite::CGruntToySprite(CGameObject* obj) : CUserLogic(obj), CWapX(obj) {
    m_38->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALL", 0);
    m_prevAnimSetNode = m_objAux->m_1c;
    m_objAux->m_1c = g_buteTree.Find("A");
    m_38->m_stateFlags |= 1;
    if (m_object->m_latchedAnimId != 0xdbba0) {
        m_object->m_latchedAnimId = 0xdbba0;
        m_object->m_flags |= 0x20000;
    }
    m_lastLayer = 0;
}

// CGruntToySprite::InitActReg @0x07f540 - construct the class's activation-
// coordinate registry singleton (g_toyActReg @0x644d58) over [2000, 2010] via
// the shared registry ctor (FUN_00408710). Free init thunk; reloc-masked.
RVA(0x0007f540, 0x15)
void CGruntToySprite::InitActReg() {
    (reinterpret_cast<CZDArrayDerived*>(&g_toyActReg))->Construct(2000, 2010);
}

// CGruntToySprite::RunAct @0x07f5c0 - resolve the coordinate-registry entry for `id`
// (inline CActReg::ResolveEntry) and, if it holds a registered handler PMF, re-resolve
// the entry and dispatch the PMF on `this`. Two inline ResolveEntry expansions because
// it has side effects and cl cannot CSE it across the guard. See RunAct notes elsewhere.
RVA(0x0007f5c0, 0x102)
void CGruntToySprite::FireActivation(i32 id) {
    if ((reinterpret_cast<CToyActEntry*>(g_toyActReg.ResolveEntry(id)))->m_fn != 0) {
        (this->*(reinterpret_cast<CToyActEntry*>(g_toyActReg.ResolveEntry(id)))->m_fn)();
    }
}

// CGruntToySprite::RegisterActs @0x07f720 - bind the class's per-frame handler
// (Update @0x07f960) to the activation key "A" (the SAME activation-name-intern
// archetype as CGruntHealthSprite::RegisterActs; see that TU for the full notes).
//
// @early-stop
// register-pinning wall (docs/patterns/zero-register-pinning.md +
// test-old-value-decrement-loop-while-postdec.md, topic:wall topic:regalloc): logic
// byte-faithful (every call/immediate/branch/offset + the `mov [entry],offset
// Update` handler store match retail); residual is the slot-vs-id callee-saved
// register choice cascading into the free-loop count materialization. Deferred.
RVA(0x0007f720, 0x18d)
void CGruntToySprite::RegisterActs() {
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
    (reinterpret_cast<CToyActEntry*>(g_toyActReg.ResolveEntry(id)))->m_fn = static_cast<i32 (CUserLogic::*)()>(&CGruntToySprite::Update);
}

// SetCell @0x07f920 - stash the (x,y) grunt cell, clear m_38 bit 0, return 1.
RVA(0x0007f920, 0x21)
i32 CGruntToySprite::SetCell(i32 x, i32 y) {
    m_cellX = x;
    m_cellY = y;
    m_38->m_stateFlags &= ~1;
    return 1;
}

// Update @0x07f960 - resolve the grunt for cell (m_cellX,m_cellY); when present, if its
// layer index changed re-clamp it through the level layer table into the bound
// renderable's layer fields, then copy the grunt's screen position (y biased by
// -0x20) into the bound renderable. Returns 0.
RVA(0x0007f960, 0x85)
i32 CGruntToySprite::Update() {
    CGrunt* e = (reinterpret_cast<CGrunt**>((reinterpret_cast<char*>(g_gameReg->m_cmdGrid) + 0x1c)))[m_cellX * 15 + m_cellY];
    if (e == 0) {
        return 0;
    }
    i32 layer = e->m_198;
    if (m_lastLayer != layer) {
        CGameObject* r = m_object;
        m_lastLayer = layer;
        CSprite* h = r->m_sprite;
        if (h != 0) {
            CImage* mapped;
            if (layer >= h->m_minIndex && layer <= h->m_maxIndex) {
                mapped = reinterpret_cast<CImage*>(h->m_items.GetAt(layer));
            } else {
                mapped = 0;
            }
            r->m_layer = mapped;
            r->m_190 = layer;
        }
    }
    m_object->m_screenX = e->m_object->m_screenX;
    m_object->m_screenY = e->m_object->m_screenY - 0x20;
    return 0;
}

// CGruntToySprite::Serialize @0x07fa20 - round-trip the own leaf state (m_cellX/m_cellY
// = 8 B, m_lastLayer = 4 B) per mode (4 = write @+0x30, 7 = read @+0x2c), then chain the
// base CUserLogic::SerializeMove (bail 0 on failure) and the +0x34 serialized-object-
// reference (CSerialObjRef::Chain via the 0x1aff thunk); return whether the ref chained.
RVA(0x0007fa20, 0x89)
i32 CGruntToySprite::SerializeMove(CGruntArchive* ar, i32 mode, i32 a3, i32 a4) {
    switch (mode) {
        case 4:
            ar->Write(&m_cellX, 8);
            ar->Write(&m_lastLayer, 4);
            break;
        case 7:
            ar->Read(&m_cellX, 8);
            ar->Read(&m_lastLayer, 4);
            break;
    }
    if (CUserLogic::SerializeMove(ar, mode, a3, a4) == 0) {
        return 0;
    }
    return Chain(ar, mode, a3, reinterpret_cast<CGameObject*>(a4)) != 0;
}
