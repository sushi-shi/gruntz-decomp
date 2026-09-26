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
    inline i32 TileIdAt(u32 x, u32 y) const;
    inline i32 OccupantAt(u32 x, u32 y) const;
    inline void ReleaseCellOccupancy(i32 tileX, i32 tileY);
    inline void AcquireCellOccupancy(i32 tileX, i32 tileY, i32 playerIndex, i32 unitIndex);
    inline SIZE
    GetGridSize() const;
    ~CGruntzMapMgr();

    virtual void Reset() OVERRIDE;

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload) OVERRIDE;

    i32 BuildCellAttributes(i32 width, i32 height);

    CPtrArray m_arr;
    i32 m_reserved90;
};

#endif // GRUNTZ_CGRUNTZMAPMGR_H
