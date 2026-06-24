// UserLogic.cpp - Gruntz game-object base hierarchy + the tile-logic leaf ctors
// (C:\Proj\Gruntz).
//
// Reconstructs CUserBase / CUserLogic (see include/Gruntz/UserLogic.h) and the
// game-object leaf constructors that fold them. Two ctor shapes:
//   * NO-ARG leaf ctors (75 B): base prologue + leaf vptr, members untouched.
//   * 1-ARG leaf ctors `(CGameObject*)`: fold the inline CUserLogic(obj) shared
//     init, then store the leaf vptr and run a per-class tail.
//
// The one out-of-line ctor the family chains is CUserBaseLink::CUserBaseLink
// (0x16d710, the +0x18 member); it + the EngStr/registrar externs are in
// src/Gruntz/UserBaseLink.cpp. Functions are defined in ascending-RVA order.
#include <Mfc.h> // RECT / CopyRect (CSingleFrameMessage centers in a bounds rect)
#include <Gruntz/UserLogic.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// CButeTree (declared in <Bute/ButeMgr.h>, pulled via UserLogic.h) - the engine
// bute store the tile-logic tails query for their "A" node. g_buteTree
// (0x6bf620) is the global instance; Find (0x16d190) is matched in
// src/Stub/CButeTree.cpp. Declared extern only so `g_buteTree.Find("A")`
// reloc-masks (the Stub TU owns the DATA label).
// ---------------------------------------------------------------------------
DATA(0x002bf620)
extern CButeTree g_buteTree;

// ---------------------------------------------------------------------------
// Out-of-line vtable anchors. These give each base class a real vftable in this
// TU so the inline ctors emit the right vptr stores. Bodies are not matched.
// (~CUserBase / ~CUserLogic are now inline in the header so leaf dtors fold the
// whole base teardown; the remaining out-of-line virtuals still anchor the
// vftables.)
int CUserBase::UserBaseVfunc1() {
    return 0;
}
int CUserBase::UserBaseVfunc2() {
    return 0;
}

int CUserLogic::UserLogicVfunc1() {
    return 0;
}
int CUserLogic::UserLogicVfunc2() {
    return 0;
}
int CUserLogic::UserLogicVfunc3() {
    return 0;
}
int CUserLogic::UserLogicVfunc4() {
    return 0;
}
int CUserLogic::UserLogicVfunc5() {
    return 0;
}
int CUserLogic::UserLogicVfunc6() {
    return 0;
}
int CUserLogic::UserLogicVfunc7() {
    return 0;
}
int CUserLogic::UserLogicVfunc8() {
    return 0;
}
int CUserLogic::UserLogicVfunc9() {
    return 0;
}
int CUserLogic::UserLogicVfuncA() {
    return 0;
}
int CUserLogic::UserLogicVfuncB() {
    return 0;
}

// ---------------------------------------------------------------------------
// CSecretTeleporterTrigger virtual support. Two engine externs the Serialize
// override (0x010a10) chains; both __thiscall ret 0x10 (4 args), modeled NO-body
// so the calls reloc-mask:
//   * CUserLogic::SerializeChain (0x16e7f0) - run on `this`.
//   * the +0x34 serializable sub-object's chain (0x8c00) - run on `&this->m_34`
//     (reached via `lea ecx,[esi+0x34]`). Modeled as a method on a tiny helper.
// (Both bodies are pinned in src/Stub/Discovered.cpp.)
struct CSerialSub34 {
    int Chain(int a, int b, int c, int d); // 0x8c00
};
// ===========================================================================
// Class declarations (one vftable each; some have both ctor shapes).
// ===========================================================================
class CSecretLevelTrigger : public CUserLogic {
public:
    CSecretLevelTrigger();                 // 0x010b20 (no-arg)
    CSecretLevelTrigger(CGameObject* obj); // 0x0424b0 (1-arg)
    virtual ~CSecretLevelTrigger() OVERRIDE;
};

// CTileTrigger is declared in <Gruntz/UserLogic.h>.

// The three CTileTrigger leaves (1-arg ctors, RTTI-named). Each adds no data
// members; the ctor chains CTileTrigger(obj) (out-of-line call) and the leaf vptr
// auto-stamps; the empty dtor folds the bare CUserLogic teardown. Their ctors
// were previously stubbed (manual-vptr) in src/Stub/{CTileSecretTrigger,CGiantRock,
// CCoveredPowerup}.cpp; modeled polymorphically here so the /GX EH-frame dtor folds
// (a manual-vptr model is frameless - see docs/patterns/eh-dtor-needs-base-subobject.md).
class CTileSecretTrigger : public CTileTrigger {
public:
    CTileSecretTrigger(CGameObject* obj); // 0x10fa60
    virtual ~CTileSecretTrigger() OVERRIDE;
};

class CGiantRock : public CTileTrigger {
public:
    CGiantRock(CGameObject* obj); // 0x10fa90
    virtual ~CGiantRock() OVERRIDE;
};

class CCoveredPowerup : public CTileTrigger {
public:
    CCoveredPowerup(CGameObject* obj); // 0x10fac0
    virtual ~CCoveredPowerup() OVERRIDE;
};

class CGruntHealthSprite : public CUserLogic {
public:
    CGruntHealthSprite();
    CGruntHealthSprite(CGameObject* obj); // 0x07eb00 (1-arg)
    virtual ~CGruntHealthSprite() OVERRIDE;
    char m_pad40[0x5c - 0x40]; // CUserLogic ends +0x40; anchors at +0x5c
    int m_5c;                  // +0x5c
    int m_60;                  // +0x60
};

