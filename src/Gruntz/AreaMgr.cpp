#include <rva.h>

#include <Gruntz/AreaMgr.h>

#include <Mfc.h>

#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Enums.h>
#include <Image/CImage.h>

#include <stdio.h>
#include <string.h>

DATA(0x0021139c)
CAreaMgr* g_pAreaMgr = &g_areaMgr;

DATA(0x002459b0)
CAreaMgr g_areaMgr;

RVA(0x00099ba0, 0x29)
CAreaMgr::CAreaMgr() {
    m_currentLevelIndex = 0;
}

inline CSpawnList::~CSpawnList() {
    DeleteAllEntries();
}
static CSpawnList* volatile g_forceDtorSink;
#pragma inline_depth(0)
void ForceEmitSpawnListDtor() {
    g_forceDtorSink->~CSpawnList();
}
#pragma inline_depth()

RVA(0x00099c20, 0x5f)
CAreaMgr::~CAreaMgr() {
    Reset();
}

RVA_COMPGEN(0x00099ca0, 0x49, ??1CSpawnList@@QAE@XZ)

RVA(0x00099d10, 0x20)
i32 InitializeLevelArea(i32 arg) {
    g_areaMgr.Reset();
    return g_areaMgr.InitializeLevel(arg) != 0;
}

// @early-stop
RVA(0x00099d40, 0x2c0)
i32 CAreaMgr::InitializeLevel(i32 index) {
    if (index <= 0 || index > 0x28) {
        return 0;
    }
    Reset();
    m_currentLevelIndex = index;
    // `index` is not a domain - it is the level ORDINAL, and every arm is that
    // same number's InitializeLevelNN.
    switch (index) {
        case 1:
            return InitializeLevel01();
        case 2:
            return InitializeLevel02();
        case 3:
            return InitializeLevel03();
        case 4:
            return InitializeLevel04();
        case 5:
            return InitializeLevel05();
        case 6:
            return InitializeLevel06();
        case 7:
            return InitializeLevel07();
        case 8:
            return InitializeLevel08();
        case 9:
            return InitializeLevel09();
        case 10:
            return InitializeLevel10();
        case 11:
            return InitializeLevel11();
        case 12:
            return InitializeLevel12();
        case 13:
            return InitializeLevel13();
        case 14:
            return InitializeLevel14();
        case 15:
            return InitializeLevel15();
        case 16:
            return InitializeLevel16();
        case 17:
            return InitializeLevel17();
        case 18:
            return InitializeLevel18();
        case 19:
            return InitializeLevel19();
        case 20:
            return InitializeLevel20();
        case 21:
            return InitializeLevel21();
        case 22:
            return InitializeLevel22();
        case 23:
            return InitializeLevel23();
        case 24:
            return InitializeLevel24();
        case 25:
            return InitializeLevel25();
        case 26:
            return InitializeLevel26();
        case 27:
            return InitializeLevel27();
        case 28:
            return InitializeLevel28();
        case 29:
            return InitializeLevel29();
        case 30:
            return InitializeLevel30();
        case 31:
            return InitializeLevel31();
        case 32:
            return InitializeLevel32();
        case 33:
            return InitializeLevel33();
        case 34:
            return InitializeLevel34();
        case 35:
            return InitializeLevel35();
        case 36:
            return InitializeLevel36();
        case 37:
            return InitializeLevel37();
        case 38:
            return InitializeLevel38();
        case 39:
            return InitializeLevel39();
        case 40:
            return InitializeLevel40();
        default:
            return 0;
    }
}

RVA(0x0009a0b0, 0x7)
void CAreaMgr::Reset() {
    m_currentLevelIndex = 0;
}

RVA(0x0009a0d0, 0x133)
CSpawnEntry* CSpawnList::FindEntry(CString name, i32 useHash) {
    for (POSITION n = m_list.GetHeadPosition(); n != NULL;) {
        CSpawnEntry* e = NextEntry(n);
        if (e == NULL) {
            continue;
        }
        if (useHash != 0) {
            CString nm = e->GetName();
            if (strncmp(nm, name, nm.GetLength()) == 0) {
                return e;
            }
        } else {
            if (name == e->GetName()) {
                return e;
            }
        }
    }
    return 0;
}

