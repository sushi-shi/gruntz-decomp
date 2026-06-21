// UserLogic.h - Gruntz game-object base hierarchy (C:\Proj\Gruntz).
//
// Reconstruction sufficient to byte-match the small game-object constructors.
// Field names are placeholders; the OFFSETS and the inheritance chain are the
// load-bearing facts the matches prove.
//
// Hierarchy (recovered from RTTI ClassHierarchyDescriptors in GRUNTZ.EXE):
//     CUserBase                       vftable 0x5e70b4  (3 virtuals)
//       +-- CUserLogic : CUserBase    vftable 0x5e705c  (12 virtuals)
//             +-- CSecretLevelTrigger, CTileTrigger, CGruntHealthSprite,
//                 CVoiceTrigger, CPathHazard, ...   (the game-object leaves;
//                 each also has CWapX as a second base at +0x34)
//
// Every leaf ctor we match follows the identical MSVC5 /GX schedule:
//   push EH frame; store CUserBase vptr (0x5e70b4) at [this+0]; construct the
//   embedded member subobject at [this+0x18] via the out-of-line ctor 0x16d710
//   (it can throw -> the EH state machine is active); then store the leaf's own
//   most-derived vftable at [this+0]; then init the leaf's own members.
//
// The /GX EH frame is emitted because TWO sub-objects with destructors are
// constructed: CUserBase's own bookkeeping field (m_aux, an inline-ctor'd member
// whose construction folds to no code) AND the +0x18 member whose ctor 0x16d710
// can throw. With only one destructible sub-object MSVC emits no cleanup frame;
// the second forces it (measured). The intermediate CUserLogic vptr store is
// dead-eliminated (nothing observes it between the CUserBase store and the
// most-derived store), so CUserBase's and CUserLogic's ctors are INLINE (folded
// into each leaf ctor; MSVC 5.0 only folds a base ctor it can see inline).
#ifndef GRUNTZ_USERLOGIC_H
#define GRUNTZ_USERLOGIC_H

#include <rva.h>

// ---------------------------------------------------------------------------
// CUserBaseLink - the object embedded at CUserBase+0x18.
//
// Its ctor (0x16d710) is the one out-of-line constructor the whole game-object
// family chains. It is a non-polymorphic registrant (stores a plain pointer at
// +0, not an RTTI vftable) that does global singleton bookkeeping; the ctor can
// throw, which is why the leaf ctors carry the /GX EH frame. Declared with an
// out-of-line ctor + dtor so MSVC emits the `lea ecx,[+0x18]; call` and keeps
// the EH state machine live.
// ---------------------------------------------------------------------------
struct CUserBaseLink {
    CUserBaseLink();          // 0x16d710 (out-of-line; can throw)
    ~CUserBaseLink();         // out-of-line dtor (cleaned up on unwind)
    void *m_0;                // +0x00  (the 0x5f04c8 pointer the ctor stores)
    char  m_pad4[0x20 - 4];   // +0x04..+0x1f  -> the member spans 0x18..0x37
};

// ---------------------------------------------------------------------------
// CUserBaseAux - a small destructible bookkeeping member of CUserBase placed
// before the +0x18 link. Its inline ctor folds to no code, but having a second
// destructible sub-object is what forces MSVC to emit the /GX cleanup frame in
// every leaf ctor (the +0x18 link's throwing ctor must unwind it). Placeholder
// shape; the load-bearing fact is "a destructible CUserBase field exists below
// +0x18".
// ---------------------------------------------------------------------------
struct CUserBaseAux {
    CUserBaseAux() {}         // inline empty (no ctor call emitted)
    ~CUserBaseAux();          // out-of-line dtor
    int m_0;
};

// ---------------------------------------------------------------------------
// CUserBase - root of the game-object hierarchy.
// vptr@0, 3 virtuals. Inline ctor so it folds into derived ctors.
// ---------------------------------------------------------------------------
class CUserBase {
public:
    CUserBase() {}            // inline: vptr store + member ctors emitted at use
    virtual ~CUserBase();     // slot 0
    virtual int UserBaseVfunc1();   // slot 1
    virtual int UserBaseVfunc2();   // slot 2

    CUserBaseAux  m_aux;      // +0x04  (destructible; forces the EH frame)
    char          m_pad8[0x18 - 0x08];
    CUserBaseLink m_link;     // +0x18..+0x37 (ctor 0x16d710, can throw)
};

// ---------------------------------------------------------------------------
// CUserLogic : CUserBase - 12 virtuals (vftable 0x5e705c). Inline empty ctor:
// the leaf ctors show no CUserLogic-specific member inits before their own.
// ---------------------------------------------------------------------------
class CUserLogic : public CUserBase {
public:
    CUserLogic() {}
    virtual ~CUserLogic() OVERRIDE;          // slot 0 (most-derived dtor)
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
};

#endif // GRUNTZ_USERLOGIC_H
