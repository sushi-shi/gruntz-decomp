// ImageSaveBmp.cpp - write an image + its palette to a BMP-style file (0x149250).
// __thiscall, ret 0x24: the first arg is a CString filename passed BY VALUE
// (callee-destroyed), followed by an 8-dword header block written verbatim. Gated
// on this->m_28 == 1, it opens the file (modeCreate|modeWrite = 0x9001), writes
// the header, the pixel buffer (m_c / m_10), then - when header word 1 bit 0x80
// is set and a palette is present - the 256-entry RGB palette (3 of every 4
// bytes). The CString + CFile stack objects force the /GX frame. Field names are
// placeholders; offsets + code bytes are the load-bearing fact.
#include <Ints.h>
#include <rva.h>

#include <Mfc.h> // CString (filename arg passed by value)

// The engine binary file writer (destructible stack local -> the /GX frame).
struct SaveFile {
    SaveFile();                                    // 0x1befd7
    ~SaveFile();                                   // 0x1bf121
    i32 Open(const char* name, i32 mode, i32 err); // 0x1bf200
    void Write(const void* buf, i32 len);          // 0x1bf362
    void Close();                                  // 0x1bf426
};

struct CImageSaver {
    i32 SaveBmp(
        CString name,
        i32 h0,
        i32 h1,
        i32 h2,
        i32 h3,
        i32 h4,
        i32 h5,
        i32 h6,
        i32 h7
    ); // 0x149250
    char p0[0xc];
    void* m_0c; // +0x0c  pixel buffer
    i32 m_10;   // +0x10  pixel buffer length
    char p14[0x20 - 0x14];
    u8* m_20; // +0x20  256-entry palette (BGRx)
    char p24[0x28 - 0x24];
    u8 m_28; // +0x28  ready flag
};

// @early-stop
// /GX CString-by-value + CFile-frame wall: the gate / header+buffer+palette write
// sequence is faithful, but the EH-state numbering across the two destructible
// stack objects + the by-value CString cleanup is not source-steerable.
RVA(0x00149250, 0x158)
i32 CImageSaver::
    SaveBmp(CString name, i32 h0, i32 h1, i32 h2, i32 h3, i32 h4, i32 h5, i32 h6, i32 h7) {
    if (m_28 != 1) {
        return 0;
    }

    SaveFile f;
    if (f.Open(name, 0x9001, 0) != 0) {
        f.Write(&h0, 0x20);
        f.Write(m_0c, m_10);
        if ((h1 & 0x80) && m_20 != 0) {
            for (i32 i = 0; i < 0x400; i += 4) {
                f.Write(&m_20[i], 1);
                f.Write(&m_20[i + 1], 1);
                f.Write(&m_20[i + 2], 1);
            }
        }
        f.Close();
        return 1;
    }
    return 0;
}
