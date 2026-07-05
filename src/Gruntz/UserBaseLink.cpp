// UserBaseLink.cpp - the out-of-line engine routines the inline game-object
// ctors chain (see include/Gruntz/UserLogic.h). Kept in their own TU so the leaf
// ctor TUs see only declarations (and assume the link ctor can throw - that is
// what makes MSVC emit the /GX EH frame). NOT MATCHED here: the RVA()/DATA()
// pins pin the symbols so the leaf ctors' calls/loads reloc-resolve on both base
// and target sides.
#include <Gruntz/UserLogic.h>
#include <rva.h>

// The +0x18 link: an EngStr name, ctor at 0x16d710 (can throw).
RVA(0x0016d710, 0x76)
CUserBaseLink::CUserBaseLink() {}

// EngStr - the engine string class (incs CString clone). Bodies live in the
// engine string TU; pinned here so the ctors' temp-construct/assign/destruct
// sequence reloc-masks.
RVA(0x0016d3a0, 0x344)
EngStr::EngStr(const char*, i32) {}
RVA(0x0016d2f0, 0xac)
EngStr& EngStr::operator=(const EngStr&) {
    return *this;
}
RVA(0x0016d2a0, 0x26)
EngStr::~EngStr() {}

// operator new the engine uses (0x1b9b46, __cdecl); reloc-masked rel32.
extern "C" void* RezAlloc(u32 n); // 0x1b9b46

// Stamp the foreign worker vtable (0x5efb80) by address - the vptr store the
// inline worker ctor emits; never a C++ ctor (that would emit a divergent vtable).
static inline void StampWorkerVtbl(CAnimWorker* w) {
    *(void**)w = &g_animWorkerVtbl;
}

// CGameObject::EnsureWorker80 (0x150eb0): the +0x80 worker variant of
// EnsureWorker88/90 - same lazy build/reuse/feed, but it RETURNS the slot-9 result
// (or 0 on the null guards). Called by AddLogicHit (0x150f50).
// @early-stop
// Expected to share the zero-register-pinning wall of EnsureWorker88/90 (this/0 in
// esi<->edi). Logic byte-exact; a pure allocator coin-flip, not source-steerable.
RVA(0x00150eb0, 0x98)
i32 CGameObject::EnsureWorker80(CGameObject* src) {
    if (src == 0) {
        return 0;
    }
    if (m_80 != 0) {
        m_80->Slot07();
    } else {
        CAnimWorker* w = (CAnimWorker*)RezAlloc(0x17c);
        if (w != 0) {
            w->m_04 = m_04;
            w->m_08 = 0;
            w->m_0c = m_0c;
            StampWorkerVtbl(w);
            w->m_collideNotify = 0;
            w->m_14 = 0;
            w->m_18 = 0;
            w->m_170 = 0;
            w->m_1c = 0;
            w->m_174 = 0;
            w->m_178 = 0;
        } else {
            w = 0;
        }
        m_80 = w;
    }
    if (m_80 == 0) {
        return 0;
    }
    return m_80->Slot09(src->m_10, 0);
}

// CGameObject::EnsureWorker88 (0x150f90): lazily build the +0x88 worker - if one
// already exists, just re-run its slot-7 reuse hook; otherwise operator new a
// fresh 0x17c-byte worker (seeded m_04=this->m_4, m_08=0, m_0c=this->m_c, the
// foreign vtable, all other fields 0), stow it at +0x88, then feed src->m_10
// through slot 9. The worker is built MANUALLY (the inline ctor), not via a C++
// ctor, so the vptr store stamps the retail 0x5efb80 by address.
// @early-stop
// zero-register-pinning wall (docs/patterns/zero-register-pinning.md): the whole
// build sequence + both dispatches are byte-identical, but retail pins this->edi
// and 0->esi while cl pins this->esi and 0->edi, and lowers the `arg==0` guard as
// an early `xor eax,eax;ret` block where cl shares the epilogue - the swap cascades
// every esi/edi. Logic exact; a pure allocator coin-flip, not source-steerable.
RVA(0x00150f90, 0x98)
void CGameObject::EnsureWorker88(CGameObject* src) {
    if (src == 0) {
        return;
    }
    if (m_88 != 0) {
        m_88->Slot07();
    } else {
        CAnimWorker* w = (CAnimWorker*)RezAlloc(0x17c);
        if (w != 0) {
            w->m_04 = m_04;
            w->m_08 = 0;
            w->m_0c = m_0c;
            StampWorkerVtbl(w);
            w->m_collideNotify = 0;
            w->m_14 = 0;
            w->m_18 = 0;
            w->m_170 = 0;
            w->m_1c = 0;
            w->m_174 = 0;
            w->m_178 = 0;
        } else {
            w = 0;
        }
        m_88 = w;
    }
    if (m_88 == 0) {
        return;
    }
    m_88->Slot09(src->m_10, 0);
}

