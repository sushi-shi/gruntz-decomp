#include <rva.h>

#include <Wap32/WapCompress.h>

#include <stddef.h>
#include <zlib.h>

RVA(0x001853b0, 0xa6)
int WapCompress(
    unsigned char* dest,
    unsigned long* pDestLen,
    unsigned char* src,
    unsigned long srcLen
) {
    z_stream s;
    s.next_in = src;
    s.avail_in = static_cast<unsigned int>(srcLen);
    s.next_out = dest;
    s.avail_out = static_cast<unsigned int>(*pDestLen);
    s.zalloc = NULL;
    s.zfree = NULL;
    s.opaque = NULL;

    int err = deflateInit_(&s, Z_DEFAULT_COMPRESSION, "1.0.4", sizeof(z_stream));
    if (err != Z_OK) {
        return err;
    }
    err = deflate(&s, Z_FINISH);
    if (err != Z_STREAM_END) {
        deflateEnd(&s);
        return err == Z_OK ? Z_BUF_ERROR : err;
    }
    *pDestLen = s.total_out;
    err = deflateEnd(&s);
    return err;
}