// @early-stop
RVA(0x0009a290, 0x138)
CSpawnEntry* CSpawnList::FindByName(const CString& name) {
    CString key(name);
    for (POSITION n = m_list.GetHeadPosition(); n != NULL;) {
        CSpawnEntry* e = NextEntry(n);
        if (e == NULL) {
            continue;
        }
        CString nm = e->GetName();
        if (strcmp(key, nm) == 0) {
            return e;
        }
        CString empty;
        if (strncmp(nm, empty, nm.GetLength()) == 0) {
            return e;
        }
    }
    return 0;
}

RVA(0x0009a420, 0x1c)
void CSpawnList::ClearFlags() {
    POSITION p = m_list.GetHeadPosition();
    if (p == NULL) {
        return;
    }
    do {
        CSpawnEntry* e = NextEntry(p);
        if (e != NULL) {
            e->m_flag = 0;
        }
    } while (p != NULL);
}

RVA(0x0009a450, 0x36)
void CSpawnList::DeleteAllEntries() {
    POSITION node = m_list.GetHeadPosition();
    while (node != NULL) {
        CSpawnEntry* e = NextEntry(node);
        if (e != NULL) {
            delete e;
        }
    }
    m_list.RemoveAll();
}

RVA(0x0009a4c0, 0x3e)
i32 CAreaMgr::LoadObjectResources(CDDrawSurfaceMgr* entry, CSymTab* src) {
    if (entry == NULL) {
        return 0;
    }
    LoadObjectImageResources(entry, src);
    LoadObjectSoundResources(entry, src);
    LoadObjectAnimResources(entry, src);
    return 1;
}

// @early-stop
RVA(0x0009a510, 0x275)
i32 CAreaMgr::LoadObjectImageResources(CDDrawSurfaceMgr* entry, CSymTab* src) {
    if (entry == NULL) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToOb* srcMap = &entry->m_imageRegistry->m_10map;
    if (srcMap == NULL) {
        return 0;
    }

    CPtrList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        CObject* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp(static_cast<const char*>(static_cast<LPCTSTR>(key)), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != NULL) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail(val);
            }
        }
    }

    POSITION dp = toAdd.GetHeadPosition();
    while (dp != NULL) {
        CDDrawWorker* obj = static_cast<CDDrawWorker*>(toAdd.GetNext(dp));
        entry->m_imageRegistry->RemoveWorker(obj);
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = b->m_list.GetHeadPosition();
    CSpawnEntry* e;
    if (b->m_cursor == NULL) {
        e = NULL;
    } else {
        e = b->NextEntry(b->m_cursor);
    }
    while (e != NULL) {
        if (e->m_flag == 0) {
            char buf[0x80];
            g_resourceInstallActive = 1;
            sprintf(buf, "IMAGEZ_%s", static_cast<const char*>(static_cast<LPCTSTR>(e->GetTail())));
            void* handle = src->ResolvePath(buf);
            if (handle == NULL) {
                return 0;
            }
            entry->m_imageRegistry
                ->InstallTree(handle, const_cast<char*>(static_cast<LPCTSTR>(e->GetName())), "");
            g_resourceInstallActive = 0;
            e->m_flag = 1;
        }
        if (b->m_cursor == NULL) {
            e = NULL;
        } else {
            e = b->NextEntry(b->m_cursor);
        }
    }
    return 1;
}

// @early-stop
RVA(0x0009a830, 0xa4)
CString CSpawnEntry::GetTail() {
    CString tmp;
    i32 len = m_name.GetLength();
    if (len == 0) {
        return tmp;
    }
    if (len > 8) {
        tmp = static_cast<const char*>(m_name) + 8;
    }
    return tmp;
}

