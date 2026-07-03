// WorldLevelPath.cpp - resolve the world's active level record by mode
// (RVA 0xdbc80, /GX). The compound sibling of CWorldState::BuildWorldLevelKey
// (0x3c0e0): resets the level record, then depending on the world's name/mode
// selects a namespace key ("BATTLEZ\<name>", "MULTI\<name>", "WORLDZ\LEVEL%i",
// "WORLDZ\TRAINING%i", or the raw level name) and resolves it. On success runs the
// record's load hook + NotifyAllPlanes, raises its dirty bit, and caches the "BACK"
// plane. Field names are placeholders; only offsets + code bytes are load-bearing.
#include <Mfc.h> // real MFC CString (ctor(const char*) 0x1b9d4c / dtor 0x1b9cde /
                 //   default ctor 0x1b9b93 / operator+(LPCTSTR,const CString&) 0x1b9ff5)
#include <rva.h>

#include <Ints.h>
#include <Globals.h>

class CSymTab {
public:
    i32 ResolveQualified(const char* name, i32 tag); // 0x13be40
};

// The loaded level record (CGameLevel): Reset at vtable +0x44, the namespace-load
// hook at +0x3c, a by-name load hook at +0x40, a dirty-flags word at +0x08, and the
// non-virtual NotifyAllPlanes / FindPlaneByName.
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
    virtual i32 LoadHook(i32 node);                   // +0x3c
    virtual i32 LoadHookByName(const char* name);     // +0x40
    virtual void Reset();                             // +0x44
    void NotifyAllPlanes();                           // 0x160f40
    struct CPlane* FindPlaneByName(const char* name); // 0x15dde0
    char m_pad04[0x8 - 0x4];
    i32 m_8; // +0x08 dirty flags
};

struct LevelMgr {
    char m_pad00[0x24];
    CGameLevel* m_24; // +0x24
};

// The world/game-registry object at +0x4: its rez-path name query (returns a CString
// by value), the current world-name string, and the two mode gates.
SIZE_UNKNOWN(CWorldObj);
class CWorldObj {
public:
    CString QueryLevelName(); // 0x4928c0 (via ILT 0x2531; __thiscall, returns by value)
    char m_pad00[0xc8];
    CString m_c8; // +0xc8 current world name
    char m_padcc[0x128 - 0xcc];
    i32 m_128; // +0x128 BATTLEZ mode gate
    i32 m_12c; // +0x12c MULTI mode gate
};

class CWorldState {
public:
    i32 BuildWorldLevelPath(i32 unused);
    char m_pad00[0x4];
    CWorldObj* m_4; // +0x04
    char m_pad08[0xc - 0x8];
    LevelMgr* m_0c; // +0x0c
    char m_pad10[0x1c - 0x10];
    i32 m_1c; // +0x1c level number
    char m_pad20[0x28 - 0x20];
    CSymTab* m_28; // +0x28 level-record symbol table
    char m_pad2c[0x34 - 0x2c];
    CSymTab* m_34; // +0x34 battlez/multi symbol table
};

// The "BACK" plane cache (reloc-masked DIR32 store).
struct ScrollView;
// The alternate-level-set gate (second run of levels). Single DATA binding here.

RVA(0x000dbc80, 0x309)
i32 CWorldState::BuildWorldLevelPath(i32 unused) {
    m_0c->m_24->Reset();
    if (m_4->m_c8.GetLength() != 0) {
        if (m_4->m_128 != 0) {
            CString key = "BATTLEZ\\" + m_4->QueryLevelName();
            i32 node = m_34->ResolveQualified(key, 0x575744);
            if (node == 0) {
                return 0;
            }
            if (m_0c->m_24->LoadHook(node) == 0) {
                return 0;
            }
        } else if (m_4->m_12c != 0) {
            CString key = "MULTI\\" + m_4->QueryLevelName();
            i32 node = m_34->ResolveQualified(key, 0x575744);
            if (node == 0) {
                return 0;
            }
            if (m_0c->m_24->LoadHook(node) == 0) {
                return 0;
            }
        } else {
            if (m_0c->m_24->LoadHookByName(m_4->QueryLevelName()) == 0) {
                return 0;
            }
        }
    } else {
        CString key;
        i32 sel = m_1c;
        if (g_6455f0 != 0) {
            sel += 0x64;
        }
        if (sel > 0x24 && sel <= 0x28) {
            key.Format("WORLDZ\\TRAINING%i", sel % 0x24);
        } else {
            key.Format("WORLDZ\\LEVEL%i", sel);
        }
        i32 node = m_28->ResolveQualified(key, 0x575744);
        if (node == 0) {
            return 0;
        }
        if (m_0c->m_24->LoadHook(node) == 0) {
            return 0;
        }
    }
    m_0c->m_24->NotifyAllPlanes();
    m_0c->m_24->m_8 |= 4;
    g_backView = (ScrollView*)m_0c->m_24->FindPlaneByName("BACK");
    return 1;
}
