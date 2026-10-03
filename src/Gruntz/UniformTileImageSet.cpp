#include <StdAfx.h>

#include <Ints.h>

#include <Gruntz/ImageSetInline.h>

i32 CUniformTileImageSet::Parse(WwdTileImageRecord* record) {
    READ_TILE_IMAGE_DIMENSIONS(record, p)
    m_collisionValue = *p++;
    return 1;
}
