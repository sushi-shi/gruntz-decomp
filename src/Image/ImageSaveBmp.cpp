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
