#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <DDrawMgr/DDrawSurfaceMgr.h>
#include <DDrawMgr/DDrawWorkerHost.h>
#include <Enums.h>
#include <Gruntz/GameLevel.h>
#include <Gruntz/ImageSets.h>
#include <Gruntz/LogicTypeId.h>
#include <Gruntz/ParseSource.h>
#include <Gruntz/SerialArchive.h>
#include <Gruntz/UserLogic.h>
#include <Io/FileMem.h>
#include <Io/FileStream.h>
#include <Wap32/Object.h>
#include <Wwd/WwdFile.h>

#include <stdlib.h>
#include <string.h>

static const i32 AXIS_UNSET = static_cast<i32>(0x80000000);

static inline void StampParamBlock(CGameLevel* o) {
    o->m_pairA[0] = 500;
    o->m_pairA[1] = 250;
    o->m_pairB[0] = 1000;
    o->m_pairB[1] = 1000;
    o->m_pairC[0] = 250;
    o->m_pairC[1] = 125;
    o->m_rectA.w = 1600;
    o->m_rectA.h = 1200;
    o->m_rectB.w = 2560;
    o->m_rectB.h = 1920;
    o->m_rectC.w = 768;
    o->m_rectC.h = 576;
}

// @early-stop
RVA(0x00082600, 0x73)
i32 CGameLevel::LookupTile(i32 x, i32 y) {
    CDDrawWorkerHost* mp;
    if (x < 0) {
        x = 0;
    } else {
        mp = m_mainPlane;
        if (x >= mp->m_gridW) {
            x = mp->m_gridW - 1;
        }
    }
    if (y < 0) {
        y = 0;
    } else {
        mp = m_mainPlane;
        if (y >= mp->m_gridH) {
            y = mp->m_gridH - 1;
        }
    }
    mp = m_mainPlane;
    i32 tile = mp->m_tileGrid[mp->m_colOffsets[y] + x];
    if (tile == TILE_UNINIT || tile == TILE_CLEAR) {
        return 0;
    }
    CTileImageSet* set = static_cast<CTileImageSet*>(m_imageSets[tile & 0xffff]);
    return set->GetCollisionAt(0, 0);
}
