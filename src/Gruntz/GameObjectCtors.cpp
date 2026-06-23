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
#include <rva.h>

// ---------------------------------------------------------------------------
// CButeTree - the engine bute store the tails query for their "A" node.
// g_buteTree (0x6bf620 -> DATA rva 0x2bf620) is the global instance; Find
// (0x16d190) is matched in src/Stub/CButeTree.cpp. Modeled minimally so
// `g_buteTree.Find("A")` reloc-masks.
// ---------------------------------------------------------------------------
class CButeTree {
public:
    void* Find(const char* key);
};
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
// pose id, then set the two sprite anchor fields m_5c/m_60. The three siblings
// are the same shape (different vtbl/str and m_5c/m_60 constants).
// ===========================================================================

// --- engine helper types (offsets load-bearing) ---------------------------
struct CSpriteObj {
    void ApplyLookupSprite(const char* key, int flag); // 0x1504d0 (__thiscall)
    char m_pad00[0x08];
    int m_08; // +0x08
    char m_pad0c[0x74 - 0x0c];
    int m_74; // +0x74
};

struct CSpriteObjAux {
    char m_pad00[0x1c];
    void* m_1c; // +0x1c
};

// --- out-of-line base shell (reloc-masks to CGruntSprite ctor 0x7eb00) ------
// NON-polymorphic (explicit m_vptr @+0): the leaf vptr is stamped by an explicit
// `*(void**)this = &g_...Vtbl` user store written FIRST, so MSVC5 schedules it at
// the ctor's ENTRY EH state (before the first throwing call's state region),
// matching retail's `mov [esi],vtbl` right after the base ctor. The base ctor is
// DECLARED only (out-of-line; its `call` reloc-masks to 0x7eb00).
struct CGruntSpriteBase {
    CGruntSpriteBase(CSpriteObj* obj);
    ~CGruntSpriteBase(); // out-of-line; unwound on throw
    void* m_vptr;        // +0x00
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
    int m_5c; // +0x5c
    int m_60; // +0x60
};

class CGruntToyTimeSprite : public CGruntSpriteBase {
public:
    CGruntToyTimeSprite(CSpriteObj* obj);
    char m_pad3c[0x5c - 0x3c];
    int m_5c; // +0x5c
    int m_60; // +0x60
};

class CGruntWingzTimeSprite : public CGruntSpriteBase {
public:
    CGruntWingzTimeSprite(CSpriteObj* obj);
    char m_pad3c[0x5c - 0x3c];
    int m_5c; // +0x5c
    int m_60; // +0x60
};

// Leaf vftables, referenced as DIR32 data.
DATA(0x005e7a44)
extern void* g_gruntStaminaSpriteVtbl;
DATA(0x005e79ec)
extern void* g_gruntToyTimeSpriteVtbl;
DATA(0x005e77cc)
extern void* g_gruntWingzTimeSpriteVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x0007fae0, 0xa0)
CGruntStaminaSprite::CGruntStaminaSprite(CSpriteObj* obj) : CGruntSpriteBase(obj) {
    *(void**)this = &g_gruntStaminaSpriteVtbl;
    m_38->ApplyLookupSprite("GAME_GRUNTSTAMINASPRITE", 1);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    CSpriteObj* o = m_10;
    if (o->m_74 != 0xdbba0) {
        o->m_74 = 0xdbba0;
        o->m_08 |= 0x20000;
    }
    m_5c = 0x64;
    m_60 = -0x20;
}

// @confidence: high
// @source: rtti-vptr
RVA(0x0007fbd0, 0xa0)
CGruntToyTimeSprite::CGruntToyTimeSprite(CSpriteObj* obj) : CGruntSpriteBase(obj) {
    *(void**)this = &g_gruntToyTimeSpriteVtbl;
    m_38->ApplyLookupSprite("GAME_GRUNTTOYTIMESPRITE", 1);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    CSpriteObj* o = m_10;
    if (o->m_74 != 0xdbba0) {
        o->m_74 = 0xdbba0;
        o->m_08 |= 0x20000;
    }
    m_5c = 0;
    m_60 = -0x20;
}

