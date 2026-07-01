// Blk6c.h - the 0x6c-byte CImageOwned image-transform descriptor block (27 i32s):
// d[0] = block size (0x6c), d[1] = transform mode, d[0x1a] = flags; the Apply path
// (0x13e0a0) copies it into CImageOwned+0x10. ONE definition shared by the
// BoundaryUpper*.cpp orphan-COMDAT cluster TUs (identical layout in both).
#ifndef GRUNTZ_GRUNTZ_BLK6C_H
#define GRUNTZ_GRUNTZ_BLK6C_H

#include <Ints.h>

#include <rva.h>

struct Blk6c {
    i32 d[0x1b];
};
SIZE_UNKNOWN(Blk6c);

#endif // GRUNTZ_GRUNTZ_BLK6C_H