class CVoiceTrigger : public CUserLogic {
public:
    CVoiceTrigger();
    virtual ~CVoiceTrigger() OVERRIDE;
};

class CTeleporter : public CUserLogic {
public:
    CTeleporter(CGameObject* obj); // 0x041020
    virtual ~CTeleporter() OVERRIDE;
    char m_pad40[0x58 - 0x40];  // CUserLogic ends +0x40; CTeleporter fields at +0x58
    int m_58, m_5c, m_60, m_64; // +0x58..+0x67
    void EnterField1();         // 0x1771 (this-method, no body)
    void EnterField2();         // 0x27d9 (this-method, no body)
};

class CSecretTeleporterTrigger : public CUserLogic {
public:
    CSecretTeleporterTrigger(CGameObject* obj); // 0x041e90
    virtual ~CSecretTeleporterTrigger() OVERRIDE;
    // The two overridden CUserLogic virtuals reconstructed below.
    int Serialize(int a, int b, int c, int d); // 0x010a10 (vtable slot 1)
    void FireActivation(int coord);            // 0x042150 (vtable slot 4)
    // The registered point-activation callback 0x042b80 stamped into the
    // coordinate registry by FireActivation. __thiscall, no args, returns int.
    int SpawnTeleporter(); // 0x042b80
};

class CWarpStonePad : public CUserLogic {
public:
    CWarpStonePad(CGameObject* obj); // 0x10d650
    virtual ~CWarpStonePad() OVERRIDE;
};

class CTileTriggerSwitch : public CUserLogic {
public:
    CTileTriggerSwitch(CGameObject* obj); // 0x10dc40
    virtual ~CTileTriggerSwitch() OVERRIDE;
};

class CTileTriggerTransition : public CUserLogic {
public:
    CTileTriggerTransition(CGameObject* obj); // 0x10faf0
    virtual ~CTileTriggerTransition() OVERRIDE;
};

class CToobSpikez : public CUserLogic {
public:
    CToobSpikez(CGameObject* obj); // 0x1145c0
    virtual ~CToobSpikez() OVERRIDE;
    int m_40; // +0x40
};

class CParticlez : public CUserLogic {
public:
    CParticlez(CGameObject* obj); // 0x046ad0
    virtual ~CParticlez() OVERRIDE;
};

class CAniCycle : public CUserLogic {
public:
    CAniCycle(CGameObject* obj); // 0x0aad20
    virtual ~CAniCycle() OVERRIDE;
    int m_40; // +0x40
};

class CSingleAnimation : public CUserLogic {
public:
    CSingleAnimation(CGameObject* obj); // 0x0ae7f0
    virtual ~CSingleAnimation() OVERRIDE;
};

// ---------------------------------------------------------------------------
// The CGruntSprite-family leaves (1-arg ctors). Each folds the inline
// CUserLogic(obj) base, stamps its own vptr, then runs a per-class sprite tail
// (ApplyLookupSprite/ApplyName + ApplyLookupGeometry, the "A" bute seed, a pose
// id force, anchor fields). m_5c/m_60 (CGruntHealthSprite/CGruntToySprite) and
// m_40 (the geometry token cache, the rest) are leaf fields.
// ---------------------------------------------------------------------------
class CGruntSelectedSprite : public CUserLogic {
public:
    CGruntSelectedSprite(CGameObject* obj); // 0x07e3e0
    virtual ~CGruntSelectedSprite() OVERRIDE;
    int m_40; // +0x40
};

class CGruntToySprite : public CUserLogic {
public:
    CGruntToySprite(CGameObject* obj); // 0x07f350
    virtual ~CGruntToySprite() OVERRIDE;
    char m_pad40[0x5c - 0x40];
    int m_5c; // +0x5c
};

class CGruntPowerupSprite : public CUserLogic {
public:
    CGruntPowerupSprite(CGameObject* obj); // 0x07fdb0
    virtual ~CGruntPowerupSprite() OVERRIDE;
    int m_40; // +0x40
};

// ---------------------------------------------------------------------------
// The eyecandy / simple-animation leaves (1-arg ctors). They share a common
// z-clamp tail: poll the +0x198 layer's bounds against g_buteMgr's
// World/BigActHeight, then toggle the +0x7c sub-object's flag bits. m_40 caches
// the geometry token where a tail reuses it.
// ---------------------------------------------------------------------------
class CSingleFrameMessage : public CUserLogic {
public:
    CSingleFrameMessage(CGameObject* obj); // 0x0ab310
    virtual ~CSingleFrameMessage() OVERRIDE;
};

class CSimpleAnimation : public CUserLogic {
public:
    CSimpleAnimation(CGameObject* obj); // 0x0ab940
    virtual ~CSimpleAnimation() OVERRIDE;
};

class CFrontCandy : public CUserLogic {
public:
    CFrontCandy(CGameObject* obj); // 0x0abfa0
    virtual ~CFrontCandy() OVERRIDE;
};

class CBehindCandy : public CUserLogic {
public:
    CBehindCandy(CGameObject* obj); // 0x0ac3f0
    virtual ~CBehindCandy() OVERRIDE;
};

