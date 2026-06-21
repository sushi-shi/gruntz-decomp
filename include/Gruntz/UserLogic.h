// UserLogic.h - Gruntz game-object base hierarchy (C:\Proj\Gruntz).
//
// Reconstruction sufficient to byte-match the game-object constructors. Field
// names are placeholders; the OFFSETS and the inheritance chain are the
// load-bearing facts the matches prove.
//
// Hierarchy (recovered from RTTI ClassHierarchyDescriptors in GRUNTZ.EXE):
//     CUserBase                       vftable 0x5e70b4  (3 virtuals)
//       +-- CUserLogic : CUserBase    vftable 0x5e705c  (12 virtuals)
//             +-- CSecretLevelTrigger, CTileTrigger, CTeleporter, CWarpStonePad,
//                 CToobSpikez, ...    (the tile-logic game-object leaves)
//
// CUserBase is just a vptr; CUserLogic owns the whole data layout and the link
// sub-object at +0x18. There are TWO ctor shapes:
//
//  * the NO-ARG leaf ctor (75 B, e.g. CTileTrigger 0x011160): store CUserBase
//    vptr (0x5e70b4); construct the +0x18 link via 0x16d710 (it can throw -> the
//    /GX EH frame); store the leaf's own most-derived vptr. The intermediate
//    CUserLogic vptr store (0x5e705c) is dead-eliminated (nothing observes it).
//
//  * the 1-ARG ctor `(CGameObject*)` (e.g. CTileTrigger 0x10e220): the same base
//    prologue PLUS the full CUserLogic init the leaves share - seed the link's
//    name from the global empty string, lazily build the logic-type table once,
//    register the three built-in handlers, set the data fields. Here the
//    CUserLogic vptr store survives (the init runs after it). Each leaf adds its
//    own most-derived vptr store + a per-class tail. Modeled as an INLINE
//    CUserLogic(CGameObject*) ctor so MSVC folds it into every leaf (it only
//    folds a base ctor it can see inline).
#ifndef GRUNTZ_USERLOGIC_H
#define GRUNTZ_USERLOGIC_H

#include <rva.h>

// ---------------------------------------------------------------------------
// EngStr - the engine's small string class (the "incs" CString clone). Layout
// {vptr@0, ?@4, len@8, buf@0xc}; size 0x10. Used by the +0x18 link's name field
// and as the throw-away temp the 1-arg ctors build from the global empty string.
// Its three operations live in the engine string TU, modeled NO-body so the
// calls reloc-mask:
//   EngStr(char const*, int) = 0x16d3a0  (836B; construct from a C string)
//   operator=(EngStr const&) = 0x16d2f0  (172B; deep copy-assign)
//   ~EngStr()                = 0x16d2a0  (38B)
// ---------------------------------------------------------------------------
struct EngStr {
    EngStr(); // default (unused; lets the link's empty ctor stub compile)
    EngStr(const char* s, int n);
    ~EngStr();
    EngStr& operator=(const EngStr& o);
    void* m_0;
    int m_4;
    int m_8;
    char* m_c;
};

// The global empty C string the link's name field is seeded from (0x6293f4).
extern "C" char g_emptyString[];

// ---------------------------------------------------------------------------
// CUserBaseLink - the destructible sub-object embedded at CUserLogic+0x18. Its
// only field is an EngStr name. Its ctor (0x16d710) is the one out-of-line
// constructor the whole game-object family chains; it can throw, which is what
// makes MSVC emit the /GX EH frame in every leaf ctor.
// ---------------------------------------------------------------------------
struct CUserBaseLink {
    CUserBaseLink();  // 0x16d710 (out-of-line; can throw)
    ~CUserBaseLink(); // out-of-line dtor (cleaned up on unwind)
    EngStr m_str;     // +0x00  (its name field; the 0x5f04c8 EngStr vptr)
};

// ---------------------------------------------------------------------------
// CGameObject - the engine object the 1-arg ctors are handed (read into edi).
// Only the fields/methods those ctors touch are modeled; bodies live in engine
// TUs (modeled NO-body so the calls reloc-mask):
//   AddLogicHit/Attack/Bump = 0x150f50 / 0x151030 / 0x151110  (__thiscall, char*)
//   m_7c                    = a sub-object pointer copied into the trigger.
// ---------------------------------------------------------------------------
struct CGameObjAux; // the sub-object reached through CGameObject::m_7c

// The +0x198 layer descriptor several eyecandy ctors poll for z-clamping (its
// +0x10/+0x14 bounds + +0x1c base offset). Only the touched offsets are modeled.
struct CGameObjLayer {
    char m_pad00[0x10];
    int m_10; // +0x10
    int m_14; // +0x14
    char m_pad18[0x1c - 0x18];
    int m_1c; // +0x1c
};

struct CGameObject {
    void AddLogicHit(char* key);                        // 0x150f50
    void AddLogicAttack(char* key);                     // 0x151030
    void AddLogicBump(char* key);                       // 0x151110
    void ApplyLookupSprite(const char* key, int flag);  // 0x1504d0
    void ApplyName(const char* name);                   // 0x150540
    int ApplyLookupGeometry(const char* key, int flag); // 0x1505b0
    char m_pad00[0x04];
    int m_04; // +0x04
    int m_08; // +0x08
    char m_pad0c[0x38 - 0x0c];
    int m_38; // +0x38
    char m_pad3c[0x40 - 0x3c];
    int m_40; // +0x40
    char m_pad44[0x5c - 0x44];
    int m_5c; // +0x5c
    int m_60; // +0x60
    char m_pad64[0x74 - 0x64];
    int m_74; // +0x74
    char m_pad78[0x7c - 0x78];
    CGameObjAux* m_7c; // +0x7c
    char m_pad80[0x164 - 0x80];
    int m_164; // +0x164
    int m_168; // +0x168
    char m_pad16c[0x198 - 0x16c];
    CGameObjLayer* m_198; // +0x198
    char m_pad19c[0x1b4 - 0x19c];
    int m_1b4; // +0x1b4
};

