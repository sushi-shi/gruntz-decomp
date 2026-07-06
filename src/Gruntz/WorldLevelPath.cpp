// WorldLevelPath.cpp - resolve the world's active level record by mode
// (RVA 0xdbc80, /GX). The compound sibling of CWorldState::BuildWorldLevelKey
// (0x3c0e0): resets the level record, then depending on the world's name/mode
// selects a namespace key ("BATTLEZ\<name>", "MULTI\<name>", "WORLDZ\LEVEL%i",
// "WORLDZ\TRAINING%i", or the raw level name) and resolves it. On success runs the
// record's load hook + NotifyAllPlanes, raises its dirty bit, and caches the "BACK"
// plane. Field names are placeholders; only offsets + code bytes are load-bearing.
#include <Mfc.h> // real MFC CString (ctor(const char*) 0x1b9d4c / dtor 0x1b9cde /
#include <Gruntz/GruntzMgr.h>
//   default ctor 0x1b9b93 / operator+(LPCTSTR,const CString&) 0x1b9ff5)
#include <rva.h>

#include <Ints.h>
#include <Globals.h>

#include <Bute/SymTab.h>       // the shared CSymTab (ResolveQualified 0x13be40)
#include <Gruntz/GameLevel.h>  // canonical CGameLevel (real virtual slots 15/16/17 + non-virtuals)
#include <Gruntz/WorldState.h> // canonical CWorldState + LevelMgr

// The loaded level record IS the canonical CGameLevel (<Gruntz/GameLevel.h>): this TU
// dispatches slot 15 LoadFromSource (+0x3c), slot 16 LoadFromFile (+0x40) and slot 17
// ReleaseChildren (+0x44) through the real vtable, and calls the non-virtual
// NotifyAllPlanes (0x160f40) / FindPlaneByName (0x15dde0); the dirty-flag word is
// CLoadable's m_08. `node` is the resolved parse-source record (CParseSource*).
class CParseSource;

// CWorldState + LevelMgr are the canonical <Gruntz/WorldState.h> (included below).

// The world/game-registry object at +0x4: its rez-path name query (returns a CString
// by value), the current world-name string, and the two mode gates.

// The "BACK" plane cache (reloc-masked DIR32 store).
struct ScrollView;
// The alternate-level-set gate (second run of levels). Single DATA binding here.

RVA(0x000dbc80, 0x309)
i32 CWorldState::BuildWorldLevelPath(i32 unused) {
    m_0c->m_24->ReleaseChildren();
    if (m_4->m_strWorldFile.GetLength() != 0) {
        if (m_4->m_128 != 0) {
            CString key = "BATTLEZ\\" + m_4->GetWorldFileName();
            i32 node = m_34->ResolveQualified(key, (void*)0x575744);
            if (node == 0) {
                return 0;
            }
            if (m_0c->m_24->LoadFromSource((CParseSource*)node) == 0) {
                return 0;
            }
        } else if (m_4->m_12c != 0) {
            CString key = "MULTI\\" + m_4->GetWorldFileName();
            i32 node = m_34->ResolveQualified(key, (void*)0x575744);
            if (node == 0) {
                return 0;
            }
            if (m_0c->m_24->LoadFromSource((CParseSource*)node) == 0) {
                return 0;
            }
        } else {
            if (m_0c->m_24->LoadFromFile(m_4->GetWorldFileName()) == 0) {
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
