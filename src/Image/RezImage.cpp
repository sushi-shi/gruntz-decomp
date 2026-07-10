// RezImage.cpp - the engine's REZ -> image resolution path (split out of Image.cpp).
//
// The CRezImage format loaders + their per-format decoders, plus the file-loader
// dispatcher CImageExtLoader. Split from the former `image` god-TU (which also held
// the CDDSurface DIRSURF surface class, now the sole content of Image.cpp) so each
// class de-fragments into its own contiguous TU (retail RVA band 0x1757c0-0x1772e0).
//
//   CRezImage::LoadFromRez  - ext dispatcher (.BMP/.PCX/.RID/.PID -> sibling loader)
//   CRezImage::Load{Bmp,Pcx,Rid,Pid,Default} + the per-format decoders
//     (DecodeBmpHeader/DecodeResData/DecodePcxData/DecodeRidData/DecodePidData)
//   CImageExtLoader::LoadByExtension + LoadPalFile/LoadPcxFile (the file-loader path)
//
// DecodeBmpHeader is the shared plane allocator (fills the BITMAPINFOHEADER,
// CreateDIBSections the pixel plane + builds the bottom-up per-row offset table);
// DecodeResData/RidData hand pre-decoded pixels to the shared blitter (DecodeBlit
// @0x175930, external/no-body); DecodePcxData/PidData RLE-decode .PCX/.PID into the
// plane. The .PID format (flags@4, width@8, height@0xc, COMPRESSION=0x20) matches
// Monolith's WAP32 layout (libwap32 wap32/pid.h). The CFileIO stack objects in the
// loaders carry a dtor -> a C++ EH frame -> this TU builds with /GX.
#include <Image/Image.h>
#include <rva.h>
// <string.h>: strrchr (find the ext dot) / _stricmp (the case-insensitive ext
// compare) + memset/memcpy in the RLE decoders.
#include <string.h>

// The .PID/.PCX-via-RezMgr flags word (header+4). Monolith's WAP32 layout
// (libwap32 wap32/pid.h). Same immediates as
// the bare masks, so naming them is matching-neutral.
enum PidFlags {
    PID_TRANSPARENCY = 0x01,     // bit0  install the transparent colour key
    PID_VIDEO_MEMORY = 0x02,     // bit1  "VID"
    PID_SYSTEM_MEMORY = 0x04,    // bit2  "SYS"
    PID_COMPRESSION = 0x20,      // bit5  "RLE" - skip/fill RLE pixel stream
    PID_EMBEDDED_PALETTE = 0x80, // bit7  trailing 768-byte VGA palette at EOF
};

// The .PID on-disk image header (32 = 0x20 bytes; 8 dwords). DecodePidData reads
// this at the head of the RezMgr payload; the RLE/uncompressed 8bpp pixel stream
// begins right after it (buf + 0x20). Typing the buffer lets the decoder read the
// header by name instead of raw offsets (matching-neutral - same offsets/widths).
struct PidHeader {
    u32 fileDesc; // +0x00  id / file descriptor
    u32 flags;    // +0x04  PidFlags
    i32 width;    // +0x08
    i32 height;   // +0x0c
    i32 offsetX;  // +0x10  draw anchor X
    i32 offsetY;  // +0x14  draw anchor Y
    u32 fill;     // +0x18  fill colour (masked to low word when flags & 0x100)
    u32 unk1;     // +0x1c
    // +0x20: the RLE/uncompressed 8bpp pixel stream begins here.
};
SIZE_UNKNOWN(PidHeader);

// The four file-extension literals (reloc-masked .rdata globals). Declared at
// file scope so each `push OFFSET` matches the binary's direct-address push.
static const char s_extBmp[] = ".BMP";
static const char s_extPcx[] = ".PCX";
static const char s_extRid[] = ".RID";
static const char s_extPid[] = ".PID";
static const char s_extPal[] = ".PAL"; // the file-loader dispatcher (0x176f90) variant

// The resource module the .DEFAULT loader pulls RT_BITMAP resources from
// (reloc-masked .data global; 0 until the engine records the instance handle).
DATA(0x002bf6e0)
extern "C" HINSTANCE g_hResModule; // 0x6bf6e0


