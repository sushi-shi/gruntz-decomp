// WwdFile.h - Gruntz WWD level-file loader (Monolith-faithful reconstruction).
//
// On-disk WWD header layout is pinned in structure/formats/wwd.h; this header
// reproduces the load-bearing fields the loader actually touches.
#ifndef SRC_WWD_WWDFILE_H
#define SRC_WWD_WWDFILE_H

typedef unsigned char  Bytef;
typedef unsigned long  uLong;
typedef unsigned long  uLongf;

// WWD file header (1524 = 0x5F4 bytes on disk). Only the fields the loaders
// read are named; the rest is padding to preserve offsets. See formats/wwd.h.
struct WwdHeader
{
    unsigned int wwdSignature;      // +0x000  == 1524 (header size)
    unsigned int field_4;           // +0x004
    unsigned int flags;             // +0x008  bit1 (0x2) = COMPRESS
    unsigned char pad_c[0x2e8 - 0x0c];
    unsigned int mainBlockLength;   // +0x2E8  inflated main-block size
    unsigned char pad_2ec[0x5f4 - 0x2ec];
};

// zlib one-shot decompressor (matched, vendor/zlib-1.0.4/uncompr.c @0x185320).
extern "C" int uncompress(Bytef* dest, uLongf* destLen, const Bytef* source, uLong sourceLen);

// Inflate the WWD main block (planes/tiles) in place into a caller buffer.
// __stdcall free function (callee cleans 12 bytes).
extern "C" int __stdcall WwdFile_InflateMainBlock(WwdHeader* src, Bytef* dest, unsigned int destLen);

#endif // SRC_WWD_WWDFILE_H