class CEyeCandy : public CUserLogic {
public:
    CEyeCandy(CGameObject* obj); // 0x0ac620
    virtual ~CEyeCandy() OVERRIDE;
};

class CFrontCandyAni : public CUserLogic {
public:
    CFrontCandyAni(CGameObject* obj); // 0x0acf40
    virtual ~CFrontCandyAni() OVERRIDE;
    int m_40; // +0x40
};

class CBehindCandyAni : public CUserLogic {
public:
    CBehindCandyAni(CGameObject* obj); // 0x0ad540
    virtual ~CBehindCandyAni() OVERRIDE;
    int m_40; // +0x40
};

class CMenuSparkle : public CUserLogic {
public:
    CMenuSparkle(CGameObject* obj); // 0x0adbe0
    virtual ~CMenuSparkle() OVERRIDE;
    int m_40; // +0x40
};

// CPathHazard (0x13170, no-arg): same folded base schedule, then zeroes its own
// eight pointer fields at +0x108..+0x12c.
class CPathHazard : public CUserLogic {
public:
    CPathHazard();
    virtual ~CPathHazard() OVERRIDE;
    char m_pad[0x108 - 0x40]; // pad CUserLogic (ends +0x40) .. +0x107
    void* m_108;
    void* m_10c;
    void* m_110;
    void* m_114;
    char m_pad118[0x120 - 0x118];
    void* m_120;
    void* m_124;
    void* m_128;
    void* m_12c;
};

// The global game registry several tails poll for level flags (WwdGameReg, the
// same symbol wwdfile labels at RVA 0x24556c; only the fields these tails read
// are modeled). Declared extern only - wwdfile owns the DATA label.
struct WwdGameRegAux {
    char m_pad00[0x3c];
    int m_3c; // +0x3c
};

// The on-screen-cue receiver (g_gameReg->m_60). The teleporter spawn fires a
// 6-arg cue (CueA, ret 0x18, via the 0x39f4 thunk). External/no-body
// (reloc-masked).
struct CTeleCueSink {
    void CueA(void* hit, int b, int c, int d, int e, int f); // 0x11b3b0
};

// The point-probe sink (g_gameReg->m_68): given screen x/y, fills two out-ints
// and returns the trigger object hit (or 0). 5-arg __thiscall ret 0x14, via the
// 0x35f3 thunk (-> 0x75af0). External/no-body (reloc-masked).
struct CTrigger; // the trigger object the probe returns / the cue receives
struct CTriggerProbe {
    CTrigger* Probe(int x, int y, int* outA, int* outB, int flag); // 0x75af0
};

// The viewport rect base reached as g_gameReg->m_30->m_24->m_5c + 0x40; the
// on-screen test reads its left/top/right/bottom (m_0/m_4/m_8/m_c).
struct CViewRect {
    int m_0; // left
    int m_4; // top
    int m_8; // right
    int m_c; // bottom
};
struct CViewport {
    char m_pad0[0x5c];
    char* m_5c; // +0x5c  rect-base pointer (test reads (CViewRect*)(m_5c + 0x40))
};

// The HUD sprite object the teleporter spawn produces / reads its template from
// (the trigger's m_10). The spawn copies its tile/teleport-link fields. Only the
// touched offsets are modeled.
struct CTeleHudAux {
    char m_pad0[0xbc];
    int m_bc; // +0xbc  teleport-link id
};
struct CTeleHudSprite {
    char m_pad0[0x5c];
    int m_5c; // +0x5c  screen x
    int m_60; // +0x60  screen y
    char m_pad64[0x7c - 0x64];
    CTeleHudAux* m_7c; // +0x7c
    char m_pad80[0x114 - 0x80];
    int m_114; // +0x114  tile col
    int m_118; // +0x118  tile row
    int m_11c; // +0x11c
    int m_120; // +0x120
    int m_124; // +0x124  state
    int m_128; // +0x128
    char m_pad12c[0x164 - 0x12c];
    int m_164; // +0x164
    int m_168; // +0x168
};

// The HUD sprite factory the spawn calls (g_gameReg->m_30->m_8->CreateSprite).
struct CTeleSpriteFactory {
    CTeleHudSprite* CreateSprite(
        int kind,
        int gx,
        int gy,
        int hint,
        const char* name,
        int flags
    ); // 0x1597b0
};

// The trigger object the probe returns (its m_10 is the HUD sprite read for the
// on-screen cue's coordinates).
struct CTrigger {
    char m_pad0[0x10];
    CTeleHudSprite* m_10; // +0x10
};
struct CTeleResHolder { // the +0x30 resource/sprite-factory holder
    char m_pad0[0x8];
    CTeleSpriteFactory* m_8; // +0x08
    char m_pad0c[0x24 - 0xc];
    CViewport* m_24; // +0x24  viewport (visible-bounds source)
};

struct WwdGameReg {
    // Fills the passed RECT with the on-screen message bounds and returns it
    // (0x2cb1 thunk; __thiscall). Modeled NO-body so the call reloc-masks.
    RECT* GetMessageBounds(RECT* out);
    char m_pad00[0x30];
    CTeleResHolder* m_30; // +0x30  sprite-factory/viewport holder
    char m_pad34[0x60 - 0x34];
    CTeleCueSink* m_60; // +0x60  on-screen cue receiver
    char m_pad64[0x68 - 0x64];
    CTriggerProbe* m_68; // +0x68  point-probe sink
    char m_pad6c[0x7c - 0x6c];
    WwdGameRegAux* m_7c; // +0x7c
    char m_pad80[0x118 - 0x80];
    int m_118; // +0x118
    char m_pad11c[0x130 - 0x11c];
    int m_130; // +0x130
    int m_134; // +0x134
};
extern WwdGameReg* g_gameReg;

