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
#include <Gruntz/UserLogic.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// CButeTree - the engine bute store the tile-logic tails query for their "A"
// node. g_buteTree (0x6bf620) is the global instance; Find (0x16d190) is matched
// in src/Stub/CButeTree.cpp. Modeled minimally so `g_buteTree.Find("A")`
// reloc-masks.
// ---------------------------------------------------------------------------
class CButeTree {
public:
    void* Find(const char* key);
};
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
    virtual ~CGruntHealthSprite() OVERRIDE;
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
    char m_pad00[0x7c];
    WwdGameRegAux* m_7c; // +0x7c
    char m_pad80[0x118 - 0x80];
    int m_118; // +0x118
    char m_pad11c[0x130 - 0x11c];
    int m_130; // +0x130
    int m_134; // +0x134
};
extern WwdGameReg* g_gameReg;

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
