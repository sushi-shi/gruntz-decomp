// GameObjectCtors.cpp - Gruntz game-object leaf constructors that need the /GX
// EH frame but whose bases live OUT-OF-LINE in engine TUs (C:\Proj\Gruntz).
//
// These are the game-object ctors whose shared base ctor is NOT inlined (so the
// CUserLogic-folded model in UserLogic.cpp does not apply): the per-class base
// is chained as a real out-of-line constructor that reloc-masks by address.
// Each ctor still emits the /GX exception-handling frame (the base constructs a
// throwing CUserBaseLink + the body has throwing calls), so this unit is built
// flags="eh". The CUserLogic-DERIVED leaves whose base folds inline live in
// UserLogic.cpp instead.
//
// Two families here:
//   * the CGruntSprite-based sprite ctors (base 0x7eb00 via thunk 0x3224):
//     CGruntStaminaSprite / CGruntToyTimeSprite / CGruntWingzTimeSprite.
//   * the CPathHazard-based ctors (base 0xb35a0 via thunk 0x2fc2):
//     CRainCloud / CUFO.
// Functions are defined in ascending-RVA order.
#include <Bute/ButeTree.h>      // canonical CButeTree (one shape)
#include <Gruntz/CPathHazard.h> // real CPathHazard base (: CUserLogic) for CRainCloud/CUFO
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/SerialArchive.h> // the shared CSerialArchive stream (Read @+0x2c / Write @+0x30)
#include <rva.h>

// ---------------------------------------------------------------------------
// CButeTree - the engine bute store the tails query for their "A" node.
// g_buteTree (0x6bf620 -> DATA rva 0x2bf620) is the global instance; Find
// (0x16d190) reloc-masks. Canonical crit-bit trie (include/Bute/ButeTree.h).
// ---------------------------------------------------------------------------
DATA(0x002bf620)
extern CButeTree g_buteTree;

// ===========================================================================
// The CGruntSprite-based sprite ctors.
//
// Sprite-ctor archetype (/GX EH frame). Chains the OUT-OF-LINE CGruntSprite base
// ctor (0x7eb00, via thunk 0x3224; engine fn, not matched -> per-file shell
// whose call reloc-masks by address). The base constructs a throwing
// CUserBaseLink and the body calls can throw, so MSVC emits the /GX frame that
// unwinds the base subobject (its declared dtor) on throw.
//
// Body: stamp the leaf vftable, register the sprite's GAME bute via
// m_38->ApplyLookupSprite(str, 1), seed the "A" bute node, force the object's
// pose id, then set the two sprite anchor fields m_anchorX/m_anchorY. The three
// siblings are the same shape (different vtbl/str and anchor constants).
// ===========================================================================

// --- engine helper types (offsets load-bearing) ---------------------------
struct CSpriteObj {
    void ApplyLookupSprite(const char* key, i32 flag); // 0x1504d0 (__thiscall)
    char m_pad00[0x08];
    i32 m_08; // +0x08
    char m_pad0c[0x74 - 0x0c];
    i32 m_74; // +0x74
};

struct CSpriteObjAux {
    char m_pad00[0x1c];
    void* m_1c; // +0x1c
};

// --- out-of-line base shell (reloc-masks to CGruntSprite ctor 0x7eb00) ------
// Real polymorphic base now: 17 declared-only virtuals (bodies in engine TUs)
// make cl emit each leaf's ??_7 vftable + the IMPLICIT post-base-ctor vptr stamp,
// replacing the explicit `*(void**)this = &g_...Vtbl` store. Leaf vtable names
// auto-derive (RTTI; config/vtable_names.csv). The base ctor stays DECLARED only
// (out-of-line; its `call` reloc-masks to 0x7eb00).
struct CGruntSpriteBase {
    CGruntSpriteBase(CSpriteObj* obj);
    ~CGruntSpriteBase(); // out-of-line; unwound on throw
    virtual void Vf0();
    virtual void Vf1();
    virtual void Vf2();
    virtual void Vf3();
    virtual void Vf4();
    virtual void Vf5();
    virtual void Vf6();
    virtual void Vf7();
    virtual void Vf8();
    virtual void Vf9();
    virtual void Vf10();
    virtual void Vf11();
    virtual void Vf12();
    virtual void Vf13();
    virtual void Vf14();
    virtual void Vf15();
    virtual void Vf16();
    // +0x00  implicit vptr (was an explicit m_vptr struct stamp)
    char m_pad04[0x10 - 0x04];
    CSpriteObj* m_10;    // +0x10
    CSpriteObjAux* m_14; // +0x14
    char m_pad18[0x30 - 0x18];
    void* m_30; // +0x30
    char m_pad34[0x38 - 0x34];
    CSpriteObj* m_38; // +0x38
};

