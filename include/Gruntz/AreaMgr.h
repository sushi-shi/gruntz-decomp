#ifndef SRC_GRUNTZ_AREAMGR_H
#define SRC_GRUNTZ_AREAMGR_H

#include <rva.h>

#include <Gruntz/SpawnList.h>
#include <Ints.h>

class CDDrawSurfaceMgr;
class CRezDir;

class CAreaMgr {
public:
    CAreaMgr();

    void Reset();

    i32 InitializeLevel(i32 index);

    b32 IsSameWorld(i32 levelIndex);

    ~CAreaMgr();

    i32 LoadObjectResources(CDDrawSurfaceMgr* surfaceMgr, CRezDir* src);
    i32 LoadObjectImageResources(CDDrawSurfaceMgr* surfaceMgr, CRezDir* src);
    i32 LoadObjectSoundResources(CDDrawSurfaceMgr* surfaceMgr, CRezDir* src);
    i32 LoadObjectAnimResources(CDDrawSurfaceMgr* surfaceMgr, CRezDir* src);

    i32 InitializeArea1Stage1();
    i32 InitializeArea1Stage2();
    i32 InitializeArea1Stage3();
    i32 InitializeArea1Stage4();
    i32 InitializeArea2Stage1();
    i32 InitializeArea2Stage2();
    i32 InitializeArea2Stage3();
    i32 InitializeArea2Stage4();
    i32 InitializeArea3Stage1();
    i32 InitializeArea3Stage2();
    i32 InitializeArea3Stage3();
    i32 InitializeArea3Stage4();
    i32 InitializeArea4Stage1();
    i32 InitializeArea4Stage2();
    i32 InitializeArea4Stage3();
    i32 InitializeArea4Stage4();
    i32 InitializeArea5Stage1();
    i32 InitializeArea5Stage2();
    i32 InitializeArea5Stage3();
    i32 InitializeArea5Stage4();
    i32 InitializeArea6Stage1();
    i32 InitializeArea6Stage2();
    i32 InitializeArea6Stage3();
    i32 InitializeArea6Stage4();
    i32 InitializeArea7Stage1();
    i32 InitializeArea7Stage2();
    i32 InitializeArea7Stage3();
    i32 InitializeArea7Stage4();
    i32 InitializeArea8Stage1();
    i32 InitializeArea8Stage2();
    i32 InitializeArea8Stage3();
    i32 InitializeArea8Stage4();
    i32 InitializeReservedLevel33();
    i32 InitializeReservedLevel34();
    i32 InitializeReservedLevel35();
    i32 InitializeReservedLevel36();
    i32 InitializeTrainingStage1();
    i32 InitializeTrainingStage2();
    i32 InitializeTrainingStage3();
    i32 InitializeTrainingStage4();

    i32 m_currentLevelIndex;
    CSpawnList m_spawnEntryList;
};

extern CAreaMgr g_areaMgr;
extern CAreaMgr* g_pAreaMgr;

#endif // SRC_GRUNTZ_AREAMGR_H
