#include "../rva.h"
// LogicTypeTable.cpp - CLogicTypeBuilder::BuildLogicTypeTable @0x8a40 (200 B,
// __thiscall ret 4). A one-shot registrar that ensures three built-in tile-logic
// types ("LogicHit", "LogicAttack", "LogicBump") are present in the engine's
// logic-type registry, lazily registering each (with its factory callback) only
// if a lookup for that key comes back empty (C:\Proj\Gruntz).
//
// For each of the three keys it:
//   1. looks the key up in the registry's string-keyed table (the matched lookup
//      helper CLogicRegistry::Lookup @0x1b8008, __thiscall, external), addressed
//      at (this->m_c->m_14)+0x10;
//   2. if the lookup result is null, calls the registry's virtual registrar
//      (vtable slot +0x24) with (factoryFn, key, 2) to install the type.
//
// Only offsets / code bytes are load-bearing; names are placeholders.
//
// BYTE-EXACT body modulo the same MSVC5 scheduling coin-flip as spriteloaders:
// the target SINKS each block's lookup out-param zero-init store (`mov [&found],0`)
// past the lea/push of &found; our cl HOISTS it before - the same instructions,
// permuted (byte-content identical). See config/units.toml. Kept wip, not
// strict-exact.

// ---------------------------------------------------------------------------
// The three built-in logic-type factory callbacks. These are real engine .text
// routines we are NOT matching here; declared as external no-body functions so
// that pushing their address emits a DIR32 relocation (reloc-masked in objdiff,
// like the target's LAB_0056e4c0/d0/e0 .text data references).
// ---------------------------------------------------------------------------
extern "C" {
void LogicHitFactory();     // @0x16e4c0  (target LAB_0056e4c0)
void LogicAttackFactory();  // @0x16e4d0  (target LAB_0056e4d0)
void LogicBumpFactory();    // @0x16e4e0  (target LAB_0056e4e0)
}

// ---------------------------------------------------------------------------
// The logic-type registry: its vtable (slot +0x24 = the registrar) lives at +0,
// and a string-keyed lookup sub-object (CLogicMap, used by Lookup) is embedded at
// +0x10. Modeled minimally so the `ecx=<registry+0x10>; call 0x1b8008` and
// `call [vtbl+0x24]` shapes reloc-mask.
// ---------------------------------------------------------------------------
struct CLogicType;
class CLogicMap {
public:
    int Lookup(char *szKey, CLogicType **ppOut);   // @0x1b8008 (__thiscall ret 8)
};
class CLogicRegistry {
public:
    // slot +0x24: install (factoryFn, key, flags) for a not-yet-present type.
    virtual void m_00();                            // +0x00
    virtual void m_04();                            // +0x04
    virtual void m_08();                            // +0x08
    virtual void m_0c();                            // +0x0c
    virtual void m_10();                            // +0x10
    virtual void m_14();                            // +0x14
    virtual void m_18();                            // +0x18
    virtual void m_1c();                            // +0x1c
    virtual void m_20();                            // +0x20
    virtual void RegisterType(void *factoryFn, char *szKey, int flags); // +0x24

    char      m_pad04[0x10 - 4];
    CLogicMap m_10map;                              // +0x10  lookup sub-object
};

// The intermediate object reached through this->m_c: its +0x14 slot points at the
// logic-type registry (the Lookup map is at registry+0x10; the registrar is the
// registry's own virtual).
struct CLogicCtx {
    char            m_pad00[0x14];
    CLogicRegistry *m_14;       // +0x14  the registry (pointer)
};

struct CLogicTypeBuilder {
    char       m_pad00[0xc];
    CLogicCtx *m_c;                     // +0xc
};

// ---------------------------------------------------------------------------
// BuildLogicTypeTable  @0x8a40  (__stdcall, ret 4 - the builder object is the
// single explicit stack arg, read into ESI; not a __thiscall).
//
// Each block re-reads obj->m_c->m_14 (the registry pointer) for BOTH the lookup
// and the register call - the target does NOT cache it across the two (it reloads
// `mov edx,[esi+0xc]; mov ecx,[edx+0x14]` at each site), so the lookup expression
// is repeated rather than hoisted into a local.
RVA(0x8a40, 0xc8)
void __stdcall BuildLogicTypeTable(CLogicTypeBuilder *obj)
{
    {
        CLogicType *found = 0;
        obj->m_c->m_14->m_10map.Lookup("LogicHit", &found);
        if (!found)
            obj->m_c->m_14->RegisterType((void *)LogicHitFactory, "LogicHit", 2);
    }
    {
        CLogicType *found = 0;
        obj->m_c->m_14->m_10map.Lookup("LogicAttack", &found);
        if (!found)
            obj->m_c->m_14->RegisterType((void *)LogicAttackFactory, "LogicAttack", 2);
    }
    {
        CLogicType *found = 0;
        obj->m_c->m_14->m_10map.Lookup("LogicBump", &found);
        if (!found)
            obj->m_c->m_14->RegisterType((void *)LogicBumpFactory, "LogicBump", 2);
    }
}
