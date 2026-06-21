// UserBaseLink.cpp - CUserBaseLink::CUserBaseLink (0x16d710), the one out-of-line
// member ctor the whole game-object family chains (the member embedded at
// CUserBase+0x18). See include/Gruntz/UserLogic.h.
//
// Kept in its own TU so the leaf-ctor TU (UserLogic.cpp) sees only the
// declaration and assumes this ctor can throw - that is what makes MSVC emit the
// /GX EH frame in the retail leaf ctors. NOT YET MATCHED: its body has 4
// unmatched callees + global singleton bookkeeping; the RVA() pins the symbol so
// the leaf ctors' `call 0x16d710` reloc resolves on both base and target sides.
#include <Gruntz/UserLogic.h>
#include <rva.h>

RVA(0x16d710, 0x76)
CUserBaseLink::CUserBaseLink() {}

CUserBaseLink::~CUserBaseLink() {}
CUserBaseAux::~CUserBaseAux() {}
