#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DirPal.h>
#include <Image/ImagePaletteNode.h>
#include <Image/ImagePool.h>
#include <Ints.h>
#include <Io/FileStream.h>

RVA(0x00177480, 0x169)
i32 CImagePaletteNode::LoadBmpFile(char* path, i32 arg) {
    CFile f;
    if (f.Open(path, 0, NULL) == 0) {
        return 0;
    }

    char fileHdr[14];
    if (f.Read(fileHdr, 0xe) == 0) {
        return 0;
    }
    char infoHdr[40];
    if (f.Read(infoHdr, 0x28) == 0) {
        return 0;
    }
    u8 raw[0x400];
    if (f.Read(raw, 0x400) == 0) {
        return 0;
    }

    Palette256 out;
    for (i32 i = 0; i < 0x400; i += 4) {
        out.m_bytes[i + 0] = raw[i + 2];
        out.m_bytes[i + 1] = raw[i + 1];
        out.m_bytes[i + 2] = raw[i + 0];
        out.m_bytes[i + 3] = 0;
    }
    return Build(out.m_entries, arg);
}

RVA(0x001775f0, 0x62)
i32 CImagePaletteNode::Apply(char* path, i32 arg) {
    HINSTANCE mod = g_hResModule;
    if (!mod) {
        return 0;
    }
    HRSRC hRsrc = FindResourceA(mod, path, "PALETTE");
    if (!hRsrc) {
        return 0;
    }
    HGLOBAL hRes = LoadResource(mod, hRsrc);
    if (!hRes) {
        return 0;
    }
    u8* data = static_cast<u8*>(LockResource(hRes));
    if (!data) {
        return 0;
    }
    return ProcessPal(data, arg);
}
