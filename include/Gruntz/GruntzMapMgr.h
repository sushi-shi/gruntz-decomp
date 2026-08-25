#ifndef GRUNTZ_CGRUNTZMAPMGR_H
#define GRUNTZ_CGRUNTZMAPMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/LogicTypeId.h>
#include <Gruntz/MapMgr.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

class CFileMemBase;

class CGruntzMapMgr : public CMapMgr {
public:
    ~CGruntzMapMgr();

    virtual void Reset() OVERRIDE;

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload) OVERRIDE;

    i32 BuildCellAttributes(i32 width, i32 height);

    CPtrArray m_arr;
    i32 m_reserved90;
};

#endif // GRUNTZ_CGRUNTZMAPMGR_H
