#include <rva.h>

#include <Gruntz/ImageSetInline.h>

// @early-stop
RVA(0x00166d40, 0x24)
i32 CUniformTileImageSet::Parse(WwdTileImageRecord* record) {
    i32* p = ReadDimensions(record);
    m_collisionValue = *p++;
    return 1;
}
