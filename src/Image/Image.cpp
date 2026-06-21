// Image.cpp - the engine's REZ -> image resolution path.
//
// Functions matched in this TU:
//   CImage::LoadFromRez  - ext dispatcher
//   CFileImage::LoadBmp  - .BMP file loader
//   CFileImage::LoadPcx  - .PCX file loader
//   CFileImage::LoadPid  - .PID file loader
//
// LoadFromRez is the file-extension DISPATCHER (the same idiom as
// RezMgr::MakeImageKey): take ext = strrchr(name,'.'), then a stricmp
// ladder on ".BMP"/".PCX"/".RID"/".PID", forwarding (name,a2,a3) verbatim to the
// matching sibling loader; no/unknown extension -> the default loader. The four
// extension literals are reloc-masked file-scope string globals; strrchr/stricmp
// are the engine's CRT helpers, called reloc-masked.
//
// CFileImage::Load{Bmp,Pcx,Pid} are the actual file consumers: construct a stack
// CFileIO, Open(path,0,0), GetLength(), `operator new` a buffer, Read it, hand it
// to a per-format decode helper, free the buffer, return the decoder's result.
// They reuse the already-matched CFileIO class (Open/Read/GetLength/ctor/dtor are
// reloc-masked engine calls) and the global operator new/delete. The CFileIO
// stack object carries a dtor -> a C++ EH frame -> this TU builds with /GX.
#include <Image/Image.h>
#include <rva.h>
// <string.h>: strrchr (find the ext dot) / _stricmp (the case-insensitive ext compare).
#include <string.h>

// The four file-extension literals (reloc-masked .rdata globals). Declared at
// file scope so each `push OFFSET` matches the binary's direct-address push.
static const char s_extBmp[] = ".BMP";
static const char s_extPcx[] = ".PCX";
static const char s_extRid[] = ".RID";
static const char s_extPid[] = ".PID";

// ---------------------------------------------------------------------------
// CImage::LoadFromRez
// ext = strrchr(name,'.'); dispatch on .BMP/.PCX/.RID/.PID, else default. Each
// branch re-tests `ext != 0` (the target's `test esi; je default` per case) and
// forwards (name,a2,a3); a matched ext returns its loader's result directly.
RVA(0x175a90, 0xee)
int CImage::LoadFromRez(char* name, void* a2, void* a3) {
    char* ext = strrchr(name, '.');

    if (ext && _stricmp(ext, s_extBmp) == 0) {
        return LoadBmp(name, a2, a3);
    } else if (ext && _stricmp(ext, s_extPcx) == 0) {
        return LoadPcx(name, a2, a3);
    } else if (ext && _stricmp(ext, s_extRid) == 0) {
        return LoadRid(name, a2, a3);
    } else if (ext && _stricmp(ext, s_extPid) == 0) {
        return LoadPid(name, a2, a3);
    }

    return LoadDefault(name, a2, a3);
}

// The resource module the .DEFAULT loader pulls RT_BITMAP resources from
// (reloc-masked .data global; 0 until the engine records the instance handle).
DATA(0x2bf6e0)
extern "C" HINSTANCE g_hResModule; // 0x6bf6e0