// The CRT rand() (0x11fee0); CMenuSparkle seeds its +0x130 timer from it.
extern "C" int rand(void);

// ---------------------------------------------------------------------------
// The per-coordinate activation registry FireActivation (0x042150) dispatches
// through. A coordinate maps to an Entry* either directly (when within the
// fast [g_actLo,g_actHi] range) via g_actBase + (coord-g_actLo)*g_actStride, or
// by a slow lookup in g_actColl (0x16da80, __thiscall ret 8), which on miss
// rebuilds the table (g_actAlloc 0x16d990 -> g_actCache, g_actColl2 insert
// 0x16d850 __thiscall ret 0xc) and yields g_actCur. The entry's first dword is a
// fn-ptr table; a nonzero entry's handler is called __thiscall on `this`.
// All globals are unnamed BSS (DATA-pinned here so the loads reloc-mask); the
// collection methods are external/no-body.
struct CActEntry; // an entry: first dword is the registered handler vtable
struct CActColl {
    int Find(int coord, int z); // 0x16da80 (__thiscall ret 8)
};
struct CActColl2 {
    void Insert(void* coll, void* item, int n); // 0x16d850 (__thiscall ret 0xc)
};
extern "C" int ActAlloc(); // 0x16d990

DATA(0x00244690)
extern int g_actLo;
DATA(0x00244694)
extern int g_actHi;
DATA(0x00244698)
extern char* g_actBase;
DATA(0x002446a0)
extern int g_actStride;
DATA(0x0024469c)
extern CActEntry* g_actCur;
DATA(0x002446a8)
extern int g_actScratch;
DATA(0x00244688)
extern CActColl g_actColl;
DATA(0x0024468c)
extern CActColl2* g_actColl2;
DATA(0x002bf464)
extern void* g_actCache;
DATA(0x002bf428)
extern void* g_actAllocResult;

// The Entry the registry yields: its first dword is the handler fn-ptr, a
// __thiscall called with the trigger as `this`.
class CSecretTeleporterTrigger;
// The entry's first dword is a pointer-to-member-function of the trigger class
// (single inheritance -> a 4-byte code pointer); FireActivation invokes it on
// `this`, emitting `mov ecx,this; call [entry]`.
typedef void (CSecretTeleporterTrigger::*ActHandler)();
struct CActEntry {
    ActHandler m_fn; // [entry]
};

// The inlined coordinate->Entry* lookup FireActivation folds in twice.
static inline CActEntry* ActLookup(int coord) {
    g_actScratch = 0;
    if (coord >= g_actLo && coord <= g_actHi) {
        return (CActEntry*)(g_actBase + (coord - g_actLo) * g_actStride);
    }
    if (g_actColl.Find(coord, 0)) {
        return (CActEntry*)(g_actBase + (coord - g_actLo) * g_actStride);
    }
    void* item = g_actCache;
    g_actAllocResult = (void*)ActAlloc();
    g_actColl2->Insert(&g_actColl, item, 0xc);
    return g_actCur;
}

// ===========================================================================
// Definitions in ascending-RVA order.
// ===========================================================================

// --- CSecretTeleporterTrigger::Serialize (0x010a10), vtable slot 1 ---
// Chains the shared serialize helper on `this`, and (only on success) the +0x34
// serializable sub-object's chain; normalizes the result to a strict bool.
RVA(0x00010a10, 0x47)
int CSecretTeleporterTrigger::Serialize(int a, int b, int c, int d) {
    if (!SerializeChain(a, b, c, d)) {
        return 0;
    }
    return ((CSerialSub34*)((char*)this + 0x34))->Chain(a, b, c, d) != 0;
}

// --- CSecretTeleporterTrigger::~CSecretTeleporterTrigger (0x010ab0) ---
// The leaf adds no members, so its dtor folds the bare CUserLogic teardown:
// store the CUserLogic vptr, inline-destruct the +0x18 link (the embedded
// ~EngStr call), store the CUserBase vptr. The destructible link forces the /GX
// EH frame. The fold requires ~CUserBase/~CUserLogic/~CUserBaseLink to be inline
// (see UserLogic.h); the empty leaf body below is enough for cl to emit it.
RVA(0x00010ab0, 0x44)
CSecretTeleporterTrigger::~CSecretTeleporterTrigger() {}

CSecretLevelTrigger::~CSecretLevelTrigger() {}
RVA(0x00010b20, 0x4b)
CSecretLevelTrigger::CSecretLevelTrigger() {}

RVA(0x00011160, 0x4b)
CTileTrigger::CTileTrigger() {}