class CGruntStaminaSprite : public CGruntSpriteBase {
public:
    CGruntStaminaSprite(CSpriteObj* obj);
    char m_pad3c[0x5c - 0x3c];
    i32 m_anchorX; // +0x5c  icon screen-offset X from the bound grunt
    i32 m_anchorY; // +0x60  icon screen-offset Y (drawn above the grunt)
};

class CGruntToyTimeSprite : public CGruntSpriteBase {
public:
    CGruntToyTimeSprite(CSpriteObj* obj);
    // GetTypeTag (0x120e0): 6-byte per-class logic-type id accessor (0x411).
    LogicTypeId GetTypeTag();
    char m_pad3c[0x5c - 0x3c];
    i32 m_anchorX; // +0x5c  icon screen-offset X from the bound grunt
    i32 m_anchorY; // +0x60  icon screen-offset Y (drawn above the grunt)
};

// The bound object the toy-time accessor reads its +0x3f4 timer field out of.
struct CToyTimeHost {
    char m_pad0[0x3f4];
    i32 m_toyTime; // +0x3f4  toy timer value
};

// GetToyTime (0x7fca0): free __stdcall +0x3f4 accessor (ret 4), sibling of
// GetWingzTime (+0x3f8). Not a sprite member: the ecx trace mis-homed this
// __stdcall callee (stale-ecx owner); it reads a foreign host, no fn-ptr storage.
i32 __stdcall GetToyTime(CToyTimeHost* o);

class CGruntWingzTimeSprite : public CGruntSpriteBase {
public:
    CGruntWingzTimeSprite(CSpriteObj* obj);
    char m_pad3c[0x5c - 0x3c];
    i32 m_anchorX; // +0x5c  icon screen-offset X from the bound grunt
    i32 m_anchorY; // +0x60  icon screen-offset Y (drawn above the grunt)
};

// Leaf vftables (??_7CGrunt{Stamina,ToyTime,WingzTime}Sprite@@6B@) are now
// emitted by cl and named on the target automatically (RTTI auto-namer).

// CGruntToyTimeSprite::GetTypeTag @0x000120e0 - the class's logic-type id (0x411),
// the 6-byte `mov eax,<id>; ret` archetype. RVA-lowest method in this TU; the
// plain dtor (@0x12130) lives in the engine_discovered stub unit.
RVA(0x000120e0, 0x6)
LogicTypeId CGruntToyTimeSprite::GetTypeTag() {
    return LOGIC_GRUNTTOYTIMESPRITE; // 0x411
}

// @confidence: high
// @source: rtti-vptr
// @early-stop
// eh-ctor-vptr-restamp-position wall (94.76%): real polymorphic base, body+offsets
// byte-identical; the single residual is the leaf vptr re-stamp (mov [esi],&??_7)
// which MSVC5's /GX EH-state machine sinks INTO the first throwing call's state 0
// (after the m_38 load) instead of retail's eager post-base-ctor stamp. NOT
// steerable by source spelling (doc names this exact ctor). See
// docs/patterns/eh-ctor-vptr-restamp-position.md. Deferred to the final sweep.
RVA(0x0007fae0, 0xa0)
CGruntStaminaSprite::CGruntStaminaSprite(CSpriteObj* obj) : CGruntSpriteBase(obj) {
    m_38->ApplyLookupSprite("GAME_GRUNTSTAMINASPRITE", 1);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    CSpriteObj* o = m_10;
    if (o->m_74 != 0xdbba0) {
        o->m_74 = 0xdbba0;
        o->m_08 |= 0x20000;
    }
    m_anchorX = 0x64;
    m_anchorY = -0x20;
}

// @confidence: high
// @source: rtti-vptr
// @early-stop
// eh-ctor-vptr-restamp-position wall (94.76%): same as CGruntStaminaSprite above -
// real polymorphic base, body byte-identical, only the leaf vptr re-stamp lands in
// the throwing call's EH state 0 instead of eagerly post-base. NOT source-steerable
// (docs/patterns/eh-ctor-vptr-restamp-position.md). Deferred to the final sweep.
RVA(0x0007fbd0, 0xa0)
CGruntToyTimeSprite::CGruntToyTimeSprite(CSpriteObj* obj) : CGruntSpriteBase(obj) {
    m_38->ApplyLookupSprite("GAME_GRUNTTOYTIMESPRITE", 1);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    CSpriteObj* o = m_10;
    if (o->m_74 != 0xdbba0) {
        o->m_74 = 0xdbba0;
        o->m_08 |= 0x20000;
    }
    m_anchorX = 0;
    m_anchorY = -0x20;
}

