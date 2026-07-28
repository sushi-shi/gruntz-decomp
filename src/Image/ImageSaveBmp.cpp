#include <Ints.h>
#include <rva.h>

#include <Mfc.h> // CString (filename arg passed by value) + CFile

#include <DDrawMgr/DDrawShadeBlit.h> // CDDrawShadeBlit + CImageFrameRebuildDesc (the real class)
#include <Io/FileStream.h>           // CFile == the MFC CFile (destructible stack local -> /GX)

RVA(0x00149250, 0x158)
i32 CDDrawShadeBlit::DecodeFrame(CString name, CImageFrameRebuildDesc desc) {
    if (m_srcBpp != 1) {
        return 0;
    }

    CFile file;
    if (file.Open(name, 0x9001, 0) == 0) {
        return 0;
    }
    file.Write(&desc, 0x20);
    file.Write(m_rleData, m_rleLen);
    if (desc.f1 & 0x80) {
        if (m_palette == 0) {
            return 0; // palette flag set but no palette -> fail
        }
        for (i32 i = 0; i < 0x100; i++) {
            file.Write(&m_palette[i].peRed, 1);
            file.Write(&m_palette[i].peGreen, 1);
            file.Write(&m_palette[i].peBlue, 1);
        }
    }
    file.Close();
    return 1;
}
