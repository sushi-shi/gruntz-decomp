// ImageSaveBmp.cpp - write an image + its palette to a BMP-style file (0x149250).
// __thiscall, ret 0x24: the first arg is a CString filename passed BY VALUE
// (callee-destroyed), followed by an 8-dword header block written verbatim. Gated
// on this->m_28 == 1, it opens the file (modeCreate|modeWrite = 0x9001), writes
// the header, the pixel buffer (m_c / m_10), then - when header word 1 bit 0x80
// is set - the 256-entry RGB palette (3 of every 4 bytes); if the palette flag is
// set but m_palette is null it FAILS (return 0). The CString + CFile stack objects
// force the /GX frame; the failure paths are written as explicit early-returns so
// MSVC lays the blocks + EH-cleanup out exactly like retail. Field names are
// placeholders; offsets + code bytes are the load-bearing fact.
#include <Ints.h>
#include <rva.h>

#include <Mfc.h> // CString (filename arg passed by value)

// The engine binary file writer CFileIO (destructible stack local -> the /GX frame).
// 0x10-byte object stamped by its ctor 0x1befd7: vptr@0, file handle@4 (init -1),
// flag@8 (init 0), and a CString filename member @0xc (its m_pchData). Modeling the
// real size is load-bearing - an empty (1-byte) view shrinks the frame from
// `sub esp,0x10` to `push ecx` and desyncs every [esp+N].
struct SaveFile {
    char _vft0[4]; // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    i32 m_handle;  // +0x04  file handle (init -1)
    i32 m_flags;   // +0x08  (init 0)
    char* m_name;  // +0x0c  CString filename buffer (m_pchData)

    SaveFile();                                    // 0x1befd7
    ~SaveFile();                                   // 0x1bf121
    i32 Open(const char* name, i32 mode, i32 err); // 0x1bf200
    void Write(const void* buf, i32 len);          // 0x1bf362
    void Close();                                  // 0x1bf426
};

struct CImageSaver {
    i32 SaveBmp(
        CString name,
        i32 header0,
        i32 header1,
        i32 header2,
        i32 header3,
        i32 header4,
        i32 header5,
        i32 header6,
        i32 header7
    ); // 0x149250
    char p0[0xc];
    void* m_pixelBuffer;     // +0x0c  pixel buffer
    i32 m_pixelBufferLength; // +0x10  pixel buffer length
    char p14[0x20 - 0x14];
    u8* m_palette; // +0x20  256-entry palette (BGRx)
    char p24[0x28 - 0x24];
    u8 m_ready; // +0x28  ready flag
};

// @early-stop
// ~98% - the /GX frame, gate, block layout, EH-state numbering across the two
// destructible stack objects, and all three failure/return paths are byte-exact
// (correct SaveFile size 0x10 + explicit early-returns were the unblock, 18.8->98%).
// The sole residue is the palette-loop write's address computation: retail loads
// m_palette straight into ecx (`mov ecx,[edi+0x20]; add ecx,esi`) while MSVC5 here
// splits it (`mov eax,[edi+0x20]; mov ecx,esi; add ecx,eax`) - a register-choice
// scheduling artifact in `&m_palette[i]`, not source-steerable (entropy tail).
RVA(0x00149250, 0x158)
i32 CImageSaver::SaveBmp(
    CString name,
    i32 header0,
    i32 header1,
    i32 header2,
    i32 header3,
    i32 header4,
    i32 header5,
    i32 header6,
    i32 header7
) {
    if (m_ready != 1) {
        return 0;
    }

    SaveFile file;
    if (file.Open(name, 0x9001, 0) == 0) {
        return 0;
    }
    file.Write(&header0, 0x20);
    file.Write(m_pixelBuffer, m_pixelBufferLength);
    if (header1 & 0x80) {
        if (m_palette == 0) {
            return 0; // palette flag set but no palette -> fail
        }
        for (i32 i = 0; i < 0x400; i += 4) {
            file.Write(&m_palette[i], 1);
            file.Write(&m_palette[i + 1], 1);
            file.Write(&m_palette[i + 2], 1);
        }
    }
    file.Close();
    return 1;
}

SIZE_UNKNOWN(CImageSaver);
SIZE_UNKNOWN(SaveFile);