// The +0x7c sub-object: its +0x08 flags, +0x1c bute-node and +0x130 timer are
// touched by the eyecandy/sparkle ctors.
struct CGameObjAux {
    char m_pad00[0x08];
    int m_08; // +0x08
    char m_pad0c[0x1c - 0x0c];
    void* m_1c; // +0x1c
    char m_pad20[0x130 - 0x20];
    int m_130; // +0x130
};

// The engine bute manager the eyecandy ctors query for "World"/"BigActHeight"
// (CButeMgr::GetInt 0x171af0). The class + its singleton g_buteMgr
// (?g_buteMgr@@3VCButeMgr@@A, RVA 0x2453d8) live in the bute TUs; declared
// extern only here so the `ecx=&g_buteMgr; call GetInt` shape reloc-masks
// against the already-matched symbols (BattlezMapConfig owns the DATA label).
#include <Bute/ButeMgr.h>
extern CButeMgr g_buteMgr;

// One-shot guard for the built-in tile-logic type registration (0x6bf674).
extern int g_logicTypesRegistered;

// BuildLogicTypeTable (0x8a40, via the 0x39c2 thunk): registers the three
// built-in logic types the first time any tile-logic object is built. It is a
// __thiscall member that IGNORES `this` (its impl reads the ctx as its explicit
// stack arg) - that is why the retail call carries `mov ecx,esi; push ctx`. See
// thiscall-ignoring-this. Declared as a CUserLogic method below.
struct CLogicTypeBuilder;

// ---------------------------------------------------------------------------
// CUserBase - root of the game-object hierarchy: just a vptr (3 virtuals,
// vftable 0x5e70b4). Inline ctor so it folds into derived ctors.
// ---------------------------------------------------------------------------
class CUserBase {
public:
    CUserBase() {}
    virtual ~CUserBase();         // slot 0
    virtual int UserBaseVfunc1(); // slot 1
    virtual int UserBaseVfunc2(); // slot 2
};

// ---------------------------------------------------------------------------
// CUserLogic : CUserBase - 12 virtuals (vftable 0x5e705c). Owns the shared data
// layout + the link sub-object. The default ctor just constructs the link (used
// by the no-arg leaves). The inline 1-arg ctor folds the full shared init into
// each leaf's 1-arg ctor.
// ---------------------------------------------------------------------------
class CUserLogic : public CUserBase {
public:
    CUserLogic() {}
    CUserLogic(CGameObject* obj);
    virtual ~CUserLogic() OVERRIDE; // slot 0 (most-derived dtor)
    virtual int UserLogicVfunc1();
    virtual int UserLogicVfunc2();
    virtual int UserLogicVfunc3();
    virtual int UserLogicVfunc4();
    virtual int UserLogicVfunc5();
    virtual int UserLogicVfunc6();
    virtual int UserLogicVfunc7();
    virtual int UserLogicVfunc8();
    virtual int UserLogicVfunc9();
    virtual int UserLogicVfuncA();
    virtual int UserLogicVfuncB();

    // Inline one-shot wrapper: registers the built-in logic types the first time
    // any tile-logic object is built. Inlined into the 1-arg ctor; its `this`
    // setup is why the retail call carries the dead `mov ecx,esi`.
    void RegisterLogicTypesOnce();
    void BuildLogicTypeTable(CLogicTypeBuilder* ctx); // 0x8a40 (ignores this)

    int m_04;             // +0x04
    int m_08;             // +0x08
    CGameObject* m_0c;    // +0x0c
    CGameObject* m_10;    // +0x10
    CGameObjAux* m_14;    // +0x14
    CUserBaseLink m_link; // +0x18..+0x27 (ctor 0x16d710, can throw)
    int m_28;             // +0x28
    int m_2c;             // +0x2c
    void* m_30;           // +0x30
    CGameObject* m_34;    // +0x34
    CGameObject* m_38;    // +0x38
    CGameObjAux* m_3c;    // +0x3c
};

// Shared 1-arg init the leaves fold in. Inline so MSVC inlines it; stores the
// CUserLogic vptr, then the full init. Defined here (not the .cpp) because only
// an inline base ctor is folded into the derived ctors.
inline CUserLogic::CUserLogic(CGameObject* obj) {
    m_0c = obj;
    m_10 = obj;
    m_14 = obj->m_7c;
    {
        EngStr tmp(g_emptyString, 0);
        m_link.m_str = tmp;
    }
    RegisterLogicTypesOnce();
    m_10->AddLogicHit("LogicHit");
    m_10->AddLogicAttack("LogicAttack");
    m_10->AddLogicBump("LogicBump");
    m_04 = 0;
    m_08 = 0;
    m_28 = 0x3e9;
    m_2c = 2;
    m_34 = obj;
    m_38 = obj;
    m_3c = obj->m_7c;
}

inline void CUserLogic::RegisterLogicTypesOnce() {
    if (!g_logicTypesRegistered) {
        BuildLogicTypeTable((CLogicTypeBuilder*)m_0c);
        g_logicTypesRegistered = 1;
    }
}

#endif // GRUNTZ_USERLOGIC_H
