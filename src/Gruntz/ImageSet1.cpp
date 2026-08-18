#include <rva.h>

#include <Gruntz/ImageSets.h>

// @early-stop
RVA(0x00166d40, 0x24)
i32 CImageSet1::Parse(void* record) {
    i32* p = &static_cast<WwdTileImageRecord*>(record)->m_width;
    m_width = *p++;
    m_height = *p++;
    m_collisionValue = *p++;
    return 1;
}
