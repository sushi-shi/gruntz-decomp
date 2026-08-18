#include <rva.h>

#include <Gruntz/ImageSets.h>

// @early-stop
RVA(0x00166d40, 0x24)
i32 CImageSet1::Parse(WwdTileImageRecord* record) {
    i32* p = &record->m_width;
    m_width = *p++;
    m_height = *p++;
    m_collisionValue = *p++;
    return 1;
}
