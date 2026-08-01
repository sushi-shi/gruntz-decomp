#include <rva.h>

#include <zlib.h>

// ===========================================================================
// 0x1853b0 - deflate the source block into dest at Z_DEFAULT_COMPRESSION (-1); on
// success store the produced length back through pDestLen and return deflateEnd's
// result. Returns the init error if deflateInit_ fails, Z_BUF_ERROR (-5) if deflate
// stops at Z_OK without reaching Z_STREAM_END, else the deflate error.
// NOTE the name is BACKWARDS and should be fixed by whoever owns the rename: this
// COMPRESSES. Its only retail caller is WwdFile_CompressMainBlock @0x160870, and the
// body is zlib's `compress()` (deflateInit_/deflate/deflateEnd), not `uncompress()`.
// Left alone here only because renaming it renames the TU and therefore the objdiff
// unit; it is a knowledge defect, not a matching one.
// ===========================================================================
// This IS zlib 1.0.4's `compress()`, verbatim - early return on the init error, the
// Z_BUF_ERROR ternary, `err = deflateEnd(); return err;` at the tail. Spelling it that
// way is worth 10.6 points over the equivalent nested/merged-exit forms, because each
// zlib statement is what splits `err`'s live range the way retail's allocator saw it:
// the init result never leaves eax (no `mov esi,eax`), the deflate result takes esi,
// and the ternary lowers to retail's `mov eax,-5 / test esi,esi / je / mov eax,esi`.
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
    s.zalloc = 0;
    s.zfree = 0;
    s.opaque = 0;

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