// @early-stop
RVA(0x0009a910, 0x261)
i32 CAreaMgr::LoadObjectSoundResources(CDDrawSurfaceMgr* entry, CSymTab* src) {
    if (entry == NULL) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToPtr* srcMap = &entry->m_soundRegistry->m_cues;
    if (srcMap == NULL) {
        return 0;
    }

    CPtrList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        void* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp(static_cast<const char*>(static_cast<LPCTSTR>(key)), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != NULL) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail(val);
            }
        }
    }

    POSITION dp = toAdd.GetHeadPosition();
    while (dp != NULL) {
        void* obj = toAdd.GetNext(dp);
        entry->m_soundRegistry->RemoveByValue(static_cast<LeafCue*>(obj));
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = b->m_list.GetHeadPosition();
    CSpawnEntry* e;
    if (b->m_cursor == NULL) {
        e = NULL;
    } else {
        e = b->NextEntry(b->m_cursor);
    }
    while (e != NULL) {
        if (e->m_flag == 0) {
            char buf[0x80];
            sprintf(buf, "SOUNDZ_%s", static_cast<const char*>(static_cast<LPCTSTR>(e->GetTail())));
            void* handle = src->ResolvePath(buf);
            if (handle == NULL) {
                return 0;
            }
            entry->m_soundRegistry->ScanTree(
                static_cast<CSymTab*>(handle),
                const_cast<char*>(static_cast<LPCTSTR>(e->GetName())),
                ""
            );
            e->m_flag = 1;
        }
        if (b->m_cursor == NULL) {
            e = NULL;
        } else {
            e = b->NextEntry(b->m_cursor);
        }
    }
    return 1;
}

// @early-stop
RVA(0x0009ac20, 0x261)
i32 CAreaMgr::LoadObjectAnimResources(CDDrawSurfaceMgr* entry, CSymTab* src) {
    if (entry == NULL) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToPtr* srcMap = &entry->m_animRegistry->m_animations;
    if (srcMap == NULL) {
        return 0;
    }

    CPtrList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        void* val;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp(static_cast<const char*>(static_cast<LPCTSTR>(key)), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != NULL) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail(val);
            }
        }
    }

    POSITION dp = toAdd.GetHeadPosition();
    while (dp != NULL) {
        void* obj = toAdd.GetNext(dp);
        entry->m_animRegistry->RemoveValue(static_cast<CAniElement*>(obj));
    }
    toAdd.RemoveAll();

    CSpawnList* b = &m_spawnEntryList;
    b->m_cursor = b->m_list.GetHeadPosition();
    CSpawnEntry* e;
    if (b->m_cursor == NULL) {
        e = NULL;
    } else {
        e = b->NextEntry(b->m_cursor);
    }
    while (e != NULL) {
        if (e->m_flag == 0) {
            char buf[0x80];
            sprintf(buf, "ANIZ_%s", static_cast<const char*>(static_cast<LPCTSTR>(e->GetTail())));
            void* handle = src->ResolvePath(buf);
            if (handle == NULL) {
                return 0;
            }
            entry->m_animRegistry->ScanTree(
                static_cast<CSymTab*>(handle),
                const_cast<char*>(static_cast<LPCTSTR>(e->GetName())),
                ""
            );
            e->m_flag = 1;
        }
        if (b->m_cursor == NULL) {
            e = NULL;
        } else {
            e = b->NextEntry(b->m_cursor);
        }
    }
    return 1;
}

RVA(0x0009af30, 0x6)
i32 CAreaMgr::InitializeLevel01() {
    return 1;
}

RVA(0x0009af50, 0x6)
i32 CAreaMgr::InitializeLevel02() {
    return 1;
}

RVA(0x0009af70, 0x6)
i32 CAreaMgr::InitializeLevel03() {
    return 1;
}

RVA(0x0009af90, 0x6)
i32 CAreaMgr::InitializeLevel04() {
    return 1;
}

RVA(0x0009afb0, 0x6)
i32 CAreaMgr::InitializeLevel05() {
    return 1;
}

RVA(0x0009afd0, 0x6)
i32 CAreaMgr::InitializeLevel06() {
    return 1;
}

RVA(0x0009aff0, 0x6)
i32 CAreaMgr::InitializeLevel07() {
    return 1;
}

RVA(0x0009b010, 0x6)
i32 CAreaMgr::InitializeLevel08() {
    return 1;
}

RVA(0x0009b030, 0x6)
i32 CAreaMgr::InitializeLevel09() {
    return 1;
}

