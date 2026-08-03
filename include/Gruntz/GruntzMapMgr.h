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

    virtual i32 Visit(CFileMemBase* ar, SerialMode b, LogicTypeId c, i32 d) OVERRIDE;

    i32 LoadAttributes(i32 width, i32 height);

    CPtrArray m_arr;
    i32 m_reserved90;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CGRUNTZMAPMGR_H
