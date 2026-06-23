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
DATA(0x2bf620)
extern CButeTree g_buteTree;

// ---------------------------------------------------------------------------
// Out-of-line vtable anchors. These give each base class a real vftable in this
// TU so the inline ctors emit the right vptr stores. Bodies are not matched.
CUserBase::~CUserBase() {}
int CUserBase::UserBaseVfunc1() {
    return 0;
}
int CUserBase::UserBaseVfunc2() {
    return 0;
}

CUserLogic::~CUserLogic() {}
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

// ===========================================================================
// Class declarations (one vftable each; some have both ctor shapes).
// ===========================================================================
class CSecretLevelTrigger : public CUserLogic {
public:
    CSecretLevelTrigger();                 // 0x010b20 (no-arg)
    CSecretLevelTrigger(CGameObject* obj); // 0x0424b0 (1-arg)
    virtual ~CSecretLevelTrigger() OVERRIDE;
};

class CTileTrigger : public CUserLogic {
public:
    CTileTrigger();                 // 0x011160 (no-arg)
    CTileTrigger(CGameObject* obj); // 0x10e220 (1-arg)
    virtual ~CTileTrigger() OVERRIDE;
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
struct WwdGameReg {
    // Fills the passed RECT with the on-screen message bounds and returns it
    // (0x2cb1 thunk; __thiscall). Modeled NO-body so the call reloc-masks.
    RECT* GetMessageBounds(RECT* out);
    char m_pad00[0x7c];
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

// ===========================================================================
// Definitions in ascending-RVA order.
// ===========================================================================
CSecretLevelTrigger::~CSecretLevelTrigger() {}
RVA(0x010b20, 0x4b)
CSecretLevelTrigger::CSecretLevelTrigger() {}

CTileTrigger::~CTileTrigger() {}
RVA(0x011160, 0x4b)
CTileTrigger::CTileTrigger() {}

CGruntHealthSprite::~CGruntHealthSprite() {}
RVA(0x011ef0, 0x4b)
CGruntHealthSprite::CGruntHealthSprite() {}

CPathHazard::~CPathHazard() {}
RVA(0x013170, 0x7b)
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
RVA(0x013470, 0x4b)
CVoiceTrigger::CVoiceTrigger() {}

// --- CTeleporter (0x041020), vptr 0x5e80cc ---
CTeleporter::~CTeleporter() {}
RVA(0x041020, 0x170)
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
CSecretTeleporterTrigger::~CSecretTeleporterTrigger() {}
RVA(0x041e90, 0x1ac)
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

// --- CSecretLevelTrigger 1-arg (0x0424b0), vptr 0x5e8804 ---
RVA(0x0424b0, 0x1a0)
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

// --- CParticlez (0x046ad0), vptr 0x5e7614 ---
CParticlez::~CParticlez() {}
RVA(0x046ad0, 0x15e)
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
RVA(0x07e3e0, 0x178)
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
RVA(0x07eb00, 0x170)
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
RVA(0x07f350, 0x16a)
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
RVA(0x07fdb0, 0x166)
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
RVA(0x0aad20, 0x15c)
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
RVA(0x0ab310, 0x18d)
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
RVA(0x0ab940, 0x1b8)
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
RVA(0x0abfa0, 0x1b6)
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
RVA(0x0ac3f0, 0x1b1)
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
RVA(0x0ac620, 0x1cf)
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
RVA(0x0acf40, 0x16e)
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
RVA(0x0ad540, 0x1f0)
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
RVA(0x0adbe0, 0x178)
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
RVA(0x0ae7f0, 0x13d)
CSingleAnimation::CSingleAnimation(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_08 |= 2;
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
}

// --- CWarpStonePad (0x10d650), vptr 0x5e71ac ---
CWarpStonePad::~CWarpStonePad() {}
RVA(0x10d650, 0x16c)
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
RVA(0x10dc40, 0x154)
CTileTriggerSwitch::CTileTriggerSwitch(CGameObject* obj) : CUserLogic(obj) {
    m_30 = m_14->m_1c;
    m_14->m_1c = g_buteTree.Find("A");
    m_38->m_08 |= 2;
    m_38->m_08 |= 1;
    m_38->m_40 |= 1;
}

// --- CTileTrigger 1-arg (0x10e220), vptr 0x5e7f14 ---
RVA(0x10e220, 0x17d)
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

// --- CTileTriggerTransition (0x10faf0), vptr 0x5e7db4 ---
CTileTriggerTransition::~CTileTriggerTransition() {}
RVA(0x10faf0, 0x128)
CTileTriggerTransition::CTileTriggerTransition(CGameObject* obj) : CUserLogic(obj) {
    m_38->m_08 |= 0x1000000;
    if (m_10->m_74 != 0) {
        m_10->m_74 = 0;
        m_10->m_08 |= 0x20000;
    }
}

// --- CToobSpikez (0x1145c0), vptr 0x5e7774 ---
CToobSpikez::~CToobSpikez() {}
RVA(0x1145c0, 0x18e)
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









