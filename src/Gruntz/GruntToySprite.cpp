// GruntToySprite.cpp - the "grunt has a toy" indicator sprite (C:\Proj\Gruntz).
// A CUserLogic-derived game object; methods in ascending retail-RVA order:
//   ~CGruntToySprite  @0x0122b0 - the /GX leaf dtor (CUserLogic teardown).
//   SetCell           @0x07f920 - stash the (x,y) grunt cell, clear m_38 bit 0.
//   Update            @0x07f960 - track the grunt's screen pos + layer.
//
// The 0x44 is a DESTRUCTOR (stamps CUserLogic 0x5e705c then CUserBase 0x5e70b4,
// tears down the +0x18 link via ~EngStr @0x16d2a0), NOT a ctor - identical in
// shape to ~CTimeBomb @0x012a70.
#include <Gruntz/GruntToySprite.h>
#include <Gruntz/SerialObjRef.h> // (moved from header; +0x34 serialized-object-ref, .cpp-only)
#include <Gruntz/MovingLogicBase.h> // CMovingLogicBase::Serialize (0x16e7f0) - shared serialize chain
#include <Wap32/ZVec.h>
#include <Wap32/ZDArrayDerived.h>
#include <Gruntz/TypeKeyColl.h> // the REAL registry class at 0x6bf650 (its fields were the shredded g_type* globals)
extern "C" CGameRegistry* g_gameReg; // *0x24556c singleton (view moved from header)

// DATA-bind the class registry singleton in the main_file .cpp (labels.py scans
// DATA() only in TU source, not headers).
DATA(0x00244d58)
CIndicatorActReg g_toyActReg; // 0x644d58

// ~CGruntToySprite @0x0122b0 - the CUserLogic-folded /GX leaf dtor (see header).
RVA(0x000122b0, 0x44)
CGruntToySprite::~CGruntToySprite() {}

// --- CGruntToySprite (0x07f350), vptr 0x5e7b4c --- the ctor anchors the
// ??_7CGruntToySprite vtable in this TU. Folds the inline CUserLogic(obj) base.
RVA(0x0007f350, 0x16a)
CGruntToySprite::CGruntToySprite(CGameObject* obj) : CUserLogic(obj) {
    TILE_LOGIC_SEED(obj);
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
    ((CZDArrayDerived*)&g_toyActReg)->Construct(2000, 2010);
}

// CGruntToySprite::RunAct @0x07f5c0 - resolve the coordinate-registry entry for `id`
// (inline CActReg::ResolveEntry) and, if it holds a registered handler PMF, re-resolve
// the entry and dispatch the PMF on `this`. Two inline ResolveEntry expansions because
// it has side effects and cl cannot CSE it across the guard. See RunAct notes elsewhere.
RVA(0x0007f5c0, 0x102)
void CGruntToySprite::RunAct(i32 id) {
    if (((CToyActEntry*)g_toyActReg.ResolveEntry(id))->m_fn != 0) {
        (this->*((CToyActEntry*)g_toyActReg.ResolveEntry(id))->m_fn)();
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
    i32 id = (i32)g_buteTree.Find(s_codeA);
    if (id == 0) {
        id = g_typeCounter;
        g_buteTree.Insert(s_codeA, (void*)id);
        char* slot = ActNameLookup(id);
        i32 n = g_typeColl.m_grown;
        void** list = (void**)g_typeColl.m_alloc;
        while (n-- != 0) {
            if (list != 0) {
                ((CString*)list)->CString::~CString();
            }
            list++;
        }
        ((CString*)slot)->operator=(s_codeA);
        g_typeCounter++;
    }
    ((CToyActEntry*)g_toyActReg.ResolveEntry(id))->m_fn = &CGruntToySprite::Update;
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
    CGruntEntry* e = ((CGruntEntry**)((char*)g_gameReg->m_cmdGrid + 0x1c))[m_cellX * 15 + m_cellY];
    if (e == 0) {
        return 0;
    }
    i32 layer = e->m_layerIndex;
    if (m_lastLayer != layer) {
        CGruntRenderable* r = (CGruntRenderable*)m_object;
        m_lastLayer = layer;
        CGruntLayerHolder* h = r->m_layerHolder;
        if (h != 0) {
            i32 mapped;
            if (layer >= h->m_layerLo && layer <= h->m_layerHi) {
                mapped = h->m_layerTable[layer];
            } else {
                mapped = 0;
            }
            r->m_mappedLayer = mapped;
            r->m_resolvedLayer = layer;
        }
    }
    m_object->m_screenX = e->m_renderable->m_screenX;
    m_object->m_screenY = e->m_renderable->m_screenY - 0x20;
    return 0;
}

// CGruntToySprite::Serialize @0x07fa20 - round-trip the own leaf state (m_cellX/m_cellY
// = 8 B, m_lastLayer = 4 B) per mode (4 = write @+0x30, 7 = read @+0x2c), then chain the
// base CUserLogic::SerializeChain (bail 0 on failure) and the +0x34 serialized-object-
// reference (CSerialObjRef::Chain via the 0x1aff thunk); return whether the ref chained.
RVA(0x0007fa20, 0x89)
i32 CGruntToySprite::Serialize(CSerialArchive* ar, i32 mode, i32 a3, i32 a4) {
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
    if (((CMovingLogicBase*)this)->Serialize((CSerialArchive*)((i32)ar), mode, a3, a4) == 0) {
        return 0;
    }
    return ((CSerialObjRef*)&m_34)->Chain(ar, mode, a3, (CSerialObj*)a4) != 0;
}
