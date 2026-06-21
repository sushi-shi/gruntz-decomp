// UserLogic.cpp - Gruntz game-object base hierarchy ctors (C:\Proj\Gruntz).
//
// Reconstructs the CUserBase / CUserLogic base classes and the small leaf
// game-object constructors that fold them. See include/Gruntz/UserLogic.h for
// the hierarchy and the shared /GX ctor schedule.
//
// The one out-of-line ctor the whole family chains is CUserBaseLink::CUserBaseLink
// (0x16d710), the member embedded at CUserBase+0x18. It is defined in
// src/Gruntz/UserBaseLink.cpp.
#include <Gruntz/UserLogic.h>
#include <rva.h>

// ---------------------------------------------------------------------------
// Out-of-line vtable anchors. These give each class a real vftable in this TU so
// the inline ctors emit the right vptr stores. Bodies are not matched.
CUserBase::~CUserBase() {}
int CUserBase::UserBaseVfunc1() { return 0; }
int CUserBase::UserBaseVfunc2() { return 0; }

CUserLogic::~CUserLogic() {}
int CUserLogic::UserLogicVfunc1() { return 0; }
int CUserLogic::UserLogicVfunc2() { return 0; }
int CUserLogic::UserLogicVfunc3() { return 0; }
int CUserLogic::UserLogicVfunc4() { return 0; }
int CUserLogic::UserLogicVfunc5() { return 0; }
int CUserLogic::UserLogicVfunc6() { return 0; }
int CUserLogic::UserLogicVfunc7() { return 0; }
int CUserLogic::UserLogicVfunc8() { return 0; }
int CUserLogic::UserLogicVfunc9() { return 0; }
int CUserLogic::UserLogicVfuncA() { return 0; }
int CUserLogic::UserLogicVfuncB() { return 0; }

// ---------------------------------------------------------------------------
// Leaf game-object constructors.
//
// Each is `: CUserLogic` with no extra member inits beyond the folded base
// schedule (store CUserBase vptr, construct the +0x18 member via 0x16d710,
// store the leaf's own most-derived vptr). These leaf classes add no data the
// 75-byte ctor touches, so the bodies are empty; the leaf's own vftable is
// anchored by an out-of-line dtor stub.
//
// Each is a separate class with its own RTTI-confirmed vftable.
// ---------------------------------------------------------------------------

class CSecretLevelTrigger : public CUserLogic {
public: CSecretLevelTrigger(); virtual ~CSecretLevelTrigger() OVERRIDE;
};
CSecretLevelTrigger::~CSecretLevelTrigger() {}
RVA(0x010b20, 0x4b)
CSecretLevelTrigger::CSecretLevelTrigger() {}

class CTileTrigger : public CUserLogic {
public: CTileTrigger(); virtual ~CTileTrigger() OVERRIDE;
};
CTileTrigger::~CTileTrigger() {}
RVA(0x011160, 0x4b)
CTileTrigger::CTileTrigger() {}

class CGruntHealthSprite : public CUserLogic {
public: CGruntHealthSprite(); virtual ~CGruntHealthSprite() OVERRIDE;
};
CGruntHealthSprite::~CGruntHealthSprite() {}
RVA(0x011ef0, 0x4b)
CGruntHealthSprite::CGruntHealthSprite() {}

class CVoiceTrigger : public CUserLogic {
public: CVoiceTrigger(); virtual ~CVoiceTrigger() OVERRIDE;
};
CVoiceTrigger::~CVoiceTrigger() {}
RVA(0x013470, 0x4b)
CVoiceTrigger::CVoiceTrigger() {}

// CPathHazard (0x13170, 123B): same folded base schedule, then zeroes its own
// eight pointer fields at +0x108..+0x12c (all reuse the xor edi,edi zero).
class CPathHazard : public CUserLogic {
public:
    CPathHazard();
    virtual ~CPathHazard() OVERRIDE;
    char m_pad[0x108 - 0x38]; // pad CUserLogic (ends +0x38) .. +0x107
    void *m_108;   // +0x108
    void *m_10c;   // +0x10c
    void *m_110;   // +0x110
    void *m_114;   // +0x114
    char m_pad118[0x120 - 0x118];
    void *m_120;   // +0x120
    void *m_124;   // +0x124
    void *m_128;   // +0x128
    void *m_12c;   // +0x12c
};
CPathHazard::~CPathHazard() {}
RVA(0x013170, 0x7b)
CPathHazard::CPathHazard()
{
    // Emitted store order (all 0): +0x108, +0x110, +0x10c, +0x114, then
    // +0x120, +0x128, +0x124, +0x12c.
    m_108 = 0;
    m_110 = 0;
    m_10c = 0;
    m_114 = 0;
    m_120 = 0;
    m_128 = 0;
    m_124 = 0;
    m_12c = 0;
}
