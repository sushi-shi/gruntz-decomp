#include <Mfc.h> // real MFC CString (ctor(const char*) 0x1b9d4c / dtor 0x1b9cde /
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

#include <Ints.h>
#include <Globals.h>

#include <Bute/SymTab.h>       // the shared CSymTab (ResolveQualified 0x13be40)
#include <Gruntz/GameLevel.h>  // canonical CGameLevel (real virtual slots 15/16/17 + non-virtuals)
#include <Gruntz/WorldState.h> // canonical CWorldState + LevelMgr

class CParseSource;

RVA(0x000dbc80, 0x309)
i32 CWorldState::BuildWorldLevelPath(i32 unused) {
    m_0c->m_24->ReleaseChildren();
    if (m_4->m_strWorldFile.GetLength() != 0) {
        if (m_4->m_128 != 0) {
            CString key = "BATTLEZ\\" + m_4->GetWorldFileName();
            CParseSource* node = m_34->ResolveQualified(key, reinterpret_cast<void*>(0x575744));
            if (node == 0) {
                return 0;
            }
            if (m_0c->m_24->LoadFromSource(node) == 0) {
                return 0;
            }
        } else if (m_4->m_12c != 0) {
            CString key = "MULTI\\" + m_4->GetWorldFileName();
            CParseSource* node = m_34->ResolveQualified(key, reinterpret_cast<void*>(0x575744));
            if (node == 0) {
                return 0;
            }
            if (m_0c->m_24->LoadFromSource(node) == 0) {
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
        if (g_levelBias100 != 0) {
            sel += 0x64;
        }
        if (sel > 0x24 && sel <= 0x28) {
            key.Format("WORLDZ\\TRAINING%i", sel % 0x24);
        } else {
            key.Format("WORLDZ\\LEVEL%i", sel);
        }
        CParseSource* node = m_28->ResolveQualified(key, reinterpret_cast<void*>(0x575744));
        if (node == 0) {
            return 0;
        }
        if (m_0c->m_24->LoadFromSource(node) == 0) {
            return 0;
        }
    }
    m_0c->m_24->NotifyAllPlanes();
    m_0c->m_24->m_08 |= 4;
    g_backView = m_0c->m_24->FindPlaneByName("BACK");
    return 1;
}
