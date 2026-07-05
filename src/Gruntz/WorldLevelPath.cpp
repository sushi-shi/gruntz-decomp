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

#include <Bute/SymTab.h>       // the shared CSymTab (ResolveQualified 0x13be40)
#include <Gruntz/GameLevel.h>  // canonical CGameLevel (real virtual slots 15/16/17 + non-virtuals)

// The loaded level record IS the canonical CGameLevel (<Gruntz/GameLevel.h>): this TU
// dispatches slot 15 LoadFromSource (+0x3c), slot 16 LoadFromFile (+0x40) and slot 17
// ReleaseChildren (+0x44) through the real vtable, and calls the non-virtual
// NotifyAllPlanes (0x160f40) / FindPlaneByName (0x15dde0); the dirty-flag word is
// CLoadable's m_08. `node` is the resolved parse-source record (CParseSource*).
class CParseSource;

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
    m_0c->m_24->ReleaseChildren();
    if (m_4->m_c8.GetLength() != 0) {
        if (m_4->m_128 != 0) {
            CString key = "BATTLEZ\\" + m_4->QueryLevelName();
            i32 node = m_34->ResolveQualified(key, (void*)0x575744);
            if (node == 0) {
                return 0;
            }
            if (m_0c->m_24->LoadFromSource((CParseSource*)node) == 0) {
                return 0;
            }
        } else if (m_4->m_12c != 0) {
            CString key = "MULTI\\" + m_4->QueryLevelName();
            i32 node = m_34->ResolveQualified(key, (void*)0x575744);
            if (node == 0) {
                return 0;
            }
            if (m_0c->m_24->LoadFromSource((CParseSource*)node) == 0) {
                return 0;
            }
        } else {
            if (m_0c->m_24->LoadFromFile(m_4->QueryLevelName()) == 0) {
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
        i32 node = m_28->ResolveQualified(key, (void*)0x575744);
        if (node == 0) {
            return 0;
        }
        if (m_0c->m_24->LoadFromSource((CParseSource*)node) == 0) {
            return 0;
        }
    }
    m_0c->m_24->NotifyAllPlanes();
    m_0c->m_24->m_08 |= 4;
    g_backView = (ScrollView*)m_0c->m_24->FindPlaneByName("BACK");
    return 1;
}