// ---------------------------------------------------------------------------
// CRezImage::DecodeBmpHeader
// The plane allocator/setup shared by every format. Records the image geometry
// (width/abs(height)/bitcount and the aligned destination stride) into the
// engine fields at this+0x434.., builds an in-place BITMAPINFOHEADER (this+0)
// with an identity DIB_PAL_COLORS table for 8bpp, CreateDIBSections the pixel
// plane (HBITMAP @+0x428, bits @+0x42c), and operator-new's the bottom-up
// per-row byte-offset table (this+0x430). Returns 0 if CreateDIBSection fails.
RVA(0x001757c0, 0x16f)
i32 CRezImage::DecodeBmpHeader(void* a2, i32 width, i32 height, i32 bitcount, void* a3) {
    m_434 = 0;
    m_width = width;
    m_height = (height < 0) ? -height : height;
    m_bitCount = bitcount;
    if (bitcount == 8) {
        m_stride = ((width + 3) / 4) * 4;
    } else {
        m_stride = width;
    }
    m_rowPad = m_stride - width;
    m_paletteScalar = 0;
    m_paletteNode = 0;
    m_transparent = 1;
    memset(&m_bih, 0, sizeof(BITMAPINFOHEADER));
    m_bih.biWidth = m_width;
    m_bih.biBitCount = (WORD)m_bitCount;
    m_bih.biSize = sizeof(BITMAPINFOHEADER);
    m_bih.biHeight = height;
    m_bih.biPlanes = 1;
    m_bih.biCompression = 0;
    m_bih.biSizeImage = 0;
    m_bih.biClrUsed = 0;
    m_bih.biClrImportant = 0;
    if (m_bitCount == 8) {
        for (i32 i = 0; i < 256; i++) {
            m_pal[i] = (u16)i;
        }
        m_dibSection =
            CreateDIBSection((HDC)a2, (BITMAPINFO*)&m_bih, DIB_PAL_COLORS, (void**)&m_pixels, 0, 0);
    } else {
        m_dibSection =
            CreateDIBSection((HDC)a2, (BITMAPINFO*)&m_bih, DIB_RGB_COLORS, (void**)&m_pixels, 0, 0);
    }
    if (!m_dibSection) {
        return 0;
    }
    m_rowOffsets = (i32*)operator new(m_height * 4);
    for (i32 i = 0; i < m_height; i++) {
        m_rowOffsets[i] = (m_height - i - 1) * (m_bitCount / 8) * m_stride;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CRezImage::DecodeBlit - the shared plane blitter the format decoders
// call. (Re)allocate/decode the plane via DecodeBmpHeader (fail -> return 0),
// then copy `src` into it: contiguous rep-movs of (m_stride*m_height*bitcount)/8
// bytes when m_rowPad==0, else row-by-row through the m_rowOffsets table (each
// row m_width bytes from the running source).
// @early-stop
// shrink-wrapped callee-save push wall (~83%): body byte-identical otherwise. Retail
// saves only ebx/esi at entry and defers `push edi`/`push ebp` past the DecodeBmpHeader
// early-out (which restores just esi/ebx); cl pushes all four upfront. Not source-
// steerable; docs/patterns/shrink-wrapped-callee-save-push.md. Final sweep.
RVA(0x00175930, 0xc6)
i32 CRezImage::DecodeBlit(void* src, void* a2, i32 width, i32 height, i32 bitcount, void* a3) {
    if (!DecodeBmpHeader(a2, width, height, bitcount, a3)) {
        return 0;
    }
    if (m_rowPad == 0) {
        memcpy(m_pixels, src, (u32)(m_stride * m_height * bitcount) >> 3);
        return 1;
    }
    char* s = (char*)src;
    for (i32 row = 0; row < m_height; row++) {
        memcpy(m_pixels + m_rowOffsets[row], s, m_width);
        s += m_width;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CRezImage::LoadFromRez
// ext = strrchr(name,'.'); dispatch on .BMP/.PCX/.RID/.PID, else default. Each
// branch re-tests `ext != 0` (the target's `test esi; je default` per case) and
// forwards (name,a2,a3); a matched ext returns its loader's result directly.
RVA(0x00175a90, 0xee)
i32 CRezImage::LoadFromRez(char* name, void* a2, void* a3) {
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

// ---------------------------------------------------------------------------
// CRezImage::DecodeResData
// The RT_BITMAP / .DEFAULT decoder: `buf` points at a packed DIB
// (BITMAPINFOHEADER + palette + pixels). Pull biWidth/biHeight/biBitCount, point
// `src` at the pixel bytes (for 8bpp the 256-entry RGBQUAD palette pushes them to
// buf+biSize+0x400, else right after the 0x28 header + the 4 RGBQUAD masks at
// buf+0x2c) and hand it to the shared blitter.
RVA(0x00175e00, 0x3d)
i32 CRezImage::DecodeResData(void* buf, void* a2, void* a3) {
    BITMAPINFOHEADER* ih = (BITMAPINFOHEADER*)buf;
    i32 bitcount = ih->biBitCount;
    i32 height = ih->biHeight;
    i32 width = ih->biWidth;
    void* src = (u8*)buf + 0x2c;
    if (bitcount == 8) {
        src = (u8*)buf + ih->biSize + 0x400;
    }
    return DecodeBlit(src, a2, width, height, bitcount, a3);
}

// ---------------------------------------------------------------------------
// CRezImage::LoadBmp
// The .BMP loader: open the file, read the 14-byte BITMAPFILEHEADER and the
// 40-byte BITMAPINFOHEADER, hand the parsed (width, height, bitcount, a2, a3)
// to the decode helper that allocates the CRezImage's pixel plane, then Seek to
// bfOffBits and Read exactly (bitcount/8)*stride*height pixel bytes into the
// plane. Returns 1 on a full read, 0 on any I/O / decode failure. The CFileIO
// stack object's dtor runs on every exit -> the C++ EH frame.
RVA(0x00175e40, 0x1b3)
i32 CRezImage::LoadBmp(char* name, void* a2, void* a3) {
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

    i32 height = ih.biHeight;
    i32 width = ih.biWidth;
    i32 bitcount = ih.biBitCount & 0xffff;
    if (!DecodeBmpHeader(a2, width, height, bitcount, a3)) {
        return 0;
    }

    file.Seek(fh.bfOffBits, 0);
    u32 size = (bitcount / 8) * m_stride * height;
    if (file.Read(m_pixels, size) != size) {
        return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CRezImage::DecodePcxData
// The .PCX decoder: parse the ZSoft header (width = Xmax-Xmin+1, height =
// Ymax-Ymin+1; bail unless BitsPerPixel==8), allocate the plane via
// DecodeBmpHeader (bitcount = NPlanes*8), then RLE-decode each scanline into a
// scratch buffer (filled back-to-front) and emit it into the plane row, either
// straight (1 plane) or interleaving 3 planes into RGB triples.
RVA(0x00176000, 0x18f)
i32 CRezImage::DecodePcxData(void* buf, void* a2, void* a3) {
    u8* hdr = (u8*)buf;
    i32 width = *(i16*)(hdr + 8) - *(i16*)(hdr + 4) + 1;
    i32 height = *(i16*)(hdr + 0xa) - *(i16*)(hdr + 6) + 1;
    if (hdr[3] != 8) {
        return 0;
    }
    if (!DecodeBmpHeader(a2, width, height, (i8)hdr[0x41] * 8, a3)) {
        return 0;
    }

    u8* src = hdr + 0x80;
    i32 scanBytes = (width * (i8)hdr[0x41] * (i8)hdr[3] + 7) / 8;
    u8* scan = (u8*)operator new(scanBytes);

    for (i32 y = 0; y < height; y++) {
        u8* dst = m_pixels + m_rowOffsets[y];
        i32 n = width * (i8)hdr[0x41];
        while (n > 0) {
            u8 c = *src++;
            if ((c & 0xc0) == 0xc0) {
                i32 count = c & 0x3f;
                u8 v = *src++;
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

        if ((i8)hdr[0x41] == 1) {
            for (i32 x = width; x != 0; x--) {
                *dst++ = scan[x - 1];
            }
        } else if ((i8)hdr[0x41] == 3) {
            u8* g = scan + width * 2;
            u8* b = g + width;
            for (i32 x = width; x != 0; x--) {
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
// CRezImage::LoadPcx
// Open the file, GetLength(); if zero return 0. `operator new` a buffer of that
// size; if it fails return 0. Read the whole file, hand the buffer (+a2,a3) to
// the PCX decode helper, free the buffer and return the decoder's result.
RVA(0x00176190, 0x126)
i32 CRezImage::LoadPcx(char* name, void* a2, void* a3) {
    CFileIO file;

    if (!file.Open(name, 0, 0)) {
        return 0;
    }
    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    i32 result = DecodePcxData(buf, a2, a3);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CRezImage::DecodeRidData
// The .RID decoder: the header at buf+8 carries (width, height) and the raw
// 8bpp pixels begin at buf+0x20; hand them straight to the blitter. a3's low bit
// gates the transparency flag at this+0x450 (cleared when not set).
RVA(0x001762c0, 0x42)
i32 CRezImage::DecodeRidData(void* buf, void* a2, void* a3) {
    i32* hdr = (i32*)((char*)buf + 8);
    i32 width = hdr[0];
    i32 height = hdr[1];
    i32 ok = DecodeBlit((char*)buf + 0x20, a2, width, height, 8, a3);
    if (!((i32)a3 & 1)) {
        m_transparent = 0;
    }
    return ok;
}

// ---------------------------------------------------------------------------
// CRezImage::LoadRid
// Byte-identical to LoadPcx except for the per-format decode helper (the .RID
// reader DecodeRidData).
RVA(0x00176310, 0x126)
i32 CRezImage::LoadRid(char* name, void* a2, void* a3) {
    CFileIO file;

    if (!file.Open(name, 0, 0)) {
        return 0;
    }
    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    i32 result = DecodeRidData(buf, a2, a3);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CRezImage::DecodePidData
// The .PID decoder. The header carries a flags word at buf+4 and geometry at
// buf+8/buf+0xc; raw run data starts at buf+0x20. After allocating the plane
// (DecodeBmpHeader, bitcount 8) two decode modes are selected by flags:
//   flags&0x20  -> a horizontal skip/fill RLE: each opcode either repeats a fill
//                  colour (high bit set, count = c-0x80) or copies `c` literal
//                  bytes, advancing x across rows by the source width (m_width).
//   else        -> a per-row PCX-style RLE ((c&0xc0)==0xc0 => run of `c&0x3f`).
// flags&0x100 masks the fill colour (buf+0x18) to a low word, else it is zeroed.
// a3's low bit gates the transparency flag at this+0x450.
RVA(0x00176440, 0x25d)
i32 CRezImage::DecodePidData(void* buf, void* a2, void* a3) {
    PidHeader* hdr = (PidHeader*)buf;
    u8* src = (u8*)(hdr + 1); // pixel stream at buf + 0x20
    i32 width = hdr->width;
    i32 height = hdr->height;
    i32 flags = hdr->flags;
    i32 fill = hdr->fill;

    if (!DecodeBmpHeader(a2, width, height, 8, a3)) {
        return 0;
    }
    if (!((i32)a3 & 1)) {
        m_transparent = 0;
    }

    if (flags & 0x100) {
        fill &= 0xffff;
    } else {
        fill = 0;
    }

    if (flags & PID_COMPRESSION) {
        m_transparent = 1;
        u8* dstRow = m_pixels + m_rowOffsets[0];
        i32 x = 0;
        i32 y = 0;
        i32 i = 0;
        while (y < m_height) {
            u8 c = src[i];
            if (c & 0x80) {
                i32 count = (c & 0xff) - 0x80;
                memset(dstRow + x, (u8)fill, count);
                x += (src[i] & 0xff) - 0x80;
                i++;
            } else {
                i32 count = c & 0xff;
                memcpy(dstRow + x, &src[i + 1], count);
                x += src[i];
                i += src[i] + 1;
            }
            if (x >= m_width) {
                y++;
                x = 0;
                if (y >= m_height) {
                    break;
                }
                dstRow = m_pixels + m_rowOffsets[y];
            }
        }
    } else {
        for (i32 y = 0; (u32)y < (u32)height; y++) {
            u8* dst = m_pixels + m_rowOffsets[y];
            i32 n = width;
            while (n > 0) {
                u8 c = *src++;
                if ((c & 0xc0) == 0xc0) {
                    i32 count = c & 0x3f;
                    u8 v = *src++;
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
// CRezImage::LoadPid
// Byte-identical to LoadPcx/LoadRid except for the .PID decode helper.
RVA(0x001766a0, 0x126)
i32 CRezImage::LoadPid(char* name, void* a2, void* a3) {
    CFileIO file;

    if (!file.Open(name, 0, 0)) {
        return 0;
    }
    u32 len = file.GetLength();
    if (len == 0) {
        return 0;
    }
    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }
    file.Read(buf, len);
    i32 result = DecodePidData(buf, a2, a3);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CRezImage::LoadDefault
// The fallback (no/unknown extension): pull the named RT_BITMAP resource from
// the engine's resource module and decode it in place. Returns 0 unless the
// module handle is set and FindResource/LoadResource/LockResource all succeed.
RVA(0x001767d0, 0x64)
i32 CRezImage::LoadDefault(char* name, void* a2, void* a3) {
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
// CImageExtLoader::LoadByExtension  @0x176f90
// ---------------------------------------------------------------------------
// The FILE-loader counterpart of LoadFromRez (sits in the 0x176df0-0x177xxx file
// cluster just past the CRezImage rez loaders): strrchr the dot, then a stricmp
// ladder on .BMP/.PCX/.PAL forwarding (path, arg) verbatim to the matching file
// loader; an absent/unknown extension falls through to Apply. Each case re-tests
// `ext != 0` (the target's per-case `test esi; je default`). The four loaders are
// reloc-masked external siblings (0x177480/0x1772e0/0x1771f0/0x1775f0); `this` is
// pure-forwarded so its layout is immaterial. __thiscall, ret 8.
class CImageExtLoader {
public:
    i32 LoadByExtension(char* path, i32 arg);
    i32 LoadBmpFile(char* path, i32 arg);  // 0x177480
    i32 LoadPcxFile(char* path, i32 arg);  // 0x1772e0  (.PCX palette tail)
    i32 LoadPalFile(char* path, i32 arg);  // 0x1771f0  (768-byte .PAL)
    i32 Apply(char* path, i32 arg);        // 0x1775f0  (default)
    i32 ProcessPal(void* rgb768, i32 arg); // 0x176e70 (external)
    i32 BuildPalette(void* rgbq, i32 arg); // 0x176df0 (external)
};

RVA(0x00176f90, 0xa4)
i32 CImageExtLoader::LoadByExtension(char* path, i32 arg) {
    char* ext = strrchr(path, '.');

    if (ext && _stricmp(ext, s_extBmp) == 0) {
        return LoadBmpFile(path, arg);
    } else if (ext && _stricmp(ext, s_extPcx) == 0) {
        return LoadPcxFile(path, arg);
    } else if (ext && _stricmp(ext, s_extPal) == 0) {
        return LoadPalFile(path, arg);
    }

    return Apply(path, arg);
}

// ---------------------------------------------------------------------------
// CImageExtLoader::LoadPalFile
// ---------------------------------------------------------------------------
// Load a raw 768-byte (.PAL) palette: open the file, require length == 0x300,
// Read the 256*3 RGB bytes into a stack buffer, hand it to ProcessPal(buf, arg).
// Any I/O failure returns 0. The CFileIO stack object forces the /GX EH frame.
// __thiscall, ret 8.
RVA(0x001771f0, 0xe2)
i32 CImageExtLoader::LoadPalFile(char* path, i32 arg) {
    CFileIO file;
    char rgb[0x300];

    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    if (file.GetLength() != 0x300) {
        return 0;
    }
    file.Read(rgb, 0x300);
    return ProcessPal(rgb, arg);
}

// ---------------------------------------------------------------------------
// CImageExtLoader::LoadPcxFile
// ---------------------------------------------------------------------------
// Load the trailing palette of a .PCX: seek 0x300 bytes back from EOF, Read the
// 256*3 RGB triples; on a short read return 0. Expand the triples in place into a
// 256-entry RGBQUAD table (R,G,B,0) and hand it to BuildPalette(table, arg). The
// CFileIO stack object forces the /GX EH frame. __thiscall, ret 8.
// @early-stop
// 93.9% de-interleave-loop induction-phase wall: the EH frame + open/seek/read +
// BuildPalette call are byte-exact, but retail phases the dst induction variable at
// base+1 (`add ecx,4` after the FIRST byte store, the four writes at [iv-1]/[iv-4]/
// [iv-3]/[iv-2], the zero-store LAST) while clean C reorders the +4 and the zero
// store; not source-steerable. Logic 100% correct (256 RGB triples -> RGBQUAD).
RVA(0x001772e0, 0x117)
i32 CImageExtLoader::LoadPcxFile(char* path, i32 arg) {
    CFileIO file;
    u8 rgb[0x300];
    u8 rgbq[0x400];

    if (!file.Open(path, 0, 0)) {
        return 0;
    }
    file.Seek(-0x300, 2);
    if (file.Read(rgb, 0x300) == 0) {
        return 0;
    }

    u8* src = rgb;
    u8* dst = rgbq;
    for (i32 i = 0x100; i != 0; i--) {
        dst[0] = *src++;
        dst[1] = *src++;
        dst[2] = *src++;
        dst[3] = 0;
        dst += 4;
    }
    return BuildPalette(rgbq, arg);
}

SIZE_UNKNOWN(CImageExtLoader);