// CGameObject::EnsureWorker90 (0x151070): identical to EnsureWorker88 but for the
// +0x90 worker slot.
// @early-stop
// same zero-register-pinning wall as EnsureWorker88 (this/0 in esi<->edi).
RVA(0x00151070, 0x98)
void CGameObject::EnsureWorker90(CGameObject* src) {
    if (src == 0) {
        return;
    }
    if (m_collideWorker != 0) {
        m_collideWorker->Slot07();
    } else {
        CAnimWorker* w = (CAnimWorker*)RezAlloc(0x17c);
        if (w != 0) {
            w->m_04 = m_04;
            w->m_08 = 0;
            w->m_0c = m_0c;
            StampWorkerVtbl(w);
            w->m_collideNotify = 0;
            w->m_14 = 0;
            w->m_18 = 0;
            w->m_170 = 0;
            w->m_1c = 0;
            w->m_174 = 0;
            w->m_178 = 0;
        } else {
            w = 0;
        }
        m_collideWorker = w;
    }
    if (m_collideWorker == 0) {
        return;
    }
    m_collideWorker->Slot09(src->m_10, 0);
}

// CGameObject's three built-in logic-handler registrars: look the logic-name key
// up in the world's CMapStringToOb (m_0c -> +0x14 -> +0x10), then feed the found
// handler through the matching lazy worker slot (Hit -> 80, Attack -> 88, Bump -> 90).
// m_0c is the family's generically-typed world/context slot (i32); reached by
// documented offset here.
// @early-stop
// scheduling coin-flip: body byte-exact EXCEPT the `handler = 0` slot-init lands one
// push early (push &out; STORE; push key) where retail schedules it after both pushes
// (push &out; push key; STORE). Same slot, independent store; MSVC5's scheduler places
// it between the arg pushes. No source ordering of the init reproduces the late slot.
RVA(0x00150f50, 0x33)
void CGameObject::AddLogicHit(char* key) {
    CGameObject* handler = 0;
    CLogicHandlerMap* map = LogicMap();
    map->Lookup(key, &handler);
    EnsureWorker80(handler);
}

// @early-stop
// same `handler = 0` scheduling coin-flip as AddLogicHit.
RVA(0x00151030, 0x33)
void CGameObject::AddLogicAttack(char* key) {
    CGameObject* handler = 0;
    CLogicHandlerMap* map = LogicMap();
    map->Lookup(key, &handler);
    EnsureWorker88(handler);
}

// @early-stop
// same `handler = 0` scheduling coin-flip as AddLogicHit.
RVA(0x00151110, 0x33)
void CGameObject::AddLogicBump(char* key) {
    CGameObject* handler = 0;
    CLogicHandlerMap* map = LogicMap();
    map->Lookup(key, &handler);
    EnsureWorker90(handler);
}

// g_logicTypesRegistered (RVA 0x2bf674, VA 0x6bf674): the one-shot logic-type
// guard. g_emptyString (RVA 0x2293f4) is already labeled by netmgrerror, so it
// is only DECLARED in the header, never re-defined here.
DATA(0x002bf674)
i32 g_logicTypesRegistered;
