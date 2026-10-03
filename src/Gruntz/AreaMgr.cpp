#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/AreaMgr.h>

#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerRegistry.h>
#include <Enums.h>
#include <Gruntz/AnimationRegistry.h>
#include <Gruntz/QuestLevel.h>
#include <Gruntz/SoundCueRegistry.h>
#include <Gruntz/SpawnList.h>
#include <Gruntz/SpawnListInline.h>
#include <Image/CImage.h>
#include <Rez/RezArchiveDir.h>
#include <Utils/MapTyped.h>

#include <stdio.h>
#include <string.h>

CAreaMgr* g_pAreaMgr = &g_areaMgr;

CAreaMgr g_areaMgr;

CAreaMgr::CAreaMgr() {
    m_currentLevelIndex = 0;
}

CAreaMgr::~CAreaMgr() {
    Reset();
}

i32 InitializeLevelArea(i32 levelIndex) {
    g_areaMgr.Reset();
    return g_areaMgr.InitializeLevel(levelIndex) != 0;
}

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

void CAreaMgr::Reset() {
    m_currentLevelIndex = 0;
}

CSpawnEntry* CSpawnList::FindEntry(CString name, b32 useHash) {
    for (POSITION n = m_list.GetHeadPosition(); n != NULL;) {
        CSpawnEntry* e = NextEntry(n);
        if (e == NULL) {
            continue;
        }
        if (useHash != false) {
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
    return NULL;
}

CSpawnEntry* CSpawnList::FindByName(const CString& name) {
    CString key = name + "_";
    for (POSITION n = m_list.GetHeadPosition(); n != NULL;) {
        CSpawnEntry* e = NextEntry(n);
        if (e == NULL) {
            continue;
        }
        CString nm = e->GetName();
        if (name.Compare(nm) == 0) {
            return e;
        }
        nm += "_";
        if (strncmp(nm, key, nm.GetLength()) == 0) {
            return e;
        }
    }
    return NULL;
}

void CSpawnList::ClearFlags() {
    POSITION p = m_list.GetHeadPosition();
    if (p == NULL) {
        return;
    }
    do {
        CSpawnEntry* e = NextEntry(p);
        if (e != NULL) {
            e->m_flag = false;
        }
    } while (p != NULL);
}

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

i32 CAreaMgr::LoadObjectResources(CDDrawSurfaceMgr* surfaceMgr, CRezDir* src) {
    if (surfaceMgr == NULL) {
        return 0;
    }
    LoadObjectImageResources(surfaceMgr, src);
    LoadObjectSoundResources(surfaceMgr, src);
    LoadObjectAnimResources(surfaceMgr, src);
    return 1;
}

i32 CAreaMgr::LoadObjectImageResources(CDDrawSurfaceMgr* surfaceMgr, CRezDir* src) {
    if (surfaceMgr == NULL) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToOb* registryMap = &surfaceMgr->m_imageRegistry->m_workersByName;
    if (registryMap == NULL) {
        return 0;
    }

    CPtrList toRemove;
    POSITION pos = registryMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        CObject* workerObject = NULL;
        registryMap->GetNextAssoc(pos, key, workerObject);
        if (strncmp(static_cast<LPCTSTR>(key), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* spawnEntry = m_spawnEntryList.FindByName(key);
            if (spawnEntry != NULL) {
                spawnEntry->m_flag = true;
            } else {
                toRemove.AddTail(workerObject);
            }
        }
    }

    pos = toRemove.GetHeadPosition();
    while (pos != NULL) {
        CDDrawWorker* worker = static_cast<CDDrawWorker*>(toRemove.GetNext(pos));
        surfaceMgr->m_imageRegistry->RemoveWorker(worker);
    }
    toRemove.RemoveAll();

    CSpawnList* spawnList = &m_spawnEntryList;
    CSpawnEntry* spawnEntry = spawnList->FirstEntry();
    while (spawnEntry != NULL) {
        if (spawnEntry->m_flag == false) {
            char resourcePath[0x80];
            g_resourceInstallActive = true;
            sprintf(resourcePath, "IMAGEZ_%s", static_cast<LPCTSTR>(spawnEntry->GetTail()));
            CRezDir* resourceTree = src->GetDirFromPath(resourcePath);
            if (resourceTree == NULL) {
                return 0;
            }
            surfaceMgr->m_imageRegistry->InstallTree(
                resourceTree,
                const_cast<char*>(static_cast<LPCTSTR>(spawnEntry->GetName())),
                "_"
            );
            TRACE("%s\n", static_cast<LPCTSTR>(spawnEntry->GetName()));
            g_resourceInstallActive = false;
            spawnEntry->m_flag = true;
        }
        spawnEntry = spawnList->NextEntry();
    }
    return 1;
}

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

i32 CAreaMgr::LoadObjectSoundResources(CDDrawSurfaceMgr* surfaceMgr, CRezDir* src) {
    if (surfaceMgr == NULL) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToPtr* registryMap = &surfaceMgr->SoundRegistry()->m_cues;
    if (registryMap == NULL) {
        return 0;
    }

    CPtrList toRemove;
    POSITION pos = registryMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        SoundCue* cue = NULL;
        MapGetNext(*registryMap, pos, key, cue);
        if (strncmp(static_cast<LPCTSTR>(key), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* spawnEntry = m_spawnEntryList.FindByName(key);
            if (spawnEntry != NULL) {
                spawnEntry->m_flag = true;
            } else {
                toRemove.AddTail(cue);
            }
        }
    }

    pos = toRemove.GetHeadPosition();
    while (pos != NULL) {
        SoundCue* cue = static_cast<SoundCue*>(toRemove.GetNext(pos));
        surfaceMgr->SoundRegistry()->RemoveCue(cue);
    }
    toRemove.RemoveAll();

    CSpawnList* spawnList = &m_spawnEntryList;
    CSpawnEntry* spawnEntry = spawnList->FirstEntry();
    while (spawnEntry != NULL) {
        if (spawnEntry->m_flag == false) {
            char resourcePath[0x80];
            sprintf(resourcePath, "SOUNDZ_%s", static_cast<LPCTSTR>(spawnEntry->GetTail()));
            CRezDir* resourceTree = src->GetDirFromPath(resourcePath);
            if (resourceTree == NULL) {
                return 0;
            }
            surfaceMgr->SoundRegistry()->LoadFromTree(
                resourceTree,
                const_cast<char*>(static_cast<LPCTSTR>(spawnEntry->GetName())),
                "_"
            );
            TRACE("%s\n", static_cast<LPCTSTR>(spawnEntry->GetName()));
            spawnEntry->m_flag = true;
        }
        spawnEntry = spawnList->NextEntry();
    }
    return 1;
}

i32 CAreaMgr::LoadObjectAnimResources(CDDrawSurfaceMgr* surfaceMgr, CRezDir* src) {
    if (surfaceMgr == NULL) {
        return 0;
    }
    m_spawnEntryList.ClearFlags();

    CMapStringToPtr* registryMap = &surfaceMgr->m_animRegistry->m_animations;
    if (registryMap == NULL) {
        return 0;
    }

    CPtrList toRemove;
    POSITION pos = registryMap->GetStartPosition();
    while (pos != NULL) {
        CString key;
        CAniElement* animation = NULL;
        MapGetNext(*registryMap, pos, key, animation);
        if (strncmp(static_cast<LPCTSTR>(key), "OBJECTZ_", 8) == 0) {
            CSpawnEntry* spawnEntry = m_spawnEntryList.FindByName(key);
            if (spawnEntry != NULL) {
                spawnEntry->m_flag = true;
            } else {
                toRemove.AddTail(animation);
            }
        }
    }

    pos = toRemove.GetHeadPosition();
    while (pos != NULL) {
        CAniElement* animation = static_cast<CAniElement*>(toRemove.GetNext(pos));
        surfaceMgr->m_animRegistry->RemoveAnimation(animation);
    }
    toRemove.RemoveAll();

    CSpawnList* spawnList = &m_spawnEntryList;
    CSpawnEntry* spawnEntry = spawnList->FirstEntry();
    while (spawnEntry != NULL) {
        if (spawnEntry->m_flag == false) {
            char resourcePath[0x80];
            sprintf(resourcePath, "ANIZ_%s", static_cast<LPCTSTR>(spawnEntry->GetTail()));
            CRezDir* resourceTree = src->GetDirFromPath(resourcePath);
            if (resourceTree == NULL) {
                return 0;
            }
            surfaceMgr->m_animRegistry->LoadFromTree(
                resourceTree,
                const_cast<char*>(static_cast<LPCTSTR>(spawnEntry->GetName())),
                "_"
            );
            TRACE("%s\n", static_cast<LPCTSTR>(spawnEntry->GetName()));
            spawnEntry->m_flag = true;
        }
        spawnEntry = spawnList->NextEntry();
    }
    return 1;
}

i32 CAreaMgr::InitializeArea1Stage1() {
    return 1;
}

i32 CAreaMgr::InitializeArea1Stage2() {
    return 1;
}

i32 CAreaMgr::InitializeArea1Stage3() {
    return 1;
}

i32 CAreaMgr::InitializeArea1Stage4() {
    return 1;
}

i32 CAreaMgr::InitializeArea2Stage1() {
    return 1;
}

i32 CAreaMgr::InitializeArea2Stage2() {
    return 1;
}

i32 CAreaMgr::InitializeArea2Stage3() {
    return 1;
}

i32 CAreaMgr::InitializeArea2Stage4() {
    return 1;
}

i32 CAreaMgr::InitializeArea3Stage1() {
    return 1;
}

i32 CAreaMgr::InitializeArea3Stage2() {
    return 1;
}

i32 CAreaMgr::InitializeArea3Stage3() {
    return 1;
}

i32 CAreaMgr::InitializeArea3Stage4() {
    return 1;
}

i32 CAreaMgr::InitializeArea4Stage1() {
    return 1;
}

i32 CAreaMgr::InitializeArea4Stage2() {
    return 1;
}

i32 CAreaMgr::InitializeArea4Stage3() {
    return 1;
}

i32 CAreaMgr::InitializeArea4Stage4() {
    return 1;
}

i32 CAreaMgr::InitializeArea5Stage1() {
    return 1;
}

i32 CAreaMgr::InitializeArea5Stage2() {
    return 1;
}

i32 CAreaMgr::InitializeArea5Stage3() {
    return 1;
}

i32 CAreaMgr::InitializeArea5Stage4() {
    return 1;
}

i32 CAreaMgr::InitializeArea6Stage1() {
    return 1;
}

i32 CAreaMgr::InitializeArea6Stage2() {
    return 1;
}

i32 CAreaMgr::InitializeArea6Stage3() {
    return 1;
}

i32 CAreaMgr::InitializeArea6Stage4() {
    return 1;
}

i32 CAreaMgr::InitializeArea7Stage1() {
    return 1;
}

i32 CAreaMgr::InitializeArea7Stage2() {
    return 1;
}

i32 CAreaMgr::InitializeArea7Stage3() {
    return 1;
}

i32 CAreaMgr::InitializeArea7Stage4() {
    return 1;
}

i32 CAreaMgr::InitializeArea8Stage1() {
    return 1;
}

i32 CAreaMgr::InitializeArea8Stage2() {
    return 1;
}

i32 CAreaMgr::InitializeArea8Stage3() {
    return 1;
}

i32 CAreaMgr::InitializeArea8Stage4() {
    return 1;
}

i32 CAreaMgr::InitializeReservedLevel33() {
    return 1;
}

i32 CAreaMgr::InitializeReservedLevel34() {
    return 1;
}

i32 CAreaMgr::InitializeReservedLevel35() {
    return 1;
}

i32 CAreaMgr::InitializeReservedLevel36() {
    return 1;
}

i32 CAreaMgr::InitializeTrainingStage1() {
    return 1;
}

i32 CAreaMgr::InitializeTrainingStage2() {
    return 1;
}

i32 CAreaMgr::InitializeTrainingStage3() {
    return 1;
}

i32 CAreaMgr::InitializeTrainingStage4() {
    return 1;
}

b32 CAreaMgr::IsSameWorld(i32 levelIndex) {
    if (levelIndex <= 0) {
        return false;
    }
    i32 requestedWorld = (levelIndex - 1) % 36 / 4 + 1;
    i32 currentWorld = (m_currentLevelIndex - 1) % 36 / 4 + 1;
    return currentWorld == requestedWorld;
}