// --- CTileTrigger / leaf destructors (0x011290 / 0x011540 / 0x011600 / 0x0116c0) ---
// All four are the SAME folded CUserLogic teardown (store CUserLogic vptr,
// inline-destruct the +0x18 link via ~EngStr 0x16d2a0, store CUserBase vptr; the
// destructible link forces the /GX EH frame; leaf vptr store dead-eliminated).
// ~CTileTrigger is inline (header) so it folds into the three leaf dtors instead
// of being called. MSVC still emits one out-of-line COMDAT copy of ~CTileTrigger
// (called by its scalar-deleting dtor); it lands at 0x011290. An inline-defined
// dtor can't hang an RVA() (the attribute would also tag the synthesized ??_G ->
// duplicate-RVA), so it is pinned by mangled name here:
// @rva-symbol: ??1CTileTrigger@@UAE@XZ 0x00011290 0x44
RVA(0x00011540, 0x44)
CTileSecretTrigger::~CTileSecretTrigger() {}
RVA(0x00011600, 0x44)
CGiantRock::~CGiantRock() {}
RVA(0x000116c0, 0x44)
CCoveredPowerup::~CCoveredPowerup() {}

CGruntHealthSprite::~CGruntHealthSprite() {}
RVA(0x00011ef0, 0x4b)
CGruntHealthSprite::CGruntHealthSprite() {}

CPathHazard::~CPathHazard() {}
RVA(0x00013170, 0x7b)
CPathHazard::CPathHazard() {
    m_108 = 0;
    m_110 = 0;
    m_10c = 0;
    m_114 = 0;
    m_120 = 0;
    m_128 = 0;
    m_124 = 0;
    m_12c = 0;
}

CVoiceTrigger::~CVoiceTrigger() {}
RVA(0x00013470, 0x4b)
CVoiceTrigger::CVoiceTrigger() {}

// --- CTeleporter (0x041020), vptr 0x5e80cc ---
CTeleporter::~CTeleporter() {}
RVA(0x00041020, 0x170)
CTeleporter::CTeleporter(CGameObject* obj) : CUserLogic(obj) {
    m_58 = 0;
    m_60 = 0;
    m_5c = 0;
    m_64 = 0;
    m_38->m_08 |= 0x2000002;
    if (m_10->m_74 != 0x1869f) {
        m_10->m_74 = 0x1869f;
        m_10->m_08 |= 0x20000;
    }
    m_10->m_5c = (m_10->m_5c & ~0x1f) + 0x10;
    m_10->m_60 = (m_10->m_60 & ~0x1f) + 0x10;
    EnterField1();
    EnterField2();
}

// --- CSecretTeleporterTrigger (0x041e90), vptr 0x5e7564 ---
RVA(0x00041e90, 0x1ac)
CSecretTeleporterTrigger::CSecretTeleporterTrigger(CGameObject* obj) : CUserLogic(obj) {
    if (g_gameReg->m_118 == 0 && g_gameReg->m_134 == 1) {
        m_38->m_08 |= 0x10000;
    } else {
        m_10->m_5c = (m_10->m_5c & ~0x1f) + 0x10;
        m_10->m_60 = (m_10->m_60 & ~0x1f) + 0x10;
        if (m_10->m_74 != 0) {
            m_10->m_74 = 0;
            m_10->m_08 |= 0x20000;
        }
        m_38->m_08 |= 2;
        m_38->m_40 |= 1;
        m_30 = m_14->m_1c;
        m_14->m_1c = g_buteTree.Find("A");
        g_gameReg->m_7c->m_3c++;
    }
}

// --- CSecretTeleporterTrigger::FireActivation (0x042150), vtable slot 4 ---
// Look the activation coordinate up in the per-coordinate registry; if the entry
// has a registered handler, look it up again and dispatch it __thiscall on this.
RVA(0x00042150, 0x102)
void CSecretTeleporterTrigger::FireActivation(int coord) {
    CActEntry* e = ActLookup(coord);
    if (e->m_fn != 0) {
        CActEntry* e2 = ActLookup(coord);
        (this->*(e2->m_fn))();
    }
}

// --- CSecretLevelTrigger 1-arg (0x0424b0), vptr 0x5e8804 ---
RVA(0x000424b0, 0x1a0)
CSecretLevelTrigger::CSecretLevelTrigger(CGameObject* obj) : CUserLogic(obj) {
    if (g_gameReg->m_134 == 1 && g_gameReg->m_130 == 0) {
        m_10->m_5c = (m_10->m_5c & ~0x1f) + 0x10;
        m_10->m_60 = (m_10->m_60 & ~0x1f) + 0x10;
        if (m_10->m_74 != 0) {
            m_10->m_74 = 0;
            m_10->m_08 |= 0x20000;
        }
        m_38->m_08 |= 2;
        m_38->m_40 |= 1;
        m_30 = m_14->m_1c;
        m_14->m_1c = g_buteTree.Find("A");
    } else {
        m_38->m_08 |= 0x10000;
    }
}

