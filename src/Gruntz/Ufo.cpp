// Ufo.cpp - the UFO path-hazard game-object (C:\Proj\Gruntz), a CPathHazard leaf
// (RTTI-proven; vtable_hierarchy --tree). Split out of the former GameObjectCtors.cpp
// catch-all so the class maps to one TU / one retail .obj. The ctor is the emission
// anchor (cl emits ??_7CUFO@@6B@ + the implicit post-base vptr stamp here).
//
// CUFO : CPathHazard - the base ctor (0xb35a0, via thunk 0x2fc2) is DECLARED only
// (out-of-line in PathHazard.cpp), so the leaf's base `call` reloc-masks by address.
// The base folds the whole CUserLogic init + constructs the throwing CUserBaseLink,
// so the leaf emits the /GX EH frame. Functions in ascending retail-RVA order.
#include <Gruntz/Ufo.h> // CUFO : CPathHazard (canonical; pulls PathHazard.h -> GameRegistry.h)
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/SpotLight.h>         // CSpotLight - the spawned spotlight's bound logic leaf
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <Gruntz/SerialArchive.h> // CSerialArchive (the inherited CWapX::Chain arg; ex SerialObjRef.h)
#include <Gruntz/LightFxMgr.h>    // CLightFxMgr (g_gameReg->m_logicPump->m_tables[]) - Method_b4cb0
#include <Gruntz/GruntzMgr.h> // complete CGruntzMgr
#include <rva.h>

// The game registry / settings singleton (*0x24556c). Modeled by PathHazard.h as
// g_gameReg (CGameRegistry*): the UFO spawns its spotlights through the world
// resource holder m_world (+0x30) -> m_8 (the sprite factory), and reads the light-FX
// pump m_logicPump (+0x78) for the rain-cloud sibling.

// CUFO::Tick (0xb4330) - vtable slot 16 override (origin CPathHazard): run the base
// CPathHazard per-frame driver, then report "not done" (return 0). Re-homed from
// OrphanLeaves.cpp (was the RunHelper2914 free function; matcher-6); the call
// reloc-masks to CPathHazard::Tick (0xb4020) via thunk 0x2914.
RVA(0x000b4330, 0x8)
i32 CUFO::Tick() {
    CPathHazard::Tick();
    return 0;
}

// @confidence: high
// @source: rtti-vptr
// @early-stop
// dead-spill frame-size wall (~81%): logic/offsets/CFG/the spotlight loop/CreateSprite
// arg marshaling all match retail (registry path is the canonical m_world->m_childGroup, byte-
// verified against 0x000b4a90). The single non-steerable /O2 residue: retail loads
// sy = o->m_screenY and spills it to [esp+0x30] but NEVER reloads it - a DEAD load+spill
// MSVC5 keeps in retail yet our recompile (same compiler) DCEs, so retail's frame
// reserves 8 bytes (sub esp,8) vs our 4, shifting every [esp+N] (incl. the EH trylevel
// slot) by 4 and cascading; making sy "used" would change semantics. Plus the
// eh-ctor-vptr-restamp-position late-stamp (docs/patterns/eh-ctor-vptr-restamp-position.md).
// Deferred to the final sweep.
RVA(0x000b4a90, 0x145)
CUFO::CUFO(CGameObject* obj) : CPathHazard(obj) {
    CWwdGameObjectA* o = m_object;
    i32 sx = o->m_screenX;
    i32 sy = o->m_screenY;
    m_value = m_38->m_1a0.m_14;
    m_38->ApplyLookupGeometry("LEVEL_UFO", 0);
    for (i32 i = 0; i < 2; ++i) {
        CWwdGameObjectA* sl =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, sx, 0, 0, "SpotLight", 0x40003);
        if (sl != 0) {
            sl->ApplyName("LEVEL_SPOTLIGHT");
            AnimWorkerObj* sub = sl->m_7c;
            sl->m_114 = 1;
            sl->m_12c = 0;
            sl->m_124 = 2;
            sl->m_11c = 0;
            sl->m_118 = i;
            sl->m_120 = m_object->m_130;
            sub->m_notify(sl);
            // The spotlight's bound logic leaf (CSpotLight): stash the UFO's owner
            // game-object into its reused +0x98 focus slot (both CGameObject*).
            (static_cast<CSpotLight*>(sl->m_7c->m_logic))->m_focus = m_object;
        }
    }
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 0x8;
    m_object->m_fillFraction = 0x80;
    m_object->m_area.left = 0;
    m_object->m_area.right = 0;
    m_object->m_area.top = 0;
    m_object->m_area.bottom = 0;
}

