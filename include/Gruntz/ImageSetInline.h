#ifndef GRUNTZ_IMAGESETINLINE_H
#define GRUNTZ_IMAGESETINLINE_H

#include <Gruntz/ImageSets.h>

#define READ_TILE_IMAGE_DIMENSIONS(record, fields)                                                 \
    m_width = (record)->m_width;                                                                   \
    m_height = (record)->m_height;                                                                 \
    i32* fields = (record)->m_fields;

#endif // GRUNTZ_IMAGESETINLINE_H