// --- CSecretTeleporterTrigger::SpawnTeleporter (0x042b80) ---
// The registered point-activation callback: probe the trigger's screen point for
// a hit grunt; if hit, spawn the "Teleporter" HUD sprite at the (tile<<5)+0x10
// position, clone the trigger's teleport-link/tile fields into it, and (when the
// hit grunt is on-screen) fire the 6-arg cue. Always closes by marking the
// trigger sub-object hidden (m_38->m_08 |= 0x10000).
RVA(0x00042b80, 0x153)
int CSecretTeleporterTrigger::SpawnTeleporter() {
    int loc0, loc4;
    CTeleHudSprite* o = (CTeleHudSprite*)m_10;
    CTrigger* hit = g_gameReg->m_68->Probe(o->m_5c, o->m_60, &loc0, &loc4, 1);
    if (hit) {
        o = (CTeleHudSprite*)m_10;
        CTeleSpriteFactory* fac = g_gameReg->m_30->m_8;
        CTeleHudSprite* spr = fac->CreateSprite(
            0,
            (o->m_114 << 5) + 0x10,
            (o->m_118 << 5) + 0x10,
            0,
            "Teleporter",
            0x40003
        );
        if (spr) {
            spr->m_124 = 2;
            spr->m_7c->m_bc = ((CTeleHudSprite*)m_10)->m_7c->m_bc;
            spr->m_164 = ((CTeleHudSprite*)m_10)->m_164;
            spr->m_168 = ((CTeleHudSprite*)m_10)->m_168;
            spr->m_11c = ((CTeleHudSprite*)m_10)->m_11c;
            spr->m_120 = ((CTeleHudSprite*)m_10)->m_120;
            spr->m_114 = ((CTeleHudSprite*)m_10)->m_114;
            spr->m_118 = ((CTeleHudSprite*)m_10)->m_118;
            spr->m_128 = 0;
            CTeleHudSprite* eo = hit->m_10;
            WwdGameReg* g = g_gameReg;
            int ey = eo->m_60;
            int ex = eo->m_5c;
            CViewRect* rc = (CViewRect*)(g->m_30->m_24->m_5c + 0x40);
            if (ex < rc->m_8 && ex >= rc->m_0 && ey < rc->m_c && ey >= rc->m_4) {
                g->m_60->CueA(hit, 0x3fc, -1, 0, -1, -1);
            }
        }
        m_38->m_08 |= 0x10000;
    }
    return 0;
}

// --- CParticlez (0x046ad0), vptr 0x5e7614 ---
CParticlez::~CParticlez() {}
RVA(0x00046ad0, 0x15e)
CParticlez::CParticlez(CGameObject* obj) : CUserLogic(obj) {
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    m_38->m_08 |= 0x2000002;
    if (m_10->m_74 != 0xcf84f) {
        m_10->m_74 = 0xcf84f;
        m_10->m_08 |= 0x20000;
    }
    m_10->m_38 = 0;
}

// --- CGruntSelectedSprite (0x07e3e0), vptr 0x5e7bfc ---
CGruntSelectedSprite::~CGruntSelectedSprite() {}
RVA(0x0007e3e0, 0x178)
CGruntSelectedSprite::CGruntSelectedSprite(CGameObject* obj) : CUserLogic(obj) {
    m_38->ApplyName("GAME_GRUNTSELECTEDSPRITE");
    m_40 = m_38->m_1b4;
    m_38->ApplyLookupGeometry("GAME_GRUNTSELECTEDSPRITE", 0);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    if (m_10->m_74 != 0x14) {
        m_10->m_74 = 0x14;
        m_10->m_08 |= 0x20000;
    }
}

// --- CGruntHealthSprite (0x07eb00), vptr 0x5e7ba4 ---
RVA(0x0007eb00, 0x170)
CGruntHealthSprite::CGruntHealthSprite(CGameObject* obj) : CUserLogic(obj) {
    m_38->ApplyLookupSprite("GAME_GRUNTHEALTHSPRITE", 1);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    m_5c = 0x64;
    if (m_10->m_74 != 0xdbba0) {
        m_10->m_74 = 0xdbba0;
        m_10->m_08 |= 0x20000;
    }
    m_60 = -0x19;
}

// --- CGruntToySprite (0x07f350), vptr 0x5e7b4c ---
CGruntToySprite::~CGruntToySprite() {}
RVA(0x0007f350, 0x16a)
CGruntToySprite::CGruntToySprite(CGameObject* obj) : CUserLogic(obj) {
    m_38->ApplyLookupSprite("GAME_STATUSBAR_TABZ_STATZTAB_SMALL", 0);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    m_38->m_40 |= 1;
    if (m_10->m_74 != 0xdbba0) {
        m_10->m_74 = 0xdbba0;
        m_10->m_08 |= 0x20000;
    }
    m_5c = 0;
}

// --- CGruntPowerupSprite (0x07fdb0), vptr 0x5e76c4 ---
CGruntPowerupSprite::~CGruntPowerupSprite() {}
RVA(0x0007fdb0, 0x166)
CGruntPowerupSprite::CGruntPowerupSprite(CGameObject* obj) : CUserLogic(obj) {
    m_38->ApplyName("GAME_LIGHTING_POWERUP");
    m_40 = m_38->m_1b4;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    if (m_10->m_74 != 0x15) {
        m_10->m_74 = 0x15;
        m_10->m_08 |= 0x20000;
    }
    m_38->m_40 |= 1;
}

