// UserBaseLink.cpp - the out-of-line engine routines the inline game-object
// ctors chain (see include/Gruntz/UserLogic.h). Kept in their own TU so the leaf
// ctor TUs see only declarations (and assume the link ctor can throw - that is
// what makes MSVC emit the /GX EH frame). NOT MATCHED here: the RVA()/DATA()
// pins pin the symbols so the leaf ctors' calls/loads reloc-resolve on both base
// and target sides.
#include <Gruntz/UserLogic.h>
#include <rva.h>

// The +0x18 link: an EngStr name, ctor at 0x16d710 (can throw).
RVA(0x16d710, 0x76)
CUserBaseLink::CUserBaseLink() {}
CUserBaseLink::~CUserBaseLink() {}

// EngStr - the engine string class (incs CString clone). Bodies live in the
// engine string TU; pinned here so the ctors' temp-construct/assign/destruct
// sequence reloc-masks.
RVA(0x16d3a0, 0x344)
EngStr::EngStr(const char*, int) {}
RVA(0x16d2f0, 0xac)
EngStr& EngStr::operator=(const EngStr&) {
    return *this;
}
RVA(0x16d2a0, 0x26)
EngStr::~EngStr() {}

// CGameObject's three built-in logic-handler registrars (engine .text). Pinned
// here so the ctors' `m_10->AddLogic*(key)` calls reloc-mask.
RVA(0x150f50, 0x33)
void CGameObject::AddLogicHit(char*) {}
RVA(0x151030, 0x33)
void CGameObject::AddLogicAttack(char*) {}
RVA(0x151110, 0x33)
void CGameObject::AddLogicBump(char*) {}

// g_logicTypesRegistered (RVA 0x2bf674, VA 0x6bf674): the one-shot logic-type
// guard. g_emptyString (RVA 0x2293f4) is already labeled by netmgrerror, so it
// is only DECLARED in the header, never re-defined here.
DATA(0x2bf674)
int g_logicTypesRegistered;
