// LogicTypeTableInline.h - the INLINE (Find-based) body of
// CUserLogic::BuildLogicTypeTable, for the tile-logic leaf ctors whose TU inlined
// the built-in logic-type registration (the "unrolled" prologue: a 3-way
// Find/RegisterType block) rather than calling the out-of-line 0x8a40
// (Lookup-based) helper the other leaves chain.
//
// Each block re-reads ctx->m_0c->m_14 (the logic-type registry) for BOTH the Find
// and the RegisterType call - the registry's own Find (0x1703 thunk, __thiscall,
// returns the found type or 0) and its virtual registrar (vtable slot +0x24).
// Including this header BEFORE a leaf ctor makes MSVC inline the block into the
// folded CUserLogic(obj) prologue. Field names are placeholders; only offsets +
// code bytes are load-bearing.
#ifndef GRUNTZ_LOGICTYPETABLEINLINE_H
#define GRUNTZ_LOGICTYPETABLEINLINE_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

// The three built-in logic-type factory callbacks (real engine .text routines we
// do not match here); declared no-body so pushing their address emits the DIR32
// relocation that reloc-masks against retail's LAB_0056e4c0/d0/e0.
extern "C" {
    void LogicHitFactory();    // 0x56e4c0
    void LogicAttackFactory(); // 0x56e4d0
    void LogicBumpFactory();   // 0x56e4e0
}

// The logic-type registry reached through ctx->m_0c->m_14: its own Find (the
// 0x1703 thunk) returns the found type (0 == absent); its virtual registrar lives
// at vtable slot +0x24.
class CLogicTypeReg {
public:
    virtual void v00(); // +0x00
    virtual void v04(); // +0x04
    virtual void v08(); // +0x08
    virtual void v0c(); // +0x0c
    virtual void v10(); // +0x10
    virtual void v14(); // +0x14
    virtual void v18(); // +0x18
    virtual void v1c(); // +0x1c
    virtual void v20(); // +0x20
    virtual void RegisterType(void* factoryFn, const char* key, i32 flags); // +0x24

    i32 Find(const char* key); // 0x1703 thunk (__thiscall, returns found type or 0)
};

// The intermediate object reached through ctx->m_0c: its +0x14 slot points at the
// logic-type registry.
struct CLogicTypeCtx {
    char m_pad00[0x14];
    CLogicTypeReg* m_14; // +0x14  the registry
};

// CLogicTypeBuilder (forward-declared in <Gruntz/UserLogic.h>): the ctor passes
// (CLogicTypeBuilder*)m_0c, i.e. the bound CGameObject; +0xc reaches the ctx.
struct CLogicTypeBuilder {
    char m_pad00[0xc];
    CLogicTypeCtx* m_0c; // +0xc
};

inline void CUserLogic::BuildLogicTypeTable(CLogicTypeBuilder* ctx) {
    if (!ctx->m_0c->m_14->Find("LogicHit")) {
        ctx->m_0c->m_14->RegisterType((void*)LogicHitFactory, "LogicHit", 2);
    }
    if (!ctx->m_0c->m_14->Find("LogicAttack")) {
        ctx->m_0c->m_14->RegisterType((void*)LogicAttackFactory, "LogicAttack", 2);
    }
    if (!ctx->m_0c->m_14->Find("LogicBump")) {
        ctx->m_0c->m_14->RegisterType((void*)LogicBumpFactory, "LogicBump", 2);
    }
}

#endif // GRUNTZ_LOGICTYPETABLEINLINE_H