// --- CAniCycle (0x0aad20), vptr 0x5e86a4 ---
CAniCycle::~CAniCycle() {}
RVA(0x000aad20, 0x15c)
CAniCycle::CAniCycle(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_08 |= 1;
    if (m_38->m_1b4 == 0) {
        m_40 = m_38->m_1b4;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
}

// --- CSingleFrameMessage (0x0ab310), vptr 0x5e864c ---
// The tail asks g_gameReg for the on-screen message-bounds RECT (0x2cb1 thunk),
// copies it into a local via the CopyRect Win32 import, then centers the object
// (m_10->m_5c/m_60) inside it. ApplyLookupSprite takes m_38->m_04 as its flag.
CSingleFrameMessage::~CSingleFrameMessage() {}
RVA(0x000ab310, 0x18d)
CSingleFrameMessage::CSingleFrameMessage(CGameObject* obj) : CUserLogic(obj) {
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    m_10->ApplyLookupSprite("GAME_MESSAGEZ", m_38->m_04);
    RECT bounds;
    RECT r;
    CopyRect(&r, g_gameReg->GetMessageBounds(&bounds));
    m_10->m_5c = r.left + (r.right - r.left) / 2;
    m_10->m_60 = r.top + (r.bottom - r.top) / 2;
}

// --- CSimpleAnimation (0x0ab940), vptr 0x5e8544 ---
CSimpleAnimation::~CSimpleAnimation() {}
RVA(0x000ab940, 0x1b8)
CSimpleAnimation::CSimpleAnimation(CGameObject* obj) : CUserLogic(obj) {
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    CGameObjLayer* aux = m_10->m_198;
    if (aux != 0) {
        if (aux->m_10 >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_10->m_198->m_14 >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_10->m_7c != 0) {
                m_10->m_7c->m_08 &= ~6;
                m_10->m_7c->m_08 |= 1;
                m_38->m_08 &= ~0x1000002;
                m_38->m_08 |= 0x800000;
            }
        }
    }
}

// --- CFrontCandy (0x0abfa0), vptr 0x5e84ec ---
CFrontCandy::~CFrontCandy() {}
RVA(0x000abfa0, 0x1b6)
CFrontCandy::CFrontCandy(CGameObject* obj) : CUserLogic(obj) {
    if (m_10->m_74 != 0xf4240) {
        m_10->m_74 = 0xf4240;
        m_10->m_08 |= 0x20000;
    }
    CGameObjLayer* aux = m_10->m_198;
    if (aux != 0) {
        if (aux->m_10 >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_10->m_198->m_14 >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_10->m_7c != 0) {
                m_10->m_7c->m_08 &= ~6;
                m_10->m_7c->m_08 |= 1;
                m_38->m_08 &= ~0x1000002;
                m_38->m_08 |= 0x800000;
            }
        }
    }
}

// --- CBehindCandy (0x0ac3f0), vptr 0x5e8494 ---
CBehindCandy::~CBehindCandy() {}
RVA(0x000ac3f0, 0x1b1)
CBehindCandy::CBehindCandy(CGameObject* obj) : CUserLogic(obj) {
    if (m_10->m_74 != 0) {
        m_10->m_74 = 0;
        m_10->m_08 |= 0x20000;
    }
    if (m_10->m_198 != 0) {
        if (m_10->m_198->m_10 >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_10->m_198->m_14 >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_10->m_7c != 0) {
                m_10->m_7c->m_08 &= ~6;
                m_10->m_7c->m_08 |= 1;
                m_38->m_08 &= ~0x1000002;
                m_38->m_08 |= 0x800000;
            }
        }
    }
}

// --- CEyeCandy (0x0ac620), vptr 0x5e843c ---
CEyeCandy::~CEyeCandy() {}
RVA(0x000ac620, 0x1cf)
CEyeCandy::CEyeCandy(CGameObject* obj) : CUserLogic(obj) {
    CGameObject* o = m_10;
    if (o->m_74 == 0 && o->m_198 != 0) {
        int v = o->m_198->m_1c + o->m_60 + 0x186a0;
        if (o->m_74 != v) {
            o->m_74 = v;
            o->m_08 |= 0x20000;
        }
    }
    CGameObjLayer* aux = m_10->m_198;
    if (aux != 0) {
        if (aux->m_10 >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_10->m_198->m_14 >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_10->m_7c != 0) {
                m_10->m_7c->m_08 &= ~6;
                m_10->m_7c->m_08 |= 1;
                m_38->m_08 &= ~0x1000002;
                m_38->m_08 |= 0x800000;
            }
        }
    }
}

