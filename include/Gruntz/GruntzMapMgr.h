#ifndef GRUNTZ_CGRUNTZMAPMGR_H
#define GRUNTZ_CGRUNTZMAPMGR_H

#include <Ints.h>
#include <Mfc.h>
#include <Gruntz/MapMgr.h>
#include <rva.h>

class CFileMemBase;

class CGruntzMapMgr : public CMapMgr {
public:
    ~CGruntzMapMgr();

    virtual void Reset() OVERRIDE;

    virtual i32 Visit(CFileMemBase* ar, i32 b, i32 c, i32 d) OVERRIDE;

    i32 LoadAttributes(i32 width, i32 height);

    CPtrArray m_arr;
    i32 m_90;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_CGRUNTZMAPMGR_H
