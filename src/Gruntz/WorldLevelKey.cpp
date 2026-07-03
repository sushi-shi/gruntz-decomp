// WorldLevelKey.cpp - resolve the WORLDZ\LEVEL%i record for the active world
// (RVA 0x3c0e0). Resets the level record, formats its namespace key, resolves it,
// and on success runs the record's load hook + NotifyAllPlanes and raises its
// dirty bit. Field names are placeholders; only offsets + code bytes are
// load-bearing.
#include <Mfc.h> // real MFC CString (default ctor 0x1b9b93 / dtor 0x1b9cde, reloc-masked)
#include <rva.h>

#include <Ints.h>

// The engine sprintf-into-CString helper (cdecl).
void Format(CString* out, const char* fmt, ...); // 0x1b2cf5 (cdecl)

class CSymTab {
public:
    i32 ResolveQualified(const char* name, i32 tag); // 0x13be40
};

// The loaded level record (CGameLevel / CLoadable): Reset at vtable +0x44,
// the namespace-load hook at +0x3c, a dirty-flags word at +0x08, and the
// non-virtual NotifyAllPlanes.
class CGameLevel {
public:
    virtual void v00();
    virtual void v04();
    virtual void v08();
    virtual void v0c();
    virtual void v10();
    virtual void v14();
    virtual void v18();
    virtual void v1c();
    virtual void v20();
    virtual void v24();
    virtual void v28();
    virtual void v2c();
    virtual void v30();
    virtual void v34();
    virtual void v38();
    virtual i32 LoadHook(i32 node); // +0x3c
    virtual void v40();
    virtual void Reset();   // +0x44
    void NotifyAllPlanes(); // 0x160f40
    char m_pad04[0x8 - 0x4];
    i32 m_8; // +0x08 dirty flags
};

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
    m_0c->m_24->Reset();
    CString key;
    Format(&key, "WORLDZ\\LEVEL%i", 1);
    i32 node = m_28->ResolveQualified(key, 0x575744);
    if (node == 0) {
        return 0;
    }
    if (m_0c->m_24->LoadHook(node) == 0) {
        return 0;
    }
    m_0c->m_24->NotifyAllPlanes();
    m_0c->m_24->m_8 |= 4;
    return 1;
}
