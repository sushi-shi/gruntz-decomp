#include <Mfc.h> // real MFC CString (ctor(const char*) 0x1b9d4c / dtor 0x1b9cde /
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

#include <Ints.h>

#include <Bute/SymTab.h>      // the shared CSymTab (ResolveQualified 0x13be40)
#include <Gruntz/GameLevel.h> // canonical CGameLevel (real virtual slots 15/16/17 + non-virtuals)
#include <Gruntz/Play.h>      // CPlay - slot-42 owner (the ex-CWorldState view is dissolved)
#include <Gruntz/GameLevel.h> // m_world->m_level (CGameLevel)
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GruntzMgr.h>     // m_mgr fields (world file, battlez/multi gates)
#include <Gruntz/MgrAutoScroll.h> // ex Globals.h

class CParseSource;

RVA(0x000dbc80, 0x309)
i32 CPlay::BuildWorldLevelPath(i32 unused) {
    m_world->m_level->ReleaseChildren();
    if (m_mgr->m_strWorldFile.GetLength() != 0) {
        if (m_mgr->m_128 != 0) {
            CString key = "BATTLEZ\\" + m_mgr->GetWorldFileName();
            CParseSource* node =
                m_gameBank->ResolveQualified(key, reinterpret_cast<void*>(0x575744));
            if (node == 0) {
                return 0;
            }
            if (m_world->m_level->LoadFromSource(node) == 0) {
                return 0;
            }
        } else if (m_mgr->m_12c != 0) {
            CString key = "MULTI\\" + m_mgr->GetWorldFileName();
            CParseSource* node =
                m_gameBank->ResolveQualified(key, reinterpret_cast<void*>(0x575744));
            if (node == 0) {
                return 0;
            }
            if (m_world->m_level->LoadFromSource(node) == 0) {
                return 0;
            }
        } else {
            if (m_world->m_level->LoadFromFile(m_mgr->GetWorldFileName()) == 0) {
                return 0;
            }
        }
    } else {
        CString key;
        i32 sel = m_levelIndex;
        if (g_levelBias100 != 0) {
            sel += 0x64;
        }
        if (sel > 0x24 && sel <= 0x28) {
            key.Format("WORLDZ\\TRAINING%i", sel % 0x24);
        } else {
            key.Format("WORLDZ\\LEVEL%i", sel);
        }
        CParseSource* node = m_levelBank->ResolveQualified(key, reinterpret_cast<void*>(0x575744));
        if (node == 0) {
            return 0;
        }
        if (m_world->m_level->LoadFromSource(node) == 0) {
            return 0;
        }
    }
    m_world->m_level->NotifyAllPlanes();
    m_world->m_level->m_flags |= 4; // the CLoadable +0x08 flag word
    g_backView = m_world->m_level->FindPlaneByName("BACK");
    return 1;
}
