#include <StdAfx.h>
#include <Io/File.h>

#include <Ints.h>
#include <Image/Image.h>

#include <DDrawMgr/DirPal.h>
#include <Image/ImagePaletteNode.h>
#include <Image/ImagePool.h>
#include <Ints.h>
#include <Io/FileStream.h>

i32 CDibPal::InitBmp(const char* path, u32 flags) {
    io::File f;
    if (f.open(path, io::ReadOnly) == false) {
        return 0;
    }

    char fileHdr[14];
    if (f.read(fileHdr, 0xe) != 0xe) {
        return 0;
    }
    char infoHdr[40];
    if (f.read(infoHdr, 0x28) != 0x28) {
        return 0;
    }
    u8 raw[0x400];
    if (f.read(raw, 0x400) != 0x400) {
        return 0;
    }

    Palette256 out;
    for (i32 i = 0; i < 0x400; i += 4) {
        out.m_bytes[i + 0] = raw[i + 2];
        out.m_bytes[i + 1] = raw[i + 1];
        out.m_bytes[i + 2] = raw[i + 0];
        out.m_bytes[i + 3] = 0;
    }
    return Init(out.m_entries, flags);
}

i32 CDibPal::InitRes(const char* resourceName, u32 flags) {
    HINSTANCE mod = CDibMgr::GetGlobalInstanceHandle();
    if (!mod) {
        return 0;
    }
    HRSRC hRsrc = FindResourceA(mod, resourceName, "PALETTE");
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
    return Init(data, flags);
}