// GetToyTime @0x0007fca0 - free __stdcall accessor: read the bound host's +0x3f4
// toy-timer field (`mov eax,[esp+4]; mov eax,[eax+0x3f4]; ret 4`).
RVA(0x0007fca0, 0xd)
i32 __stdcall GetToyTime(CToyTimeHost* o) {
    return o->m_toyTime;
}

// @confidence: high
// @source: rtti-vptr
// @early-stop
// eh-ctor-vptr-restamp-position wall (94.76%): same as the two sprite ctors above -
// real polymorphic base, body byte-identical, only the leaf vptr re-stamp lands in
// the throwing call's EH state 0 instead of eagerly post-base. NOT source-steerable
// (docs/patterns/eh-ctor-vptr-restamp-position.md). Deferred to the final sweep.
RVA(0x0007fcc0, 0xa0)
CGruntWingzTimeSprite::CGruntWingzTimeSprite(CSpriteObj* obj) : CGruntSpriteBase(obj) {
    m_38->ApplyLookupSprite("GAME_GRUNTWINGZTIMESPRITE", 1);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    CSpriteObj* o = m_10;
    if (o->m_74 != 0xdbba0) {
        o->m_74 = 0xdbba0;
        o->m_08 |= 0x20000;
    }
    m_anchorX = 0;
    m_anchorY = -0x26;
}

// ===========================================================================
// The CPathHazard-based ctors (CRainCloud / CUFO).
//
// Both are 1-arg `(CGameObject*)` ctors that chain the OUT-OF-LINE CPathHazard
// 1-arg base ctor (0xb35a0, via thunk 0x2fc2; engine fn, not matched -> base
// shell whose `call` reloc-masks by address). That base folds the whole
// CUserLogic init and constructs the throwing CUserBaseLink, so the leaf emits
// the /GX frame. In the base, m_10 and m_38 both point at the constructed
// CGameObject (set equal during init); the leaf bodies write the hazard data
// fields the base leaves uninitialized.
// ===========================================================================

// The hazard ctors poke the bound engine game-object directly (m_10 == m_38 ==
// obj), modeled as the real CGameObject (<Gruntz/UserLogic.h>) - no per-TU view.
// ApplyLookupGeometry (0x1505b0) reloc-masks.

// The global game registry the hazard ctors poll; wwdfile owns the real DATA
// label (0x24556c). +0x78 is a sub-object whose +0x28 (for the rain cloud) and
// +0x30 (the spotlight factory, for the UFO) are read.
// The spotlight-spawn factory embedded at CHazardRegInner+0x08 (Spawn is 0x1597b0).
// A tiny helper with just the method so the __thiscall lowers cleanly and reloc-
// masks; embedded (not a cast) so `->m_08.Spawn(...)` takes its address directly.
struct CSpotLight;
struct CSpotLightFactory {
    CSpotLight* Spawn(i32 a, i32 b, i32 c, i32 d, const char* e, i32 f); // 0x1597b0
};
struct CHazardRegInner {
    char m_pad00[0x08];
    CSpotLightFactory m_08; // +0x08  embedded spotlight-spawn factory (its `this`)
};
struct CHazardRegSub {
    char m_pad00[0x28];
    i32 m_28; // +0x28
    char m_pad2c[0x30 - 0x2c];
    CHazardRegInner* m_30; // +0x30
};
struct CHazardReg {
    char m_pad00[0x78];
    CHazardRegSub* m_78; // +0x78
};
// wwdfile owns the DATA label for 0x24556c; declared extern only here so the
// reads reloc-mask (the type name only affects the masked symbol name).
extern CHazardReg* g_gameReg;

