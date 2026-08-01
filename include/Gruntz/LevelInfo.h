#ifndef SRC_GRUNTZ_LEVELINFO_H
#define SRC_GRUNTZ_LEVELINFO_H

#include <Ints.h>
#include <rva.h>

class CDDrawChildGroup;

class CMapMgr;
class CTriggerMgr;
struct CGameObject;
class CTileTriggerContainer;

class CDDrawWorkerHost;

struct CLevelViewHolder {
    char m_pad00[0x5c];
    CDDrawWorkerHost* m_5c;
};
SIZE_UNKNOWN();
struct CLevelList {
    char m_pad00[0x8];
    CDDrawChildGroup* m_coll;
    char m_pad0c[0x24 - 0xc];
    CLevelViewHolder* m_view;
};
SIZE_UNKNOWN();

struct CLevelInfo {
    char m_pad00[0x4];
    i32 m_levelNum;
    char m_pad08[0x10 - 0x8];

    CGameObject* m_10;
    char m_pad14[0x2c - 0x14];
    class CPlay* m_spawnInfo;
    CLevelList* m_objList;
    char m_pad34[0x35 - 0x34];
    char m_path[0x68 - 0x35];
    CTriggerMgr* m_triggerMgr;
    char m_pad6c[0x70 - 0x6c];
    CMapMgr* m_dims;
    char m_pad74[0x75 - 0x74];
    char m_name[0xf8 - 0x75];
    i32 m_isCustom;
    i32 m_isBattlez;
};
SIZE_UNKNOWN();

#endif // SRC_GRUNTZ_LEVELINFO_H
