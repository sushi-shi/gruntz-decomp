#ifndef GRUNTZ_IMAGESETINLINE_H
#define GRUNTZ_IMAGESETINLINE_H

#include <Gruntz/ImageSets.h>

#define READ_TILE_IMAGE_DIMENSIONS(record, fields)                                                 \
    i32* fields = &record->m_width;                                                                \
    m_width = *fields++;                                                                           \
    m_height = *fields++;

#endif // GRUNTZ_IMAGESETINLINE_H