// A spawned CSpotLight the CUFO ctor configures (returned by the +0x30->+0x8
// factory's Spawn at 0x1597b0). Only the touched offsets are modeled; ApplyName
// (0x150540) and the +0x7c sub-object's vtable poke (slot +0x10) reloc-mask.
struct CSpotLightSubInner {
    char m_pad00[0x98];
    CGameObject* m_owner; // +0x98  the bound owner game-object (== CUFO's m_10)
};
struct CSpotLightSub {
    // The +0x7c sub-object's vtable. Slot +0x10 (Configure) is invoked indirectly
    // on `this` with the parent spotlight pushed as an arg; modeled as a typed
    // vtable so the indirect `call [eax+0x10]` falls out (no cast).
    struct Vtbl {
        void* s0[4];
        void (*Configure)(CSpotLightSub*, CSpotLight*); // slot +0x10
    };
    Vtbl* m_vptr; // +0x00
    char m_pad04[0x18 - 0x04];
    CSpotLightSubInner* m_18; // +0x18
};
struct CSpotLight {
    void ApplyName(const char* name); // 0x150540 (__thiscall)
    char m_pad00[0x7c];
    CSpotLightSub* m_7c; // +0x7c
    char m_pad80[0x114 - 0x80];
    i32 m_114; // +0x114
    i32 m_118; // +0x118
    i32 m_11c; // +0x11c
    i32 m_120; // +0x120
    i32 m_124; // +0x124
    char m_pad128[0x12c - 0x128];
    i32 m_12c; // +0x12c
};

// CRainCloud / CUFO : CPathHazard (RTTI-proven; vtable_hierarchy --tree). The real
// base is the fully-modeled 21-slot CPathHazard (<Gruntz/CPathHazard.h>); its ctor
// (0xb35a0) stays DECLARED only (out-of-line; the leaf ctor's `call` reloc-masks
// to it via thunk 0x2fc2). cl emits each leaf's ??_7 + the implicit post-base-ctor
// vptr stamp; the inherited m_10/m_38 (CUserLogic, == obj) and m_savedGeoId (+0x40)
// are read directly. Replaces the old fabricated `CPathHazardBase` stand-in.

class CRainCloud : public CPathHazard {
public:
    CRainCloud(CGameObject* obj);
    // The slots CRainCloud overrides over CPathHazard's vtable (declared only;
    // reloc-masked). slots 1/2 (origin CUserBase) stay inherited-attributed.
    virtual ~CRainCloud() OVERRIDE;         // slot 0
    virtual i32 Tick() OVERRIDE;            // slot 16
    virtual i32 HitTest(i32, i32) OVERRIDE; // slot 20
};

class CUFO : public CPathHazard {
public:
    CUFO(CGameObject* obj);
    virtual ~CUFO() OVERRIDE;    // slot 0
    virtual i32 Tick() OVERRIDE; // slot 16
    // CUFO's serialize (slot 1 in retail): kept a plain reconstructed method - its
    // 4-arg shape can't override the base's placeholder slot-1 signature; the
    // vtable slot stays inherited-attributed (reloc-masked).
    i32 Serialize(void* stream, i32 tag, i32 c, i32 d);      // 0x0b4d30
    i32 SerializeChain(void* stream, i32 tag, i32 c, i32 d); // 0x16e7f0 (base chain; call-only)
};

// Leaf vftables (??_7CRainCloud@@6B@ / ??_7CUFO@@6B@) are emitted by cl and
// named on the target automatically (RTTI auto-namer).

// @confidence: high
// @source: rtti-vptr
RVA(0x000b49b0, 0xa8)
CRainCloud::CRainCloud(CGameObject* obj) : CPathHazard(obj) {
    CGameObject* o = m_object;
    i32 n = g_gameReg->m_78->m_28;
    o->m_drawActive = 1;
    o->m_drawFillCmd = 0x7;
    o->m_drawFillArg = n;
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("LEVEL_RAINCLOUD", 0);
    m_object->m_144 = 1;
    m_object->m_14c = 1;
    m_object->m_148 = 1;
    m_object->m_150 = 1;
}

