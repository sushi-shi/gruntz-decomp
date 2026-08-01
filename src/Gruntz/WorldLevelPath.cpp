#include <Mfc.h>
#include <Gruntz/GruntzMgr.h>
#include <rva.h>

#include <Ints.h>

#include <Bute/SymTab.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/Play.h>
#include <Gruntz/GameLevel.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/MgrAutoScroll.h>

class CParseSource;

RVA(0x000dbc80, 0x309)
i32 CPlay::BuildWorldLevelPath(i32 unused) {
    m_world->m_level->ReleaseChildren();
    if (m_mgr->m_strWorldFile.GetLength() != 0) {
        if (m_mgr->m_128 != 0) {
            CString key = "BATTLEZ\\" + m_mgr->GetWorldFileName();
            CParseSource* node = m_gameBank->ResolveQualified(key, 0x575744);
            if (node == 0) {
                return 0;
            }
            if (m_world->m_level->LoadFromSource(node) == 0) {
                return 0;
            }
        } else if (m_mgr->m_12c != 0) {
            CString key = "MULTI\\" + m_mgr->GetWorldFileName();
            CParseSource* node = m_gameBank->ResolveQualified(key, 0x575744);
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
        CParseSource* node = m_levelBank->ResolveQualified(key, 0x575744);
        if (node == 0) {
            return 0;
        }
        if (m_world->m_level->LoadFromSource(node) == 0) {
            return 0;
        }
    }
    m_world->m_level->NotifyAllPlanes();
    m_world->m_level->m_flags |= 4;
    g_backView = m_world->m_level->FindPlaneByName("BACK");
    return 1;
}
