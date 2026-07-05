// WorldLevelKey.cpp - resolve the WORLDZ\LEVEL%i record for the active world
// (RVA 0x3c0e0). Resets the level record, formats its namespace key, resolves it,
// and on success runs the record's load hook + NotifyAllPlanes and raises its
// dirty bit. Field names are placeholders; only offsets + code bytes are
// load-bearing.
#include <Mfc.h> // real MFC CString (default ctor 0x1b9b93 / dtor 0x1b9cde, reloc-masked)
#include <rva.h>

#include <Ints.h>

#include <Bute/SymTab.h>       // the shared CSymTab (ResolveQualified 0x13be40)
#include <Gruntz/GameLevel.h>  // canonical CGameLevel (real virtual slots 15/17 + non-virtuals)
#include <Gruntz/WorldState.h> // canonical CWorldState + LevelMgr

// The loaded level record IS the canonical CGameLevel (<Gruntz/GameLevel.h>): this TU
// dispatches slot 15 LoadFromSource (+0x3c) and slot 17 ReleaseChildren (+0x44) through
// the real vtable, and calls the non-virtual NotifyAllPlanes (0x160f40); the dirty-flag
// word is CLoadable's m_08. `node` is the resolved parse-source record (CParseSource*).
class CParseSource;

// CWorldState + LevelMgr are the canonical <Gruntz/WorldState.h> (included below).

RVA(0x0003c0e0, 0xfb)
i32 CWorldState::BuildWorldLevelKey(i32 unused) {
    m_0c->m_24->ReleaseChildren();
    CString key;
    key.Format("WORLDZ\\LEVEL%i", 1);
    i32 node = m_28->ResolveQualified(key, (void*)0x575744);
    if (node == 0) {
        return 0;
    }
    if (m_0c->m_24->LoadFromSource((CParseSource*)node) == 0) {
        return 0;
    }
    m_0c->m_24->NotifyAllPlanes();
    m_0c->m_24->m_08 |= 4;
    return 1;
}
