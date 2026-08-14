#include <rva.h>

#include <Wap32/WapUncompress.h>

#include <stddef.h>
#include <zlib.h>

RVA(0x001853b0, 0xa6)
int WapUncompress(
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

    int err = deflateInit_(&s, -1, "1.0.4", sizeof(z_stream));
    if (err != 0) {
        return err;
    }
    err = deflate(&s, 4);
    if (err != 1) {
        deflateEnd(&s);
        return err == 0 ? -5 : err;
    }
    *pDestLen = s.total_out;
    err = deflateEnd(&s);
    return err;
}
