#include <rva.h>

#include <Gruntz/ImageSetInline.h>

// @early-stop
RVA(0x00166c70, 0x24)
i32 CUniformTileImageSet::Parse(WwdTileImageRecord* record) {
    READ_TILE_IMAGE_DIMENSIONS(record, p)
    m_collisionValue = *p++;
    return 1;
}