// @confidence: high
// @source: rtti-vptr
RVA(0x0007fcc0, 0xa0)
CGruntWingzTimeSprite::CGruntWingzTimeSprite(CSpriteObj* obj) : CGruntSpriteBase(obj) {
    *(void**)this = &g_gruntWingzTimeSpriteVtbl;
    m_38->ApplyLookupSprite("GAME_GRUNTWINGZTIMESPRITE", 1);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    CSpriteObj* o = m_10;
    if (o->m_74 != 0xdbba0) {
        o->m_74 = 0xdbba0;
        o->m_08 |= 0x20000;
    }
    m_5c = 0;
    m_60 = -0x26;
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

// The engine game-object the hazard ctors poke (m_10 == m_38 == obj). Only the
// touched offsets are modeled; ApplyLookupGeometry reloc-masks (0x1505b0).
struct CHazardObj {
    int ApplyLookupGeometry(const char* key, int flag); // 0x1505b0 (__thiscall)
    char m_pad00[0x08];
    int m_08; // +0x08
    char m_pad0c[0x4c - 0x0c];
    int m_4c; // +0x4c
    int m_50; // +0x50
    int m_54; // +0x54
    int m_58; // +0x58
    int m_5c; // +0x5c
    int m_60; // +0x60
    char m_pad64[0x130 - 0x64];
    int m_130; // +0x130
    char m_pad134[0x144 - 0x134];
    int m_144; // +0x144
    int m_148; // +0x148
    int m_14c; // +0x14c
    int m_150; // +0x150
    char m_pad154[0x1b4 - 0x154];
    int m_1b4; // +0x1b4
};

// The global game registry the hazard ctors poll; wwdfile owns the real DATA
// label (0x24556c). +0x78 is a sub-object whose +0x28 (for the rain cloud) and
// +0x30 (the spotlight factory, for the UFO) are read.
struct CHazardRegInner {
    char m_pad00[0x08];
    int m_08; // +0x08 -> the factory `this` for Spawn
};
struct CHazardRegSub {
    char m_pad00[0x28];
    int m_28; // +0x28
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
    int m_98; // +0x98
};
struct CSpotLight;
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
    int m_114; // +0x114
    int m_118; // +0x118
    int m_11c; // +0x11c
    int m_120; // +0x120
    int m_124; // +0x124
    char m_pad128[0x12c - 0x128];
    int m_12c; // +0x12c
};

// The spotlight-spawn factory (g_gameReg->m_78->m_30->m_08 is the thiscall
// `this`; Spawn is 0x1597b0). Modeled as a method on a tiny helper so the
// thiscall lowers cleanly and reloc-masks.
struct CSpotLightFactory {
    CSpotLight* Spawn(int a, int b, int c, int d, const char* e, int f); // 0x1597b0
};

// --- out-of-line base shell (reloc-masks to CPathHazard ctor 0xb35a0) -------
// NON-polymorphic (explicit m_vptr @+0): each leaf stamps its own vptr via an
// explicit `*(void**)this = &g_...Vtbl` store written FIRST (same rationale as
// the sprite ctors above). The base ctor is DECLARED only.
struct CPathHazardBase {
    CPathHazardBase(CHazardObj* obj);
    ~CPathHazardBase(); // out-of-line; unwound on throw
    void* m_vptr;       // +0x00
    char m_pad04[0x10 - 0x04];
    CHazardObj* m_10; // +0x10  (== obj)
    char m_pad14[0x38 - 0x14];
    CHazardObj* m_38; // +0x38  (== obj)
    int m_40;         // +0x40
};

class CRainCloud : public CPathHazardBase {
public:
    CRainCloud(CHazardObj* obj);
};

class CUFO : public CPathHazardBase {
public:
    CUFO(CHazardObj* obj);
};

DATA(0x005e7324)
extern void* g_rainCloudVtbl;
DATA(0x005e72b4)
extern void* g_ufoVtbl;

// @confidence: high
// @source: rtti-vptr
RVA(0x000b49b0, 0xa8)
CRainCloud::CRainCloud(CHazardObj* obj) : CPathHazardBase(obj) {
    CHazardObj* o = m_10;
    *(void**)this = &g_rainCloudVtbl;
    int n = g_gameReg->m_78->m_28;
    o->m_58 = 1;
    o->m_50 = 0x7;
    o->m_4c = n;
    m_40 = m_38->m_1b4;
    m_38->ApplyLookupGeometry("LEVEL_RAINCLOUD", 0);
    m_10->m_144 = 1;
    m_10->m_14c = 1;
    m_10->m_148 = 1;
    m_10->m_150 = 1;
}

// @confidence: high
// @source: rtti-vptr
RVA(0x000b4a90, 0x145)
CUFO::CUFO(CHazardObj* obj) : CPathHazardBase(obj) {
    CHazardObj* o = m_10;
    *(void**)this = &g_ufoVtbl;
    int sx = o->m_5c;
    int sy = o->m_60;
    m_40 = m_38->m_1b4;
    m_38->ApplyLookupGeometry("LEVEL_UFO", 0);
    for (int i = 0; i < 2; ++i) {
        CSpotLight* sl = ((CSpotLightFactory*)((char*)&g_gameReg->m_78->m_30->m_08))
                             ->Spawn(0, sx, 0, 0, "SpotLight", 0x40003);
        if (sl != 0) {
            sl->ApplyName("LEVEL_SPOTLIGHT");
            CSpotLightSub* sub = sl->m_7c;
            sl->m_114 = 1;
            sl->m_12c = 0;
            sl->m_124 = 2;
            sl->m_11c = 0;
            sl->m_118 = i;
            sl->m_120 = m_10->m_130;
            sub->m_vptr->Configure(sub, sl);
            sl->m_7c->m_18->m_98 = (int)m_10;
        }
    }
    m_10->m_58 = 1;
    m_10->m_50 = 0x8;
    m_10->m_54 = 0x80;
    m_10->m_144 = 0;
    m_10->m_14c = 0;
    m_10->m_148 = 0;
    m_10->m_150 = 0;
}

// size 0x64 recovered from operator-new sites (gruntz.analysis.news)
SIZE(CGruntStaminaSprite, 0x64);

// size 0x64 recovered from operator-new sites (gruntz.analysis.news)
SIZE(CGruntToyTimeSprite, 0x64);

// size 0x64 recovered from operator-new sites (gruntz.analysis.news)
SIZE(CGruntWingzTimeSprite, 0x64);