// ---------------------------------------------------------------------------
// CUFO::SerializeMove (0x0b4c40) - vtable slot 1 (the real slot-1 override; thunk
// 0x3fb7). Chains its BASE CPathHazard::SerializeMove (call 0x3035 -> 0xb4d30, the
// inherited field-transfer serialize); bails 0 on failure. On success + mode 8 it
// re-applies the ctor's draw-fill render state on the bound object (the same
// m_drawActive=1 / m_drawFillCmd=8 / m_fillFraction=0x80 the ctor seeds above).
// ---------------------------------------------------------------------------
RVA(0x000b4c40, 0x4b)
i32 CUFO::SerializeMove(CGruntArchive* ar, i32 mode, i32 c, i32 d) {
    if (!CPathHazard::SerializeMove(ar, mode, c, d)) {
        return 0;
    }
    if (mode == 8) {
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = mode;
        o->m_fillFraction = 0x80;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CUFO::Method_b4cb0 (0x0b4cb0) - re-homed from the AppHelpers.cpp holding TU (was the
// CHandlerB4::Handle / CSub10 view; xref-proven a CUFO method - it calls the base SerializeMove
// (0xb4d30) on `this` and sits in the ufo obj between SerializeMove and Serialize). Same
// serialize-then-configure archetype as SerializeMove, but on tag 8 it decorates the
// bound object (m_object, +0x10) with the light-FX draw-fill state: fill-cmd 7 pointing
// at the light-FX pump's shade table slot 5 (g_gameReg->m_logicPump->m_tables[5]). The
// exact method name is unrecovered; class is CUFO (placeholder keeps the RVA).
// ---------------------------------------------------------------------------
RVA(0x000b4cb0, 0x56)
i32 CUFO::Method_b4cb0(void* stream, i32 tag, i32 c, i32 d) {
    if (!CPathHazard::SerializeMove(static_cast<CGruntArchive*>(stream), tag, c, d)) {
        return 0;
    }
    if (tag == 8) {
        CShadeTable* x = g_gameReg->m_logicPump->m_tables[5];
        CWwdGameObjectA* o = m_object;
        o->m_drawActive = 1;
        o->m_drawFillCmd = 7;
        o->m_drawFillArg = reinterpret_cast<i32>(x);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CPathHazard::SerializeMove (0x0b4d30) - the BASE's vtable slot-1 override.
//
// RE-ATTRIBUTED 2026-07-17 (SM1) from `CUFO::Serialize`, a non-virtual twin that
// left CPathHazard's declared slot-1 virtual (PathHazard.h) with no definition.
// PROOF (gruntz.match.vtable_slot_binding WIRING):
//   * vtable_scan --holds 0x0b4d30 -> held by exactly ONE vtable: CPathHazard's
//     (0x1e7394) slot 1, via its ILT thunk. NOT CUFO's.
//   * CUFO : CPathHazard by RTTI, and CUFO's own slot 1 is a DIFFERENT body
//     (0x0b4c40) which CALLS this one - i.e. the ordinary derived->base chain.
//     A base cannot dispatch a derived class's method, so the old name was
//     impossible; CUFO inherits this body, it does not own it.
// Body stays in this TU (its retail obj neighborhood); attribution != placement.
//
// Same archetype as
// CKitchenSlime::SerializeMove: chain the shared CUserLogic serialize (0x16e7f0) and
// the +0x34 serializable sub-object (0x408c00 via the 0x1aff thunk) first - bail
// on either failure - then transfer the per-instance state. The state is three
// tag-gated groups (tag 7 = read via slot 0x2c, tag 4 = transfer via slot 0x30):
// two quad-pairs (a shared-pointer helper) then a big block of seven quadwords,
// a thirteen-element quadword array (a loop) and five dwords.
//
// The serialization stream is the shared WAP32 CSerialArchive (Read @ vtable +0x2c,
// slot 11 - the read/load direction; Write @ +0x30, slot 12 - the store/transfer
// direction; <Gruntz/SerialArchive.h>).
// ---------------------------------------------------------------------------

// One tag-gated quad-pair (two adjacent 8-byte fields), shared between two state
// vectors. Inlined with the field pointer as a parameter so cl computes the base
// once (lea) and the second field as base+8 (add) - the retail group-1/group-2 shape.
static inline void SerQuadPair(CSerialArchive* s, i32 tag, char* p) {
    if (tag != 4) {
        if (tag == 7) {
            s->Read(p, 8);
            s->Read(p + 8, 8);
        }
    } else {
        s->Write(p, 8);
        s->Write(p + 8, 8);
    }
}

RVA(0x000b4d30, 0x287)
i32 CPathHazard::SerializeMove(CGruntArchive* stream, i32 tag, i32 c, i32 d) {
    CSerialArchive* s = stream;
    char* B = reinterpret_cast<char*>(this);
    if (CUserLogic::SerializeMove(stream, tag, c, d) == 0) {
        return 0;
    }
    if (Chain(static_cast<CSerialArchive*>(stream), tag, c, reinterpret_cast<CGameObject*>(d))
        == 0) {
        return 0;
    }
    SerQuadPair(s, tag, B + 0x108);
    SerQuadPair(s, tag, B + 0x120);
    if (tag != 4) {
        if (tag == 7) {
            s->Read(B + 0x58, 8);
            s->Read(B + 0x60, 8);
            s->Read(B + 0x68, 8);
            s->Read(B + 0x70, 8);
            s->Read(B + 0x78, 8);
            s->Read(B + 0x80, 8);
            s->Read(B + 0x88, 8);
            char* p = B + 0x90;
            i32 n = 13;
            do {
                s->Read(p, 8);
                p += 8;
            } while (--n != 0);
            s->Read(B + 0xf8, 4);
            s->Read(B + 0xfc, 4);
            s->Read(B + 0x100, 4);
            s->Read(B + 0x104, 4);
            s->Read(B + 0x118, 4);
        }
    } else {
        s->Write(B + 0x58, 8);
        s->Write(B + 0x60, 8);
        s->Write(B + 0x68, 8);
        s->Write(B + 0x70, 8);
        s->Write(B + 0x78, 8);
        s->Write(B + 0x80, 8);
        s->Write(B + 0x88, 8);
        char* p = B + 0x90;
        i32 n = 13;
        do {
            s->Write(p, 8);
            p += 8;
        } while (--n != 0);
        s->Write(B + 0xf8, 4);
        s->Write(B + 0xfc, 4);
        s->Write(B + 0x100, 4);
        s->Write(B + 0x104, 4);
        s->Write(B + 0x118, 4);
    }
    return 1;
}
