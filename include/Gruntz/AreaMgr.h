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

    i32 Dispatch(i32 index);

    i32 SameGroup(i32 a);

    ~CAreaMgr();

    i32 LoadObjectResources(CDDrawSurfaceMgr* entry, CSymTab* src);
    i32 LoadObjectImageResources(CDDrawSurfaceMgr* entry, CSymTab* src);
    i32 LoadObjectSoundResources(CDDrawSurfaceMgr* entry, CSymTab* src);
    i32 LoadObjectAnimResources(CDDrawSurfaceMgr* entry, CSymTab* src);

    i32 H01();
    i32 H02();
    i32 H03();
    i32 H04();
    i32 H05();
    i32 H06();
    i32 H07();
    i32 H08();
    i32 H09();
    i32 H10();
    i32 H11();
    i32 H12();
    i32 H13();
    i32 H14();
    i32 H15();
    i32 H16();
    i32 H17();
    i32 H18();
    i32 H19();
    i32 H20();
    i32 H21();
    i32 H22();
    i32 H23();
    i32 H24();
    i32 H25();
    i32 H26();
    i32 H27();
    i32 H28();
    i32 H29();
    i32 H30();
    i32 H31();
    i32 H32();
    i32 H33();
    i32 H34();
    i32 H35();
    i32 H36();
    i32 H37();
    i32 H38();
    i32 H39();
    i32 H40();

    i32 m_currentAreaIndex;
    CSpawnList m_spawnEntryList;
};
SIZE(0x28);

extern CAreaMgr g_areaMgr;
extern CAreaMgr* g_pAreaMgr;

extern "C" i32 SpawnNameCmp(const char* a, const char* b, i32 n);

#endif // SRC_GRUNTZ_AREAMGR_H
