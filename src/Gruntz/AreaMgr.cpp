#include <rva.h>

#include <Gruntz/AreaMgr.h>

#include <Mfc.h>

#include <Bute/SymTab.h>
#include <DDrawMgr/DDrawSubMgrLeaf.h>
#include <DDrawMgr/DDrawSubMgrLeafScan.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Enums.h>
#include <Gruntz/QuestLevel.h>
#include <Image/CImage.h>
#include <Utils/MapTyped.h>

#include <stdio.h>
#include <string.h>

DATA(0x0021139c)
CAreaMgr* g_pAreaMgr = &g_areaMgr;

RVA_DYNINIT(0x00099b60, 0xa, g_areaMgr)
RVA_DYNINIT(0x00099b80, 0xa, g_areaMgr)
RVA_DYNINIT(0x00099be0, 0xe, g_areaMgr)
RVA_DYNINIT(0x00099c00, 0xa, g_areaMgr)
DATA(0x002459b0)
CAreaMgr g_areaMgr;

RVA(0x00099ba0, 0x29)
CAreaMgr::CAreaMgr() {
    m_currentLevelIndex = 0;
}

inline CSpawnList::~CSpawnList() {
    DeleteAllEntries();
}

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

RVA(0x00099d40, 0x2c0)
i32 CAreaMgr::InitializeLevel(i32 index) {
    QuestLevel level = static_cast<QuestLevel>(index);
    if (level <= QUESTLEVEL_NONE || level > QUESTLEVEL_TRAINING_LAST) {
        return 0;
    }
    Reset();
    m_currentLevelIndex = index;
    i32 result = 0;
    switch (level) {
        case QUESTLEVEL_AREA1_STAGE1:
            result = InitializeArea1Stage1();
            break;
        case QUESTLEVEL_AREA1_STAGE2:
            result = InitializeArea1Stage2();
            break;
        case QUESTLEVEL_AREA1_STAGE3:
            result = InitializeArea1Stage3();
            break;
        case QUESTLEVEL_AREA1_STAGE4:
            result = InitializeArea1Stage4();
            break;
        case QUESTLEVEL_AREA2_STAGE1:
            result = InitializeArea2Stage1();
            break;
        case QUESTLEVEL_AREA2_STAGE2:
            result = InitializeArea2Stage2();
            break;
        case QUESTLEVEL_AREA2_STAGE3:
            result = InitializeArea2Stage3();
            break;
        case QUESTLEVEL_AREA2_STAGE4:
            result = InitializeArea2Stage4();
            break;
        case QUESTLEVEL_AREA3_STAGE1:
            result = InitializeArea3Stage1();
            break;
        case QUESTLEVEL_AREA3_STAGE2:
            result = InitializeArea3Stage2();
            break;
        case QUESTLEVEL_AREA3_STAGE3:
            result = InitializeArea3Stage3();
            break;
        case QUESTLEVEL_AREA3_STAGE4:
            result = InitializeArea3Stage4();
            break;
        case QUESTLEVEL_AREA4_STAGE1:
            result = InitializeArea4Stage1();
            break;
        case QUESTLEVEL_AREA4_STAGE2:
            result = InitializeArea4Stage2();
            break;
        case QUESTLEVEL_AREA4_STAGE3:
            result = InitializeArea4Stage3();
            break;
        case QUESTLEVEL_AREA4_STAGE4:
            result = InitializeArea4Stage4();
            break;
        case QUESTLEVEL_AREA5_STAGE1:
            result = InitializeArea5Stage1();
            break;
        case QUESTLEVEL_AREA5_STAGE2:
            result = InitializeArea5Stage2();
            break;
        case QUESTLEVEL_AREA5_STAGE3:
            result = InitializeArea5Stage3();
            break;
        case QUESTLEVEL_AREA5_STAGE4:
            result = InitializeArea5Stage4();
            break;
        case QUESTLEVEL_AREA6_STAGE1:
            result = InitializeArea6Stage1();
            break;
        case QUESTLEVEL_AREA6_STAGE2:
            result = InitializeArea6Stage2();
            break;
        case QUESTLEVEL_AREA6_STAGE3:
            result = InitializeArea6Stage3();
            break;
        case QUESTLEVEL_AREA6_STAGE4:
            result = InitializeArea6Stage4();
            break;
        case QUESTLEVEL_AREA7_STAGE1:
            result = InitializeArea7Stage1();
            break;
        case QUESTLEVEL_AREA7_STAGE2:
            result = InitializeArea7Stage2();
            break;
        case QUESTLEVEL_AREA7_STAGE3:
            result = InitializeArea7Stage3();
            break;
        case QUESTLEVEL_AREA7_STAGE4:
            result = InitializeArea7Stage4();
            break;
        case QUESTLEVEL_AREA8_STAGE1:
            result = InitializeArea8Stage1();
            break;
        case QUESTLEVEL_AREA8_STAGE2:
            result = InitializeArea8Stage2();
            break;
        case QUESTLEVEL_AREA8_STAGE3:
            result = InitializeArea8Stage3();
            break;
        case QUESTLEVEL_AREA8_STAGE4:
            result = InitializeArea8Stage4();
            break;
        case QUESTLEVEL_RESERVED_33:
            result = InitializeReservedLevel33();
            break;
        case QUESTLEVEL_RESERVED_34:
            result = InitializeReservedLevel34();
            break;
        case QUESTLEVEL_RESERVED_35:
            result = InitializeReservedLevel35();
            break;
        case QUESTLEVEL_RESERVED_36:
            result = InitializeReservedLevel36();
            break;
        case QUESTLEVEL_TRAINING_STAGE1:
            result = InitializeTrainingStage1();
            break;
        case QUESTLEVEL_TRAINING_STAGE2:
            result = InitializeTrainingStage2();
            break;
        case QUESTLEVEL_TRAINING_STAGE3:
            result = InitializeTrainingStage3();
            break;
        case QUESTLEVEL_TRAINING_STAGE4:
            result = InitializeTrainingStage4();
            break;
    }
    return result;
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

RVA(0x0009a290, 0x138)
CSpawnEntry* CSpawnList::FindByName(const CString& name) {
    CString key = name + "_";
    for (POSITION n = m_list.GetHeadPosition(); n != NULL;) {
        CSpawnEntry* e = NextEntry(n);
        if (e == NULL) {
            continue;
        }
        CString nm = e->GetName();
        if (strcmp(name, nm) == 0) {
            return e;
        }
        nm += "_";
        if (strncmp(nm, key, nm.GetLength()) == 0) {
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

// cl folds the implicit `~CSpawnEntry` (one CString member at offset 0) into a
// bare tail-call COMDAT; retail's copy is the 5-byte `jmp ??1CString` at 0x9a4a0,
// reached through the ILT thunk 0x22b6 that DeleteAllEntries calls.
RVA_COMPGEN(0x0009a4a0, 0x5, ??1CSpawnEntry@@QAE@XZ)

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

RVA(0x0009a510, 0x275)
i32 CAreaMgr::LoadObjectImageResources(CDDrawSurfaceMgr* entry, CSymTab* src) {
    if (entry == NULL) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToOb* srcMap = &entry->m_imageRegistry->m_workersByName;
    if (srcMap == NULL) {
        return 0;
    }

    CPtrList toAdd;
    POSITION pos = srcMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        CObject* val = NULL;
        srcMap->GetNextAssoc(pos, key, val);
        if (strncmp(static_cast<LPCTSTR>(key), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != NULL) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail(val);
            }
        }
    }

    pos = toAdd.GetHeadPosition();
    while (pos != NULL) {
        CDDrawWorker* obj = static_cast<CDDrawWorker*>(toAdd.GetNext(pos));
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
            sprintf(buf, "IMAGEZ_%s", static_cast<LPCTSTR>(e->GetTail()));
            CSymTab* handle = src->ResolvePath(buf);
            if (handle == NULL) {
                return 0;
            }
            entry->m_imageRegistry
                ->InstallTree(handle, const_cast<char*>(static_cast<LPCTSTR>(e->GetName())), "_");
            TRACE("%s\n", static_cast<LPCTSTR>(e->GetName()));
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

RVA(0x0009a830, 0xa4)
CString CSpawnEntry::GetTail() {
    CString tmp;
    i32 len = m_name.GetLength();
    if (len == 0) {
        return tmp;
    }
    if (len <= 8) {
        return tmp;
    }
    tmp = static_cast<const char*>(m_name) + 8;
    return tmp;
}

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
        LeafCue* value = NULL;
        MapGetNext(*srcMap, pos, key, value);
        if (strncmp(static_cast<LPCTSTR>(key), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != NULL) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail(value);
            }
        }
    }

    pos = toAdd.GetHeadPosition();
    while (pos != NULL) {
        LeafCue* obj = static_cast<LeafCue*>(toAdd.GetNext(pos));
        entry->m_soundRegistry->RemoveByValue(obj);
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
            sprintf(buf, "SOUNDZ_%s", static_cast<LPCTSTR>(e->GetTail()));
            CSymTab* handle = src->ResolvePath(buf);
            if (handle == NULL) {
                return 0;
            }
            entry->m_soundRegistry->ScanTree(
                static_cast<CSymTab*>(handle),
                const_cast<char*>(static_cast<LPCTSTR>(e->GetName())),
                "_"
            );
            TRACE("%s\n", static_cast<LPCTSTR>(e->GetName()));
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
        CAniElement* value = NULL;
        MapGetNext(*srcMap, pos, key, value);
        if (strncmp(static_cast<LPCTSTR>(key), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* found = m_spawnEntryList.FindByName(key);
            if (found != NULL) {
                found->m_flag = 1;
            } else {
                toAdd.AddTail(value);
            }
        }
    }

    pos = toAdd.GetHeadPosition();
    while (pos != NULL) {
        CAniElement* obj = static_cast<CAniElement*>(toAdd.GetNext(pos));
        entry->m_animRegistry->RemoveValue(obj);
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
            sprintf(buf, "ANIZ_%s", static_cast<LPCTSTR>(e->GetTail()));
            CSymTab* handle = src->ResolvePath(buf);
            if (handle == NULL) {
                return 0;
            }
            entry->m_animRegistry->ScanTree(
                static_cast<CSymTab*>(handle),
                const_cast<char*>(static_cast<LPCTSTR>(e->GetName())),
                "_"
            );
            TRACE("%s\n", static_cast<LPCTSTR>(e->GetName()));
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
i32 CAreaMgr::InitializeArea1Stage1() {
    return 1;
}

RVA(0x0009af50, 0x6)
i32 CAreaMgr::InitializeArea1Stage2() {
    return 1;
}

RVA(0x0009af70, 0x6)
i32 CAreaMgr::InitializeArea1Stage3() {
    return 1;
}

RVA(0x0009af90, 0x6)
i32 CAreaMgr::InitializeArea1Stage4() {
    return 1;
}

RVA(0x0009afb0, 0x6)
i32 CAreaMgr::InitializeArea2Stage1() {
    return 1;
}

RVA(0x0009afd0, 0x6)
i32 CAreaMgr::InitializeArea2Stage2() {
    return 1;
}

RVA(0x0009aff0, 0x6)
i32 CAreaMgr::InitializeArea2Stage3() {
    return 1;
}

RVA(0x0009b010, 0x6)
i32 CAreaMgr::InitializeArea2Stage4() {
    return 1;
}

RVA(0x0009b030, 0x6)
i32 CAreaMgr::InitializeArea3Stage1() {
    return 1;
}

RVA(0x0009b050, 0x6)
i32 CAreaMgr::InitializeArea3Stage2() {
    return 1;
}

RVA(0x0009b070, 0x6)
i32 CAreaMgr::InitializeArea3Stage3() {
    return 1;
}

RVA(0x0009b090, 0x6)
i32 CAreaMgr::InitializeArea3Stage4() {
    return 1;
}

RVA(0x0009b0b0, 0x6)
i32 CAreaMgr::InitializeArea4Stage1() {
    return 1;
}

RVA(0x0009b0d0, 0x6)
i32 CAreaMgr::InitializeArea4Stage2() {
    return 1;
}

RVA(0x0009b0f0, 0x6)
i32 CAreaMgr::InitializeArea4Stage3() {
    return 1;
}

RVA(0x0009b110, 0x6)
i32 CAreaMgr::InitializeArea4Stage4() {
    return 1;
}

RVA(0x0009b130, 0x6)
i32 CAreaMgr::InitializeArea5Stage1() {
    return 1;
}

RVA(0x0009b150, 0x6)
i32 CAreaMgr::InitializeArea5Stage2() {
    return 1;
}

RVA(0x0009b170, 0x6)
i32 CAreaMgr::InitializeArea5Stage3() {
    return 1;
}

RVA(0x0009b190, 0x6)
i32 CAreaMgr::InitializeArea5Stage4() {
    return 1;
}

RVA(0x0009b1b0, 0x6)
i32 CAreaMgr::InitializeArea6Stage1() {
    return 1;
}

RVA(0x0009b1d0, 0x6)
i32 CAreaMgr::InitializeArea6Stage2() {
    return 1;
}

RVA(0x0009b1f0, 0x6)
i32 CAreaMgr::InitializeArea6Stage3() {
    return 1;
}

RVA(0x0009b210, 0x6)
i32 CAreaMgr::InitializeArea6Stage4() {
    return 1;
}

RVA(0x0009b230, 0x6)
i32 CAreaMgr::InitializeArea7Stage1() {
    return 1;
}

RVA(0x0009b250, 0x6)
i32 CAreaMgr::InitializeArea7Stage2() {
    return 1;
}

RVA(0x0009b270, 0x6)
i32 CAreaMgr::InitializeArea7Stage3() {
    return 1;
}

RVA(0x0009b290, 0x6)
i32 CAreaMgr::InitializeArea7Stage4() {
    return 1;
}

RVA(0x0009b2b0, 0x6)
i32 CAreaMgr::InitializeArea8Stage1() {
    return 1;
}

RVA(0x0009b2d0, 0x6)
i32 CAreaMgr::InitializeArea8Stage2() {
    return 1;
}

RVA(0x0009b2f0, 0x6)
i32 CAreaMgr::InitializeArea8Stage3() {
    return 1;
}

RVA(0x0009b310, 0x6)
i32 CAreaMgr::InitializeArea8Stage4() {
    return 1;
}

RVA(0x0009b330, 0x6)
i32 CAreaMgr::InitializeReservedLevel33() {
    return 1;
}

RVA(0x0009b350, 0x6)
i32 CAreaMgr::InitializeReservedLevel34() {
    return 1;
}

RVA(0x0009b370, 0x6)
i32 CAreaMgr::InitializeReservedLevel35() {
    return 1;
}

RVA(0x0009b390, 0x6)
i32 CAreaMgr::InitializeReservedLevel36() {
    return 1;
}

RVA(0x0009b3b0, 0x6)
i32 CAreaMgr::InitializeTrainingStage1() {
    return 1;
}

RVA(0x0009b3d0, 0x6)
i32 CAreaMgr::InitializeTrainingStage2() {
    return 1;
}

RVA(0x0009b3f0, 0x6)
i32 CAreaMgr::InitializeTrainingStage3() {
    return 1;
}

RVA(0x0009b410, 0x6)
i32 CAreaMgr::InitializeTrainingStage4() {
    return 1;
}

// @early-stop
RVA(0x0009b430, 0x49)
b32 CAreaMgr::IsSameWorld(i32 levelIndex) {
    if (levelIndex <= 0) {
        return 0;
    }
    i32 requestedWorld = (levelIndex - 1) % 36 / 4 + 1;
    i32 currentWorld = (m_currentLevelIndex - 1) % 36 / 4 + 1;
    return currentWorld == requestedWorld;
}