// ---------------------------------------------------------------------------
// CImage::LoadBmp
// The .BMP loader: open the file, read the 14-byte BITMAPFILEHEADER and the
// 40-byte BITMAPINFOHEADER, hand the parsed (width, height, bitcount, a2, a3)
// to the decode helper that allocates the CImage's pixel plane, then Seek to
// bfOffBits and Read exactly (bitcount/8)*stride*height pixel bytes into the
// plane. Returns 1 on a full read, 0 on any I/O / decode failure. The CFileIO
// stack object's dtor runs on every exit -> the C++ EH frame.
RVA(0x175e40, 0x1b3)
int CImage::LoadBmp(char* name, void* a2, void* a3) {
    CFileIO file;
    BITMAPFILEHEADER fh;
    BITMAPINFOHEADER ih;

    if (!file.Open(name, 0, 0)) {
        return 0;
    }
    if (file.Read(&fh, sizeof(fh)) == 0) {
        return 0;
    }
    if (file.Read(&ih, sizeof(ih)) == 0) {
        return 0;
    }

    int height = ih.biHeight;
    int width = ih.biWidth;
    int bitcount = ih.biBitCount & 0xffff;
    if (!DecodeBmpHeader(a2, width, height, bitcount, a3)) {
        return 0;
    }

    file.Seek(fh.bfOffBits, 0);
    unsigned int size = (bitcount / 8) * m_444 * height;
    if (file.Read(m_42c, size) != size) {
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CImage::LoadPcx
// Open the file, GetLength(); if zero return 0. `operator new` a buffer of that
// size; if it fails return 0. Read the whole file, hand the buffer (+a2,a3) to
// the PCX decode helper, free the buffer and return the decoder's result.
RVA(0x176190, 0x126)
int CImage::LoadPcx(char* name, void* a2, void* a3) {
    CFileIO file;

    if (!file.Open(name, 0, 0)) {
        return 0;
    }
    unsigned int len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    int result = DecodePcxData(buf, a2, a3);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CImage::LoadRid
// Byte-identical to LoadPcx except for the per-format decode helper (the .RID
// reader DecodeRidData).
RVA(0x176310, 0x126)
int CImage::LoadRid(char* name, void* a2, void* a3) {
    CFileIO file;

    if (!file.Open(name, 0, 0)) {
        return 0;
    }
    unsigned int len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    int result = DecodeRidData(buf, a2, a3);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CImage::LoadPid
// Byte-identical to LoadPcx/LoadRid except for the .PID decode helper.
RVA(0x1766a0, 0x126)
int CImage::LoadPid(char* name, void* a2, void* a3) {
    CFileIO file;

    if (!file.Open(name, 0, 0)) {
        return 0;
    }
    unsigned int len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    int result = DecodePidData(buf, a2, a3);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CImage::LoadDefault
// The fallback (no/unknown extension): pull the named RT_BITMAP resource from
// the engine's resource module and decode it in place. Returns 0 unless the
// module handle is set and FindResource/LoadResource/LockResource all succeed.
RVA(0x1767d0, 0x64)
int CImage::LoadDefault(char* name, void* a2, void* a3) {
    HINSTANCE hModule = g_hResModule;
    if (!hModule) {
        return 0;
    }
    HRSRC hRsrc = FindResourceA(hModule, name, (LPCSTR)RT_BITMAP);
    if (!hRsrc) {
        return 0;
    }
    HGLOBAL hGlobal = LoadResource(hModule, hRsrc);
    if (!hGlobal) {
        return 0;
    }
    void* data = LockResource(hGlobal);
    if (!data) {
        return 0;
    }
    return DecodeResData(data, a2, a3);
}

// ---------------------------------------------------------------------------
// CFileImage::LoadBmp
// Open the file named by `path`; on failure return 0. GetLength(); if the length
// is zero return 0. `operator new` a buffer of that size; if it fails return 0.
// Read the file; if the read count != length, free + return 0. Else decode and
// return the decoder's result. The CFileIO dtor + buffer free run on every exit.
RVA(0x144110, 0x156)
void* CFileImage::LoadBmp(char* name, char* path) {
    CFileIO file;

    if (!file.Open(path, 0, 0)) {
        return 0;
    }

    unsigned int len = file.GetLength();
    if (len == 0) {
        return 0;
    }

    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    void* result = DecodeBmp(name, buf, len);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CFileImage::LoadPcx
// Byte-identical to LoadBmp except for the per-format decode helper (DecodePcx).
RVA(0x145110, 0x156)
void* CFileImage::LoadPcx(char* name, char* path) {
    CFileIO file;

    if (!file.Open(path, 0, 0)) {
        return 0;
    }

    unsigned int len = file.GetLength();
    if (len == 0) {
        return 0;
    }

    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    void* result = DecodePcx(name, buf, len);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CFileImage::LoadPid
// Like LoadBmp/LoadPcx, but: (1) it does NOT guard length==0 - it allocates the
// buffer for whatever GetLength() returns and only null-checks the allocation;
// (2) the decoder takes a fourth pass-through arg (a3).
RVA(0x145cd0, 0x130)
void* CFileImage::LoadPid(char* name, char* path, void* a3) {
    CFileIO file;

    if (!file.Open(path, 0, 0)) {
        return 0;
    }

    unsigned int len = file.GetLength();
    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    void* result = DecodePid(name, buf, len, a3);
    operator delete(buf);
    return result;
}

// ===========================================================================
// CFileImage::DecodePcxEx
//
// Opens a PCX file, reads data, calls DecodePcxData.
// ===========================================================================
RVA(0x1459d0, 0x135)
int CFileImage::DecodePcxEx(char* name, char* path, void* a3, void* a4) {
    CFileIO file;

    if (!file.Open(path, 0, 0)) {
        return 0;
    }

    unsigned int len = file.GetLength();
    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    int result = DecodePcxData(name, buf, len, a3, a4);
    operator delete(buf);
    return result;
}
