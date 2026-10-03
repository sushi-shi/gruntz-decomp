#ifndef GRUNTZ_CGRUNTZMAPMGR_H
#define GRUNTZ_CGRUNTZMAPMGR_H

#include <vector>

#include <Ints.h>

#include <Gruntz/Brickz.h>
#include <Gruntz/CoordPool.h>
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

    virtual void Reset()  ;

    virtual i32
    SerializeDispatch(CFileMemBase* ar, SerialMode mode, LogicTypeId typeId, i32 payload)  ;

    i32 BuildCellAttributes(i32 width, i32 height);

    std::vector<Coord*> m_arr;

    i32 m_reserved90;
};

inline void CGruntzMapMgr::Reset() {
    for (i32 i = 0; i < static_cast<i32>(m_arr.size()); i++) {
        Coord* elem = static_cast<Coord*>(m_arr[i]);
        if (elem != NULL) {
            g_coordPool.Push(elem);
        }
    }
    m_arr.clear();
    CMapMgr::Reset();
}

#endif
