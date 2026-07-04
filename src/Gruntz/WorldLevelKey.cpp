// WorldLevelKey.cpp - resolve the WORLDZ\LEVEL%i record for the active world
// (RVA 0x3c0e0). Resets the level record, formats its namespace key, resolves it,
// and on success runs the record's load hook + NotifyAllPlanes and raises its
// dirty bit. Field names are placeholders; only offsets + code bytes are
// load-bearing.
#include <Mfc.h> // real MFC CString (default ctor 0x1b9b93 / dtor 0x1b9cde, reloc-masked)
#include <rva.h>

#include <Ints.h>

#include <Bute/SymTab.h> // the shared CSymTab (ResolveQualified 0x13be40)

// The engine sprintf-into-CString helper (cdecl).
void Format(CString* out, const char* fmt, ...); // 0x1b2cf5 (cdecl)

// The loaded level record (CGameLevel / CLoadable), viewed as a FOREIGN dispatch
// object here: the full class + its vtable live in the gamelevel TU; only the
// namespace-load hook (slot 15, +0x3c) and Reset (slot 17, +0x44) are dispatched from
// this TU. Honest model = a manual vptr into a typed vtable struct naming ONLY those
// two slots as 4-byte thiscall PMFs + char pad[], NO fake virtuals; m_vtbl sits at
// +0x00 exactly where the fake vptr did, so the object layout (m_8 @ +0x08) is
// byte-identical. NotifyAllPlanes is a non-virtual (0x160f40).
struct CGameLevelVtbl;
class CGameLevel {
public:
    CGameLevelVtbl* m_vtbl; // +0x00
    char m_pad04[0x8 - 0x4];
    i32 m_8;                    // +0x08 dirty flags
    i32 CallLoadHook(i32 node); // vtbl +0x3c
    void CallReset();           // vtbl +0x44
    void NotifyAllPlanes();     // 0x160f40 (non-virtual)
};
typedef i32 (CGameLevel::*GlLoadFn)(i32);
typedef void (CGameLevel::*GlResetFn)();
struct CGameLevelVtbl {
    char m_pad00[0x3c];
    GlLoadFn LoadHook; // +0x3c
    char m_pad40[0x44 - 0x40];
    GlResetFn Reset; // +0x44
};
SIZE_UNKNOWN(CGameLevelVtbl);
inline i32 CGameLevel::CallLoadHook(i32 node) {
    return (this->*(m_vtbl->LoadHook))(node);
}
inline void CGameLevel::CallReset() {
    (this->*(m_vtbl->Reset))();
}

SIZE_UNKNOWN(LevelMgr);
struct LevelMgr {
    char m_pad00[0x24];
    CGameLevel* m_24; // +0x24
};

SIZE_UNKNOWN(CWorldState);
class CWorldState {
public:
    i32 BuildWorldLevelKey(i32 unused);
    char m_pad00[0x0c];
    LevelMgr* m_0c; // +0x0c
    char m_pad10[0x28 - 0x10];
    CSymTab* m_28; // +0x28
};

RVA(0x0003c0e0, 0xfb)
i32 CWorldState::BuildWorldLevelKey(i32 unused) {
    m_0c->m_24->CallReset();
    CString key;
    Format(&key, "WORLDZ\\LEVEL%i", 1);
    i32 node = m_28->ResolveQualified(key, (void*)0x575744);
    if (node == 0) {
        return 0;
    }
    if (m_0c->m_24->CallLoadHook(node) == 0) {
        return 0;
    }
    m_0c->m_24->NotifyAllPlanes();
    m_0c->m_24->m_8 |= 4;
    return 1;
}
