#ifndef GRUNTZ_CDONOTHINGNORMALDTOR_H
#define GRUNTZ_CDONOTHINGNORMALDTOR_H

#include <rva.h>
#include <Gruntz/UserLogic.h> // CUserLogic base (CDoNothingNormal : CUserLogic)

class CDoNothingNormal : public CUserLogic, public CWapX {
public:
    CDoNothingNormal() {} // default (the vtable-emitting Realize site + dtor TU)
    // The worker pump's case-0 leaf ctor (@0xa9e00): OUT-OF-LINE CUserLogic(owner)
    // base call (the pump TU sets USERLOGIC_OOL_CTOR) + CWapX(owner) - the very
    // m_34/m_38/m_3c = owner/owner/owner->m_7c triple the ex-DnnRec scaffold
    // reinvented - then raise the owner's bit0.
    CDoNothingNormal(CGameObject* owner) : CUserLogic(owner), CWapX(owner) {
        owner->m_flags |= 1;
    }
    virtual i32 SerializeMove(CGruntArchive*, i32, i32, i32) OVERRIDE; // slot 1
    RVA(0x0000f7e0, 0x6)
    virtual LogicTypeId GetTypeTag() OVERRIDE {
        return LOGIC_DONOTHINGNORMAL;
    } // slot 2
public:
    // NO user-declared dtor: retail's is COMPILER-GENERATED (implicit
    // elides the leaf-vptr restamp; @rva-symbol pin in the home TU).
};
SIZE(0x54); // new-site push 0x54: CUserLogic 0x34 + CWapX 0x20 EXACTLY
VTBL(CDoNothingNormal, 0x1e859c);

#endif // GRUNTZ_CDONOTHINGNORMALDTOR_H
