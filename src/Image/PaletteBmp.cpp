#include <Image/ImagePaletteNode.h> // ApiCallerStubs::CImagePaletteNode (Build/ProcessPal)
#include <Io/FileStream.h>          // CFile - the engine KERNEL32 file reader (0x1befd7..)
#include <Ints.h>
#include <rva.h>
#include <Image/ImagePool.h> // g_hResModule (ex .cpp extern)
#include <DDrawMgr/DirPal.h> // Palette256 - the 0x400-byte colour table's two views

namespace ApiCallerStubs {

    // @early-stop
    // /GX CFile-frame plateau: the BMP read + BGR->RGB swap + Build hand-off are
    // faithful; the exact stack-buffer offset assignment under the EH frame is not
    // source-steerable.
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

        // retail's swizzle @0x177575 is a FLAT byte loop over the 0x400-byte BMP colour
        // table (`mov cl,[esp+eax+0x452]` ... `add eax,4; cmp eax,0x400`); Palette256
        // names both readings of those bytes, so the Build hand-off needs no pun.
        Palette256 out;
        for (i32 i = 0; i < 0x400; i += 4) {
            out.m_bytes[i + 0] = raw[i + 2];
            out.m_bytes[i + 1] = raw[i + 1];
            out.m_bytes[i + 2] = raw[i + 0];
            out.m_bytes[i + 3] = 0;
        }
        return Build(out.m_entries, 0);
    }

    // __thiscall(path, arg): find/load/lock a PALETTE resource from the app resource
    // module, then hand the locked data on to ProcessPal (0x176e70).
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
