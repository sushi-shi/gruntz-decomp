#ifndef GRUNTZ_PIX16_H
#define GRUNTZ_PIX16_H

#include <Ints.h>

// A 16bpp raster cursor.
//
// Locked DirectDraw surfaces, the shade CLUT and the packed image rows are all
// ADDRESSED in bytes - the surface pitch, the CLUT bank stride and the table
// offsets are byte quantities - while the values at those addresses are 16 bits
// wide. Both readings of the one pointer are real, so they get names here instead
// of a pun at every load and store.
//
// Assign the BYTE arm, read the WORD arm: keeping the byte expression verbatim is
// load-bearing. Routing the CLUT reads through a converting accessor instead
// re-associates `g_clut + (off)` into `(g_clut + off) + bank` and cost
// ShadeBlt/ShadeRect 3-7% when it was tried.
union Pix16Ptr {
    u8* m_bytes;
    char* m_chars; // the same cursor where the source hands back char* (Lock, CFile)
    u16* m_words;
    i16* m_swords; // the signed spelling the raster globals use
    i32* m_dwords; // record tables walked by BYTE stride whose fields are dwords
};

// A file/wire RECORD pointer and the byte cursor over the same buffer. Formats whose
// payload sits at a RUNTIME offset - BMP's bfOffBits, a PCX/PID trailing palette, the
// colour table after a variable-length info header - can only be reached by byte
// arithmetic, so both readings of the buffer's base are named rather than punned.
union RecordBytes {
    void* m_rec;
    u8* m_bytes;
    char* m_chars;
    i32* m_dwords; // bands the engine reads as a raw dword run
};

// The read-only twin (Load16-style probes off a const byte cursor).
union Pix16CPtr {
    const u8* m_bytes;
    const char* m_chars;
    const u16* m_words;
    const i16* m_swords;
    const i32* m_dwords; // record tables walked by BYTE stride whose fields are dwords
};

#endif // GRUNTZ_PIX16_H
