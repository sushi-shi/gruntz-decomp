// ImageSaveBmp.cpp - CDDrawShadeBlit::DecodeFrame (0x149250): write the decoded
// shaded-sprite (its RLE pixel buffer + palette) out to a BMP-style file.
// __thiscall, ret 0x24: the filename is a CString passed BY VALUE (callee-destroyed),
// followed by the 8-dword CImageFrameRebuildDesc header block written verbatim. Gated
// on m_srcBpp == 1 (the 8bpp path), it opens the file (modeCreate|modeWrite = 0x9001),
// writes the header, the RLE pixel buffer (m_rleData / m_rleLen), then - when header
// word 1 bit 0x80 is set - the 256-entry RGB palette (3 of every 4 bytes); if the
// palette flag is set but m_palette is null it FAILS (return 0). The CString + CFileIO
// stack objects force the /GX frame; the failure paths are written as explicit
// early-returns so MSVC lays the blocks + EH-cleanup out exactly like retail.
//
// (Former per-TU views dissolved: `CImageSaver` was CDDrawShadeBlit - its m_pixelBuffer/
// m_pixelBufferLength/m_palette/m_ready were m_rleData@0xc / m_rleLen@0x10 / m_palette@0x20 /
// m_srcBpp@0x28; the fake `?SaveBmp@CImageSaver@@` name also left CDDrawShadeBlit::Rebuild's
// DecodeFrame call unresolved. `SaveFile` was the MFC CFile (CFileIO) - ctor 0x1befd7 =
// ??0CFile@@QAE@XZ.)
#include <Ints.h>
#include <rva.h>

#include <Mfc.h> // CString (filename arg passed by value) + CFile

#include <DDrawMgr/DDrawShadeBlit.h> // CDDrawShadeBlit + CImageFrameRebuildDesc (the real class)
#include <Io/FileStream.h>           // CFileIO == the MFC CFile (destructible stack local -> /GX)

// @early-stop
// ~98% - the /GX frame, gate, block layout, EH-state numbering across the two
// destructible stack objects, and all three failure/return paths are byte-exact
// (correct CFile size 0x10 + explicit early-returns were the unblock, 18.8->98%).
// The sole residue is the palette-loop write's address computation: retail loads
// m_palette straight into ecx (`mov ecx,[edi+0x20]; add ecx,esi`) while MSVC5 here
// splits it (`mov eax,[edi+0x20]; mov ecx,esi; add ecx,eax`) - a register-choice
// scheduling artifact in `&m_palette[i]`, not source-steerable (entropy tail).
RVA(0x00149250, 0x158)
i32 CDDrawShadeBlit::DecodeFrame(CString name, CImageFrameRebuildDesc desc) {
    if (m_srcBpp != 1) {
        return 0;
    }

    CFileIO file;
    if (file.Open(name, 0x9001, 0) == 0) {
        return 0;
    }
    file.Write(&desc, 0x20);
    file.Write(m_rleData, m_rleLen);
    if (desc.f1 & 0x80) {
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
