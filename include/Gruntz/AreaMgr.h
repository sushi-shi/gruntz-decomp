#ifndef SRC_GRUNTZ_AREAMGR_H
#define SRC_GRUNTZ_AREAMGR_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SpawnList.h>

class CDDrawSurfaceMgr;
class CSymTab;

class CAreaMgr {
public:
    CAreaMgr();

    void Reset();

    i32 InitializeLevel(i32 index);

    i32 IsSameWorld(i32 a);

    ~CAreaMgr();

    i32 LoadObjectResources(CDDrawSurfaceMgr* entry, CSymTab* src);
    i32 LoadObjectImageResources(CDDrawSurfaceMgr* entry, CSymTab* src);
    i32 LoadObjectSoundResources(CDDrawSurfaceMgr* entry, CSymTab* src);
    i32 LoadObjectAnimResources(CDDrawSurfaceMgr* entry, CSymTab* src);

    i32 InitializeLevel01();
    i32 InitializeLevel02();
    i32 InitializeLevel03();
    i32 InitializeLevel04();
    i32 InitializeLevel05();
    i32 InitializeLevel06();
    i32 InitializeLevel07();
    i32 InitializeLevel08();
    i32 InitializeLevel09();
    i32 InitializeLevel10();
    i32 InitializeLevel11();
    i32 InitializeLevel12();
    i32 InitializeLevel13();
    i32 InitializeLevel14();
    i32 InitializeLevel15();
    i32 InitializeLevel16();
    i32 InitializeLevel17();
    i32 InitializeLevel18();
    i32 InitializeLevel19();
    i32 InitializeLevel20();
    i32 InitializeLevel21();
    i32 InitializeLevel22();
    i32 InitializeLevel23();
    i32 InitializeLevel24();
    i32 InitializeLevel25();
    i32 InitializeLevel26();
    i32 InitializeLevel27();
    i32 InitializeLevel28();
    i32 InitializeLevel29();
    i32 InitializeLevel30();
    i32 InitializeLevel31();
    i32 InitializeLevel32();
    i32 InitializeLevel33();
    i32 InitializeLevel34();
    i32 InitializeLevel35();
    i32 InitializeLevel36();
    i32 InitializeLevel37();
    i32 InitializeLevel38();
    i32 InitializeLevel39();
    i32 InitializeLevel40();

    i32 m_currentLevelIndex;
    CSpawnList m_spawnEntryList;
};
SIZE(0x28);

extern CAreaMgr g_areaMgr;
extern CAreaMgr* g_pAreaMgr;

#endif // SRC_GRUNTZ_AREAMGR_H