// @confidence: high
// @source: rtti-vptr
// @early-stop
// dead-spill + EH-state wall (~81%): logic/offsets/CFG/the spotlight loop/Spawn arg
// marshaling all match retail. Two non-steerable /O2 residues: (1) retail loads sy =
// o->m_60 and spills it to [esp+0x30] but NEVER reloads it - a DEAD load+spill MSVC5
// keeps in retail yet our recompile (same compiler) DCEs, so retail's frame reserves
// 8 bytes vs our 4, shifting every [esp+N] (incl. the EH trylevel slot) by 4 and
// cascading; making sy "used" to recover the dead spill would change semantics. (2)
// the eh-ctor-vptr-restamp-position late-stamp (same wall as the sprite ctors). The
// dead-store half is the entropy-tail artifact of docs/patterns/reloc-typing-vptr-
// global.md / SBI_ImageSet::Serialize. Deferred to the final sweep.
RVA(0x000b4a90, 0x145)
CUFO::CUFO(CGameObject* obj) : CPathHazard(obj) {
    CGameObject* o = m_object;
    i32 sx = o->m_screenX;
    i32 sy = o->m_screenY;
    m_savedGeoId = m_38->m_geoId;
    m_38->ApplyLookupGeometry("LEVEL_UFO", 0);
    for (i32 i = 0; i < 2; ++i) {
        CSpotLight* sl = g_gameReg->m_78->m_30->m_08.Spawn(0, sx, 0, 0, "SpotLight", 0x40003);
        if (sl != 0) {
            sl->ApplyName("LEVEL_SPOTLIGHT");
            CSpotLightSub* sub = sl->m_7c;
            sl->m_114 = 1;
            sl->m_12c = 0;
            sl->m_124 = 2;
            sl->m_11c = 0;
            sl->m_118 = i;
            sl->m_120 = m_object->m_130;
            sub->m_vptr->Configure(sub, sl);
            sl->m_7c->m_18->m_owner = m_object;
        }
    }
    m_object->m_drawActive = 1;
    m_object->m_drawFillCmd = 0x8;
    m_object->m_fillFraction = 0x80;
    m_object->m_144 = 0;
    m_object->m_14c = 0;
    m_object->m_148 = 0;
    m_object->m_150 = 0;
}

// ---------------------------------------------------------------------------
// CUFO::Serialize (0x0b4d30) - the UFO's serialize override. Same archetype as
// CKitchenSlime::Serialize: chain the shared CUserLogic serialize (0x16e7f0) and
// the +0x34 serializable sub-object (0x408c00 via the 0x1aff thunk) first - bail
// on either failure - then transfer the per-instance state. The state is three
// tag-gated groups (tag 7 = read via slot 0x2c, tag 4 = transfer via slot 0x30):
// two quad-pairs (a shared-pointer helper) then a big block of seven quadwords,
// a thirteen-element quadword array (a loop) and five dwords.
// ---------------------------------------------------------------------------

// The serialization stream is the shared WAP32 CSerialArchive (Read @ vtable +0x2c,
// slot 11 - the read/load direction; Write @ +0x30, slot 12 - the store/transfer
// direction), now the one modeled class in <Gruntz/SerialArchive.h> - the former local
// `CHazardStream` view (same shape as CKitchenSlime's CSlimeStream) is folded away.

// The +0x34 serializable sub-object the UFO chains into (Chain @0x408c00 via the
// 0x1aff thunk; same sub-chain CKitchenSlime uses).
struct CHazardSerialSub {
    i32 Chain(void* s, i32 tag, i32 c, i32 d); // 0x408c00
};

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
i32 CUFO::Serialize(void* stream, i32 tag, i32 c, i32 d) {
    CSerialArchive* s = (CSerialArchive*)stream;
    char* B = (char*)this;
    if (SerializeChain(stream, tag, c, d) == 0) {
        return 0;
    }
    if (((CHazardSerialSub*)(B + 0x34))->Chain(stream, tag, c, d) == 0) {
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

// size 0x64 recovered from operator-new sites (gruntz.analysis.news)
SIZE(CGruntStaminaSprite, 0x64);

// size 0x64 recovered from operator-new sites (gruntz.analysis.news)
SIZE(CGruntToyTimeSprite, 0x64);

// size 0x64 recovered from operator-new sites (gruntz.analysis.news)
SIZE(CGruntWingzTimeSprite, 0x64);

SIZE_UNKNOWN(CButeTree);
SIZE_UNKNOWN(CGruntSpriteBase);
SIZE_UNKNOWN(CHazardReg);
SIZE_UNKNOWN(CHazardRegInner);
SIZE_UNKNOWN(CHazardRegSub);
SIZE_UNKNOWN(CHazardSerialSub);
SIZE_UNKNOWN(CRainCloud);
SIZE_UNKNOWN(CSpotLight);
SIZE_UNKNOWN(CSpotLightFactory);
SIZE_UNKNOWN(CSpotLightSub);
SIZE_UNKNOWN(CSpotLightSubInner);
SIZE_UNKNOWN(CSpriteObj);
SIZE_UNKNOWN(CSpriteObjAux);
SIZE_UNKNOWN(CToyTimeHost);
SIZE_UNKNOWN(CUFO);
