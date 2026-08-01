#include <Image/ImagePaletteNode.h>
#include <Io/FileStream.h>
#include <Ints.h>
#include <rva.h>
#include <Image/ImagePool.h>
#include <DDrawMgr/DirPal.h>

namespace ApiCallerStubs {

    // @early-stop
    RVA(0x00177480, 0x169)
    i32 CImagePaletteNode::LoadBmpFile(char* path, i32 arg) {
        CFile f;
        if (f.Open(path, 0, 0) == 0) {
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
        return Build(out.m_entries, 0);
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
        void* data = LockResource(hRes);
        if (!data) {
            return 0;
        }
        return ProcessPal(data, arg);
    }

} // namespace ApiCallerStubs
