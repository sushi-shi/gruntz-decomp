// Image.cpp - the engine's REZ -> image resolution path.
//
// Functions matched in this TU:
//   CImage::LoadFromRez  - ext dispatcher
//   CImage::Load{Bmp,Pcx,Rid,Pid,Default} + the per-format decoders
//     (DecodeBmpHeader/DecodeResData/DecodePcxData/DecodeRidData/DecodePidData)
//   CFileImage::LoadBmp  - .BMP file loader
//   CFileImage::LoadPcx  - .PCX file loader
//   CFileImage::LoadPid  - .PID file loader
//
// The decoders are the bit/byte-twiddling readers the loaders call: DecodeBmpHeader
// is the shared plane allocator (fills the in-place BITMAPINFOHEADER, CreateDIBSections
// the pixel plane + builds the bottom-up per-row offset table); DecodeResData/RidData
// hand pre-decoded pixels to the shared blitter (DecodeBlit @0x175930, external/no-body);
// DecodePcxData/PidData RLE-decode .PCX/.PID into the plane. The .PID format (flags@4,
// width@8, height@0xc, COMPRESSION=0x20) matches Monolith's WAP32 layout (libwap32's
// wap32/pid.h). The decoders are entropy-prone register/scheduling tails: the small
// ones land ~96%, the RLE ones ~73-91% with logically-faithful but differently-allocated
// codegen (the documented entropy plateau, not a reconstruction error).
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
// CImage::DecodeBmpHeader
// The plane allocator/setup shared by every format. Records the image geometry
// (width/abs(height)/bitcount and the aligned destination stride) into the
// engine fields at this+0x434.., builds an in-place BITMAPINFOHEADER (this+0)
// with an identity DIB_PAL_COLORS table for 8bpp, CreateDIBSections the pixel
// plane (HBITMAP @+0x428, bits @+0x42c), and operator-new's the bottom-up
// per-row byte-offset table (this+0x430). Returns 0 if CreateDIBSection fails.
RVA(0x1757c0, 0x16f)
int CImage::DecodeBmpHeader(void* a2, int width, int height, int bitcount, void* a3) {
    m_434 = 0;
    m_438 = width;
    m_43c = (height < 0) ? -height : height;
    m_440 = bitcount;
    if (bitcount == 8) {
        m_444 = ((width + 3) / 4) * 4;
    } else {
        m_444 = width;
    }
    m_448 = m_444 - width;
    m_454 = 0;
    m_458 = 0;
    m_450 = 1;
    memset(&m_bih, 0, sizeof(BITMAPINFOHEADER));
    m_bih.biWidth = m_438;
    m_bih.biBitCount = (WORD)m_440;
    m_bih.biSize = sizeof(BITMAPINFOHEADER);
    m_bih.biHeight = height;
    m_bih.biPlanes = 1;
    m_bih.biCompression = 0;
    m_bih.biSizeImage = 0;
    m_bih.biClrUsed = 0;
    m_bih.biClrImportant = 0;
    if (m_440 == 8) {
        for (int i = 0; i < 256; i++) {
            m_pal[i] = (unsigned short)i;
        }
        m_428 = CreateDIBSection((HDC)a2, (BITMAPINFO*)this, DIB_PAL_COLORS, &m_42c, 0, 0);
    } else {
        m_428 = CreateDIBSection((HDC)a2, (BITMAPINFO*)this, DIB_RGB_COLORS, &m_42c, 0, 0);
    }
    if (!m_428) {
        return 0;
    }
    m_430 = (int*)operator new(m_43c * 4);
    for (int i = 0; i < m_43c; i++) {
        m_430[i] = (m_43c - i - 1) * (m_440 / 8) * m_444;
    }
    return 1;
}

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
// CImage::DecodeResData
// The RT_BITMAP / .DEFAULT decoder: `buf` points at a packed DIB
// (BITMAPINFOHEADER + palette + pixels). Pull biWidth/biHeight/biBitCount, point
// `src` at the pixel bytes (for 8bpp the 256-entry RGBQUAD palette pushes them to
// buf+biSize+0x400, else right after the 0x28 header + the 4 RGBQUAD masks at
// buf+0x2c) and hand it to the shared blitter.
RVA(0x175e00, 0x3d)
int CImage::DecodeResData(void* buf, void* a2, void* a3) {
    unsigned char* hdr = (unsigned char*)buf;
    int bitcount = *(unsigned short*)(hdr + 0xe);
    int height = *(int*)(hdr + 8);
    int width = *(int*)(hdr + 4);
    void* src = hdr + 0x2c;
    if (bitcount == 8) {
        src = hdr + *(int*)hdr + 0x400;
    }
    return DecodeBlit(src, a2, width, height, bitcount, a3);
}

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
// CImage::DecodePcxData
// The .PCX decoder: parse the ZSoft header (width = Xmax-Xmin+1, height =
// Ymax-Ymin+1; bail unless BitsPerPixel==8), allocate the plane via
// DecodeBmpHeader (bitcount = NPlanes*8), then RLE-decode each scanline into a
// scratch buffer (filled back-to-front) and emit it into the plane row, either
// straight (1 plane) or interleaving 3 planes into RGB triples.
RVA(0x176000, 0x18f)
int CImage::DecodePcxData(void* buf, void* a2, void* a3) {
    unsigned char* hdr = (unsigned char*)buf;
    int width = *(short*)(hdr + 8) - *(short*)(hdr + 4) + 1;
    int height = *(short*)(hdr + 0xa) - *(short*)(hdr + 6) + 1;
    if (hdr[3] != 8) {
        return 0;
    }
    if (!DecodeBmpHeader(a2, width, height, (signed char)hdr[0x41] * 8, a3)) {
        return 0;
    }

    unsigned char* src = hdr + 0x80;
    int scanBytes = (width * (signed char)hdr[0x41] * (signed char)hdr[3] + 7) / 8;
    unsigned char* scan = (unsigned char*)operator new(scanBytes);

    for (int y = 0; y < height; y++) {
        unsigned char* dst = (unsigned char*)m_42c + m_430[y];
        int n = width * (signed char)hdr[0x41];
        while (n > 0) {
            unsigned char c = *src++;
            if ((c & 0xc0) == 0xc0) {
                int count = c & 0x3f;
                unsigned char v = *src++;
                if (count > 0) {
                    do {
                        --n;
                        --count;
                        scan[n] = v;
                    } while (count != 0);
                }
            } else {
                scan[--n] = c;
            }
        }

        if ((signed char)hdr[0x41] == 1) {
            for (int x = width; x != 0; x--) {
                *dst++ = scan[x - 1];
            }
        } else if ((signed char)hdr[0x41] == 3) {
            unsigned char* g = scan + width * 2;
            unsigned char* b = g + width;
            for (int x = width; x != 0; x--) {
                *dst++ = scan[x - 1];
                *dst++ = g[-1];
                *dst++ = b[-1];
                --g;
                --b;
            }
        }
    }

    operator delete(scan);
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
// CImage::DecodeRidData
// The .RID decoder: the header at buf+8 carries (width, height) and the raw
// 8bpp pixels begin at buf+0x20; hand them straight to the blitter. a3's low bit
// gates the transparency flag at this+0x450 (cleared when not set).
RVA(0x1762c0, 0x42)
int CImage::DecodeRidData(void* buf, void* a2, void* a3) {
    int* hdr = (int*)((char*)buf + 8);
    int width = hdr[0];
    int height = hdr[1];
    int ok = DecodeBlit((char*)buf + 0x20, a2, width, height, 8, a3);
    if (!((int)a3 & 1)) {
        m_450 = 0;
    }
    return ok;
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
// CImage::DecodePidData
// The .PID decoder. The header carries a flags word at buf+4 and geometry at
// buf+8/buf+0xc; raw run data starts at buf+0x20. After allocating the plane
// (DecodeBmpHeader, bitcount 8) two decode modes are selected by flags:
//   flags&0x20  -> a horizontal skip/fill RLE: each opcode either repeats a fill
//                  colour (high bit set, count = c-0x80) or copies `c` literal
//                  bytes, advancing x across rows by the source width (m_438).
//   else        -> a per-row PCX-style RLE ((c&0xc0)==0xc0 => run of `c&0x3f`).
// flags&0x100 masks the fill colour (buf+0x18) to a low word, else it is zeroed.
// a3's low bit gates the transparency flag at this+0x450.
RVA(0x176440, 0x25d)
int CImage::DecodePidData(void* buf, void* a2, void* a3) {
    unsigned char* src = (unsigned char*)buf + 0x20;
    int width = *(int*)((char*)buf + 8);
    int height = *(int*)((char*)buf + 0xc);
    int flags = *(int*)((char*)buf + 4);
    int fill = *(int*)((char*)buf + 0x18);

    if (!DecodeBmpHeader(a2, width, height, 8, a3)) {
        return 0;
    }
    if (!((int)a3 & 1)) {
        m_450 = 0;
    }

    if (flags & 0x100) {
        fill &= 0xffff;
    } else {
        fill = 0;
    }

    if (flags & 0x20) {
        m_450 = 1;
        unsigned char* dstRow = (unsigned char*)m_42c + m_430[0];
        int x = 0;
        int y = 0;
        int i = 0;
        while (y < m_43c) {
            unsigned char c = src[i];
            if (c & 0x80) {
                int count = (c & 0xff) - 0x80;
                memset(dstRow + x, (unsigned char)fill, count);
                x += (src[i] & 0xff) - 0x80;
                i++;
            } else {
                int count = c & 0xff;
                memcpy(dstRow + x, &src[i + 1], count);
                x += src[i];
                i += src[i] + 1;
            }
            if (x >= m_438) {
                y++;
                x = 0;
                if (y >= m_43c) {
                    break;
                }
                dstRow = (unsigned char*)m_42c + m_430[y];
            }
        }
    } else {
        for (int y = 0; (unsigned int)y < (unsigned int)height; y++) {
            unsigned char* dst = (unsigned char*)m_42c + m_430[y];
            int n = width;
            while (n > 0) {
                unsigned char c = *src++;
                if ((c & 0xc0) == 0xc0) {
                    int count = c & 0x3f;
                    unsigned char v = *src++;
                    if (count > 0) {
                        memset(dst, v, count);
                        dst += count;
                    }
                    n -= count;
                } else {
                    *dst++ = c;
                    n--;
                }
            }
        }
    }
    return 1;
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