// --- CFrontCandyAni (0x0acf40), vptr 0x5e83e4 ---
CFrontCandyAni::~CFrontCandyAni() {}
RVA(0x000acf40, 0x16e)
CFrontCandyAni::CFrontCandyAni(CGameObject* obj) : CUserLogic(obj) {
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    if (m_38->m_1b4 == 0) {
        m_40 = m_38->m_1b4;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    if (m_10->m_74 != 0xf4240) {
        m_10->m_74 = 0xf4240;
        m_10->m_08 |= 0x20000;
    }
}

// --- CBehindCandyAni (0x0ad540), vptr 0x5e838c ---
CBehindCandyAni::~CBehindCandyAni() {}
RVA(0x000ad540, 0x1f0)
CBehindCandyAni::CBehindCandyAni(CGameObject* obj) : CUserLogic(obj) {
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    if (m_38->m_1b4 == 0) {
        m_40 = m_38->m_1b4;
        m_38->ApplyLookupGeometry("GAME_CYCLE100", 0);
    }
    if (m_10->m_74 != 0) {
        m_10->m_74 = 0;
        m_10->m_08 |= 0x20000;
    }
    if (m_10->m_198 != 0) {
        if (m_10->m_198->m_10 >= g_buteMgr.GetInt("World", "BigActHeight")
            || m_10->m_198->m_14 >= g_buteMgr.GetInt("World", "BigActHeight")) {
            if (m_10->m_7c != 0) {
                m_10->m_7c->m_08 &= ~6;
                m_10->m_7c->m_08 |= 1;
                m_38->m_08 &= ~0x1000002;
                m_38->m_08 |= 0x800000;
            }
        }
    }
}

// --- CMenuSparkle (0x0adbe0), vptr 0x5e82dc ---
CMenuSparkle::~CMenuSparkle() {}
RVA(0x000adbe0, 0x178)
CMenuSparkle::CMenuSparkle(CGameObject* obj) : CUserLogic(obj) {
    m_38->ApplyName("MENU_SPARKLE");
    m_40 = m_38->m_1b4;
    m_38->ApplyLookupGeometry("MENU_FORWARD100", 0);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    m_14->m_130 = rand() % 0xfa1 + 0x3e8;
}

// --- CSingleAnimation (0x0ae7f0), vptr 0x5e745c ---
CSingleAnimation::~CSingleAnimation() {}
RVA(0x000ae7f0, 0x13d)
CSingleAnimation::CSingleAnimation(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_08 |= 2;
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
}

// --- CWarpStonePad (0x10d650), vptr 0x5e71ac ---
CWarpStonePad::~CWarpStonePad() {}
RVA(0x0010d650, 0x16c)
CWarpStonePad::CWarpStonePad(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_08 |= 2;
    m_38->m_08 |= 1;
    if (g_gameReg->m_134 == 1) {
        m_38->m_40 |= 1;
        m_38->m_08 |= 0x10000;
    }
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
}

// --- CTileTriggerSwitch (0x10dc40), vptr 0x5e7f6c ---
CTileTriggerSwitch::~CTileTriggerSwitch() {}
RVA(0x0010dc40, 0x154)
CTileTriggerSwitch::CTileTriggerSwitch(CGameObject* obj) : CUserLogic(obj) {
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    m_38->m_08 |= 2;
    m_38->m_08 |= 1;
    m_38->m_40 |= 1;
}

// --- CTileTrigger 1-arg (0x10e220), vptr 0x5e7f14 ---
RVA(0x0010e220, 0x17d)
CTileTrigger::CTileTrigger(CGameObject* obj) : CUserLogic(obj) {
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    m_38->m_08 |= 2;
    m_38->m_08 |= 1;
    m_38->m_40 |= 1;
    m_10->m_164 = m_10->m_5c >> 5;
    m_10->m_168 = m_10->m_60 >> 5;
    m_10->m_04 = (m_10->m_164 << 8) + m_10->m_168;
}

// --- The three CTileTrigger leaves' 1-arg ctors (0x10fa60/90/c0) ---
// Each just chains CTileTrigger(obj) (out-of-line call) then the leaf vptr
// auto-stamps. vptrs: CTileSecretTrigger 0x5e7e64, CGiantRock 0x5e7d5c,
// CCoveredPowerup 0x5e7e0c.
RVA(0x0010fa60, 0x19)
CTileSecretTrigger::CTileSecretTrigger(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fa90, 0x19)
CGiantRock::CGiantRock(CGameObject* obj) : CTileTrigger(obj) {}
RVA(0x0010fac0, 0x19)
CCoveredPowerup::CCoveredPowerup(CGameObject* obj) : CTileTrigger(obj) {}

// --- CTileTriggerTransition (0x10faf0), vptr 0x5e7db4 ---
CTileTriggerTransition::~CTileTriggerTransition() {}
RVA(0x0010faf0, 0x128)
CTileTriggerTransition::CTileTriggerTransition(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_08 |= 0x1000000;
    if (m_10->m_74 != 0) {
        m_10->m_74 = 0;
        m_10->m_08 |= 0x20000;
    }
}

// --- CToobSpikez (0x1145c0), vptr 0x5e7774 ---
CToobSpikez::~CToobSpikez() {}
RVA(0x001145c0, 0x18e)
CToobSpikez::CToobSpikez(CGameObject* obj) : CUserLogic(obj) {
    m_40 = m_38->m_1b4;
    m_38->ApplyLookupGeometry("GAME_CYCLE100", 2);
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    m_38->m_08 |= 2;
    m_10->m_164 = m_10->m_5c >> 5;
    m_10->m_168 = m_10->m_60 >> 5;
    if (m_10->m_74 != 0xc) {
        m_10->m_74 = 0xc;
        m_10->m_08 |= 0x20000;
    }
}

// @confidence: low
// @source: winapi:CopyRect
// @stub
RVA(0x0004d800, 0x423)
int CUserLogic::winapi_04d800_CopyRect(int, int, int, int, int, int, int, int, int, int, int, int) {
    return 0;
}

// @confidence: low
// @source: winapi:PostMessageA
// @stub
RVA(0x00064540, 0x11c)
int CUserLogic::winapi_064540_PostMessageA() {
    return 0;
}

// @confidence: low
// @source: winapi:IntersectRect;PtInRect
// @stub
RVA(0x000ee800, 0x971)
int CUserLogic::winapi_0ee800_IntersectRect_PtInRect() {
    return 0;
}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0004dd50, 0x22c0)
void CUserLogic::LoadGruntTypeTable(int, int, int, int) {}

// @confidence: med
// @source: string-xref
// @stub
RVA(0x0005d210, 0x1443)
void CUserLogic::LoadGruntTuningConstants(int) {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x000612a0, 0x23c)
void CUserLogic::LoadGruntDecayConfig() {}

// @confidence: low
// @source: decomp-xref
// @stub
RVA(0x00061570, 0x11d)
void CUserLogic::LoadGruntDecayConfig2() {}

// @confidence: med
// @source: decomp-xref
// @stub
RVA(0x00065a60, 0x159)
void CUserLogic::LoadWandGruntItemConfig() {}