RVA(0x0009b050, 0x6)
i32 CAreaMgr::InitializeLevel10() {
    return 1;
}

RVA(0x0009b070, 0x6)
i32 CAreaMgr::InitializeLevel11() {
    return 1;
}

RVA(0x0009b090, 0x6)
i32 CAreaMgr::InitializeLevel12() {
    return 1;
}

RVA(0x0009b0b0, 0x6)
i32 CAreaMgr::InitializeLevel13() {
    return 1;
}

RVA(0x0009b0d0, 0x6)
i32 CAreaMgr::InitializeLevel14() {
    return 1;
}

RVA(0x0009b0f0, 0x6)
i32 CAreaMgr::InitializeLevel15() {
    return 1;
}

RVA(0x0009b110, 0x6)
i32 CAreaMgr::InitializeLevel16() {
    return 1;
}

RVA(0x0009b130, 0x6)
i32 CAreaMgr::InitializeLevel17() {
    return 1;
}

RVA(0x0009b150, 0x6)
i32 CAreaMgr::InitializeLevel18() {
    return 1;
}

RVA(0x0009b170, 0x6)
i32 CAreaMgr::InitializeLevel19() {
    return 1;
}

RVA(0x0009b190, 0x6)
i32 CAreaMgr::InitializeLevel20() {
    return 1;
}

RVA(0x0009b1b0, 0x6)
i32 CAreaMgr::InitializeLevel21() {
    return 1;
}

RVA(0x0009b1d0, 0x6)
i32 CAreaMgr::InitializeLevel22() {
    return 1;
}

RVA(0x0009b1f0, 0x6)
i32 CAreaMgr::InitializeLevel23() {
    return 1;
}

RVA(0x0009b210, 0x6)
i32 CAreaMgr::InitializeLevel24() {
    return 1;
}

RVA(0x0009b230, 0x6)
i32 CAreaMgr::InitializeLevel25() {
    return 1;
}

RVA(0x0009b250, 0x6)
i32 CAreaMgr::InitializeLevel26() {
    return 1;
}

RVA(0x0009b270, 0x6)
i32 CAreaMgr::InitializeLevel27() {
    return 1;
}

RVA(0x0009b290, 0x6)
i32 CAreaMgr::InitializeLevel28() {
    return 1;
}

RVA(0x0009b2b0, 0x6)
i32 CAreaMgr::InitializeLevel29() {
    return 1;
}

RVA(0x0009b2d0, 0x6)
i32 CAreaMgr::InitializeLevel30() {
    return 1;
}

RVA(0x0009b2f0, 0x6)
i32 CAreaMgr::InitializeLevel31() {
    return 1;
}

RVA(0x0009b310, 0x6)
i32 CAreaMgr::InitializeLevel32() {
    return 1;
}

RVA(0x0009b330, 0x6)
i32 CAreaMgr::InitializeLevel33() {
    return 1;
}

RVA(0x0009b350, 0x6)
i32 CAreaMgr::InitializeLevel34() {
    return 1;
}

RVA(0x0009b370, 0x6)
i32 CAreaMgr::InitializeLevel35() {
    return 1;
}

RVA(0x0009b390, 0x6)
i32 CAreaMgr::InitializeLevel36() {
    return 1;
}

RVA(0x0009b3b0, 0x6)
i32 CAreaMgr::InitializeLevel37() {
    return 1;
}

RVA(0x0009b3d0, 0x6)
i32 CAreaMgr::InitializeLevel38() {
    return 1;
}

RVA(0x0009b3f0, 0x6)
i32 CAreaMgr::InitializeLevel39() {
    return 1;
}

RVA(0x0009b410, 0x6)
i32 CAreaMgr::InitializeLevel40() {
    return 1;
}

// @early-stop
RVA(0x0009b430, 0x49)
i32 CAreaMgr::IsSameWorld(i32 a) {
    if (a <= 0) {
        return 0;
    }
    i32 ga = (a - 1) % 36 / 4 + 1;
    i32 gc = (m_currentLevelIndex - 1) % 36 / 4 + 1;
    return gc == ga;
}
