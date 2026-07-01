// Image.cpp - the engine's REZ -> image resolution path.
//
// Functions matched in this TU:
//   CRezImage::LoadFromRez  - ext dispatcher
//   CRezImage::Load{Bmp,Pcx,Rid,Pid,Default} + the per-format decoders
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
#include <Rez/RezMgr.h> // RezAlloc/RezFree - the engine allocator the decoders use
#include <rva.h>
// <string.h>: strrchr (find the ext dot) / _stricmp (the case-insensitive ext compare).
#include <string.h>
// CDDSurface (DIRSURF.CPP) is the SAME object as CFileImage here - the file-image
// surface and the DirectDraw surface wrapper are one class viewed two ways. The
// blitters/run-decoders are its leaf methods (named CFileImage::* to match the
// already-matched decoder callers), but their bodies touch the rich surface
// layout + call the CDDSurface COM thunks (Lock/SetColorKey, reloc-masked).
#include <Gruntz/CDirectDrawMgr.h>

// The .PID/.PCX-via-RezMgr flags word (header+4). Monolith's WAP32 layout
// (libwap32 wap32/pid.h, mirrored in src/Stub/types/wwd.h). Same immediates as
// the bare masks, so naming them is matching-neutral.
enum PidFlags {
    PID_TRANSPARENCY = 0x01,     // bit0  install the transparent colour key
    PID_VIDEO_MEMORY = 0x02,     // bit1  "VID"
    PID_SYSTEM_MEMORY = 0x04,    // bit2  "SYS"
    PID_COMPRESSION = 0x20,      // bit5  "RLE" - skip/fill RLE pixel stream
    PID_EMBEDDED_PALETTE = 0x80, // bit7  trailing 768-byte VGA palette at EOF
};

// The CFileImage::Resolve / ResolveEx `type` selector. Same case immediates as the
// bare 1/2/4 labels (the running-subtract switch chain is value-driven), so naming
// them is matching-neutral; bodies stay in retail .text order (4, 2, 1).
enum FileImageFormat {
    FMT_BMP = 1,
    FMT_PCX = 2,
    FMT_PID = 4,
};

// Per-decoder 256-entry RGBQUAD palette buffers (file-scope BSS, reloc-masked).
// Each decoder builds its own when the source carries an inline/trailing palette;
// the buffer addresses are differently-named symbols in the base obj (the absolute
// pushes/compares reloc-mask against the retail DAT_* names). Declared in their
// retail BSS order so the layout follows the binary.
static u8 s_palBmp[0x400];     // 0x6842f0
static u8 s_palPcx[0x400];     // 0x6846f0
static u8 s_palPidData[0x400]; // 0x684ef0 (CFileImage::DecodePid)
static u8 s_palPcxData[0x400]; // 0x6852f0 (CFileImage::DecodePcxData)

// The four file-extension literals (reloc-masked .rdata globals). Declared at
// file scope so each `push OFFSET` matches the binary's direct-address push.
static const char s_extBmp[] = ".BMP";
static const char s_extPcx[] = ".PCX";
static const char s_extRid[] = ".RID";
static const char s_extPid[] = ".PID";
static const char s_extPal[] = ".PAL"; // the file-loader dispatcher (0x176f90) variant

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
    m_454 = 0;
    m_458 = 0;
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
            CreateDIBSection((HDC)a2, (BITMAPINFO*)this, DIB_PAL_COLORS, &m_pixels, 0, 0);
    } else {
        m_dibSection =
            CreateDIBSection((HDC)a2, (BITMAPINFO*)this, DIB_RGB_COLORS, &m_pixels, 0, 0);
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
        memcpy((char*)m_pixels + m_rowOffsets[row], s, m_width);
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

// The resource module the .DEFAULT loader pulls RT_BITMAP resources from
// (reloc-masked .data global; 0 until the engine records the instance handle).
DATA(0x002bf6e0)
extern "C" HINSTANCE g_hResModule; // 0x6bf6e0

// ---------------------------------------------------------------------------
// CRezImage::DecodeResData
// The RT_BITMAP / .DEFAULT decoder: `buf` points at a packed DIB
// (BITMAPINFOHEADER + palette + pixels). Pull biWidth/biHeight/biBitCount, point
// `src` at the pixel bytes (for 8bpp the 256-entry RGBQUAD palette pushes them to
// buf+biSize+0x400, else right after the 0x28 header + the 4 RGBQUAD masks at
// buf+0x2c) and hand it to the shared blitter.
RVA(0x00175e00, 0x3d)
i32 CRezImage::DecodeResData(void* buf, void* a2, void* a3) {
    u8* hdr = (u8*)buf;
    i32 bitcount = *(u16*)(hdr + 0xe);
    i32 height = *(i32*)(hdr + 8);
    i32 width = *(i32*)(hdr + 4);
    void* src = hdr + 0x2c;
    if (bitcount == 8) {
        src = hdr + *(i32*)hdr + 0x400;
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
        u8* dst = (u8*)m_pixels + m_rowOffsets[y];
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
    u8* src = (u8*)buf + 0x20;
    i32 width = *(i32*)((char*)buf + 8);
    i32 height = *(i32*)((char*)buf + 0xc);
    i32 flags = *(i32*)((char*)buf + 4);
    i32 fill = *(i32*)((char*)buf + 0x18);

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
        u8* dstRow = (u8*)m_pixels + m_rowOffsets[0];
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
                dstRow = (u8*)m_pixels + m_rowOffsets[y];
            }
        }
    } else {
        for (i32 y = 0; (u32)y < (u32)height; y++) {
            u8* dst = (u8*)m_pixels + m_rowOffsets[y];
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

// ===========================================================================
// CFileImage surface helpers (DIRSURF.CPP leaf methods). `this` is the same
// object the CDDSurface wrapper holds, so the bodies view it as a CDDSurface to
// reach the rich surface layout + the COM thunks. Named CFileImage::* to pair
// with the matched decoder callers; placed in retail-RVA order (all below the
// CFileImage decoders' RVAs, so they lead the CFileImage section).
// ===========================================================================

// ---------------------------------------------------------------------------
// CFileImage::BlitSurf
// The DecodePcxData destination setup: zero the surface's DDSURFACEDESC, stash
// the colour-key arg (m_78), record width/height into the desc, set dwSize/
// dwFlags, and - when a4 names a non-zero source bpp that differs from the
// palette context's bpp (surf->m_palBitCount) - flag a colour-key blit (dwFlags|0x1000,
// ddckCKSrcBlt dwFlags 0x20, the key colour at m_64). Then dispatch the surface's
// own slot-8 virtual with `surf`.
RVA(0x0013e0d0, 0x66)
i32 CFileImage::BlitSurf(void* surf, i32 width, i32 height, i32 a4, i32 a5) {
    CDDSurface* s = (CDDSurface*)this;
    i32* desc = (i32*)s->m_desc;
    for (i32 i = 0x1b; i != 0; i--) {
        *desc++ = 0;
    }
    *(i32*)(s->m_desc + 0x68) = a5; // m_78
    *(i32*)(s->m_desc + 0xc) = width;
    *(i32*)(s->m_desc + 8) = height;
    *(i32*)s->m_desc = 0x6c;    // dwSize
    *(i32*)(s->m_desc + 4) = 7; // dwFlags
    if (a4 != 0 && a4 != ((CFileImage*)surf)->m_palBitCount) {
        *(i32*)(s->m_desc + 4) = 0x1007;
        *(i32*)(s->m_desc + 0x48) = 0x20; // m_58
        s->m_64 = a4;
    }
    return s->v20(surf);
}

// ---------------------------------------------------------------------------
// CFileImage::Resolve
// The file-format dispatcher (the .REZ payload path). `type` (1=BMP, 2=PCX,
// 4=PID) selects the matching decoder; the destination buffer arg (`size`) must
// be non-zero. Each decoder is handed (surf, buf, size); PID additionally takes
// the transparency-colour pass-through (surf2). Returns 1 on a successful decode,
// else 0. The near-consecutive case labels lower to MSVC's running-subtract chain
// (dec/dec/sub 2), so this is spelled as a switch (see
// docs/patterns/switch-subtract-chain-vs-ifelse.md), case bodies in retail .text
// order (4, 2, 1).
RVA(0x0013e550, 0x71)
i32 CFileImage::Resolve(void* surf, void* buf, i32 type, u32 size, void* surf2) {
    if (size == 0) {
        return 0;
    }
    switch (type) {
        case FMT_PID:
            if (!DecodePid((char*)surf, buf, size, surf2)) {
                return 0;
            }
            break;
        case FMT_PCX:
            if (!DecodePcx((char*)surf, buf, size)) {
                return 0;
            }
            break;
        case FMT_BMP:
            if (!DecodeBmp((char*)surf, buf, size)) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::Fill
// Colour-fill blt: build a zeroed DDBLTFX (dwSize 0x64, dwFillColor = `color` at
// +0x50) and Blt(NULL, NULL, NULL, DDBLT_COLORFILL|DDBLT_WAIT (0x1000400), &fx)
// through the surface's BltEx thunk. A bad HRESULT routes through
// CDirectDrawMgr::GetErrorString (DIRSURF.CPP, line 0x22c). Returns hr == DD_OK.
RVA(0x0013e760, 0x63)
i32 CFileImage::Fill(u32 color) {
    CDDSurface* s = (CDDSurface*)this;
    i32 fx[0x19]; // DDBLTFX (0x64 bytes)
    i32* p = fx;
    for (i32 i = 0x19; i != 0; i--) {
        *p++ = 0;
    }
    fx[0] = 0x64;          // dwSize
    fx[0x14] = (i32)color; // dwFillColor @ +0x50
    i32 hr = s->BltEx(0, 0, 0, 0x1000400, fx);
    if (hr != 0) {
        CDirectDrawMgr::GetErrorString((char*)"C:\\Proj\\DDrawMgr\\DIRSURF.CPP", 0x22c, hr);
    }
    return hr == 0;
}

// ---------------------------------------------------------------------------
// CFileImage::FillPalette
// Installs the transparency colour. arg == -1 means "no colour key": clear the
// have-key flag (m_bc) and pass {-1,-1}; otherwise set m_bc and set the surface
// source colour key to {arg, arg} (DDCKEY_SRCBLT = 8).
RVA(0x0013eb40, 0x3c)
void CFileImage::FillPalette(void* arg) {
    CDDSurface* s = (CDDSurface*)this;
    u32 ck[2];
    ck[0] = (u32)arg;
    ck[1] = (u32)arg;
    if ((i32)arg != -1) {
        s->m_bc = 1;
    } else {
        s->m_bc = 0;
    }
    s->SetColorKey(8, ck);
}

// ---------------------------------------------------------------------------
// CFileImage::BlitDirect
// Straight copy of `src` into the locked surface. Lock() returns the locked bits
// pointer (m_34); on failure return 0. Each row of m_ac bytes is copied into the
// row at locked + row*lPitch; mode 2 walks rows bottom-up (flipped), else top-
// down. Unlock and return 1.
RVA(0x0013ece0, 0xc7)
i32 CFileImage::BlitDirect(void* src, i32 mode) {
    CDDSurface* s = (CDDSurface*)this;
    i32 locked = s->Lock(0);
    if (locked == 0) {
        return 0;
    }
    u8* p = (u8*)src;
    if (mode == 2) {
        for (i32 row = *(i32*)(s->m_desc + 8) - 1; row >= 0; row--) {
            u8* dst = (u8*)locked + row * *(i32*)(s->m_desc + 0x10);
            u8* sp = p;
            i32 n = s->m_ac;
            for (i32 i = n; i > 0; i--) {
                *dst++ = *sp++;
            }
            p += n;
        }
    } else {
        for (i32 row = 0; row < *(i32*)(s->m_desc + 8); row++) {
            u8* dst = (u8*)locked + row * *(i32*)(s->m_desc + 0x10);
            u8* sp = p;
            i32 n = s->m_ac;
            for (i32 i = n; i > 0; i--) {
                *dst++ = *sp++;
            }
            p += n;
        }
    }
    s->m_8->vtbl->Unlock(s->m_8, 0);
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::Blit
// Palette-remap copy dispatcher. Selects a specialization by (dest bpp = m_a8,
// src bpp = bitcount). When m_a8==0 / bitcount agree on the "no remap" fast path
// it delegates to BlitDirect; otherwise a nested switch on dest bpp (8/16/24)
// then src bpp picks the matching Blit<dest><src> specialization. Unhandled
// combinations return 0.
RVA(0x0013faa0, 0x108)
i32 CFileImage::Blit(void* src, i32 bitcount, void* palette, i32 mode) {
    CDDSurface* s = (CDDSurface*)this;
    i32 dest = s->m_a8;
    if ((dest == 0) == bitcount) {
        return BlitDirect(src, mode);
    }
    switch (dest) {
        case 8:
            switch (bitcount) {
                case 0x10:
                    return Blit816(src, palette, mode);
                case 0x18:
                    return Blit824(src, palette, mode);
            }
            return 0;
        case 0x10:
            switch (bitcount) {
                case 8:
                    return Blit168(src, palette, mode);
                case 0x18:
                    return Blit1624(src, mode);
            }
            return 0;
        case 0x18:
            switch (bitcount) {
                case 8:
                    return Blit248(src, palette, mode);
                case 0x10:
                    return Blit2416(src, mode);
            }
            return 0;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// CFileImage::Blit248  (8bpp src -> 24bpp dest, palette remap)
// Lock the surface, walk it row-by-row (mode 2 = bottom-up flipped, else top-
// down) writing each source palette index's RGBQUAD bytes (2,1,0) as 3 dest
// bytes, then Unlock. Returns 0 if the palette is null or the lock fails.
// @early-stop
// 94.3% - both inner conversion loops byte-exact; residual is an edi<->ebp
// induction-variable allocation swap (src pinned in edi vs retail's ebp, which
// propagates a different ModRM byte through every src reference in both loops)
// + retail's `cmp $2,[esp+mode]` memory compare vs our reg-loaded `mov ecx,
// [mode];cmp ecx,2`. Both stem from `src` being a single live variable across
// the two branches (loaded before the mode test); a per-branch `src` flips the
// load late but un-spills `locked` and breaks the `push ecx` frame (drops to
// 91%). Regalloc-ordering wall (docs/patterns/zero-register-pinning.md).
RVA(0x0013fe60, 0x11e)
i32 CFileImage::Blit248(void* srcv, void* palv, i32 mode) {
    CDDSurface* s = (CDDSurface*)this;
    u8* pal = (u8*)palv;
    if (pal == 0) {
        return 0;
    }
    i32 locked = s->Lock(0);
    if (locked == 0) {
        return 0;
    }
    u8* src = (u8*)srcv;
    if (mode == 2) {
        for (i32 row = *(i32*)(s->m_desc + 8) - 1; row >= 0; row--) {
            u8* dst = (u8*)locked + row * *(i32*)(s->m_desc + 0x10);
            for (i32 col = 0; col < *(i32*)(s->m_desc + 0xc); col++) {
                u8 idx = *src++;
                *dst++ = pal[idx * 4 + 2];
                *dst++ = pal[idx * 4 + 1];
                *dst++ = pal[idx * 4];
            }
        }
    } else {
        for (i32 row = 0; row < *(i32*)(s->m_desc + 8); row++) {
            u8* dst = (u8*)locked + row * *(i32*)(s->m_desc + 0x10);
            for (i32 col = 0; col < *(i32*)(s->m_desc + 0xc); col++) {
                u8 idx = *src++;
                *dst++ = pal[idx * 4 + 2];
                *dst++ = pal[idx * 4 + 1];
                *dst++ = pal[idx * 4];
            }
        }
    }
    s->m_8->vtbl->Unlock(s->m_8, 0);
    return 1;
}

// The CFileImage vtable (reloc-masked .rdata global). The destructor restores the
// vptr to it before tearing the object down; the class's virtuals live in other
// (unmatched) TUs, so the vtable is modeled as a DATA extern + a manual stamp
// rather than letting the compiler emit a divergent one.
DATA(0x001ef7f0)
extern void* g_fileImageVtbl; // 0x5ef7f0

// ---------------------------------------------------------------------------
// CFileImage::~CFileImage
// The virtual destructor: MSVC stamps the vptr (compiler-implicit, stamp-first),
// runs the shared surface teardown (FreeSurfaces: release the held DirectDraw
// surfaces + walk the +0x98 object array), then destroys the owned CPtrArray at
// +0x94. The CPtrArray member-dtor is guarded -> the /GX EH frame. The implicit
// stamp reloc-masks against the shared 0x5ef7f0 surface vtable.
RVA(0x00141350, 0x53)
CFileImage::~CFileImage() {
    FreeSurfaces();
}

// ---------------------------------------------------------------------------
// CFileImage::FreeSurfaces (vtable slot 4, @+0x10) - the shared surface teardown.
// Walk the +0x94 CPtrArray (m_pData@0x98, count@0x9c, unsigned) running each
// element's slot-0 scalar-deleting destructor, RemoveAll the array (SetSize(0,-1)),
// then - unless the "don't-own" flag (m_7c & 1) is set - Release the two held
// IDirectDrawSurfaces (m_8/m_c) and null them, and clear m_b8.
RVA(0x0013e4d0, 0x7e)
void CFileImage::FreeSurfaces() {
    CDDSurface* s = (CDDSurface*)this;
    for (u32 i = 0; i < (u32)m_elements.GetSize(); i++) {
        CFileImageElement* e = (CFileImageElement*)m_elements[i];
        if (e != 0) {
            e->ScalarDtor(1);
        }
    }
    m_elements.SetSize(0, -1);
    if (s->m_8 != 0) {
        if ((s->m_7c & 1) == 0) {
            s->m_8->vtbl->Release(s->m_8);
        }
        s->m_8 = 0;
    }
    if (s->m_c != 0) {
        if ((s->m_7c & 1) == 0) {
            s->m_c->vtbl->Release(s->m_c);
        }
        s->m_c = 0;
    }
    s->m_b8 = 0;
}

// ---------------------------------------------------------------------------
// CFileImageSurface::ScalarDelete - the derived surface wrapper's `??_G`
// scalar-deleting destructor (vtable slot 0 @0x5efa58). Run the teardown copy, then -
// when the low bit of the hidden flags arg is set - RezFree the object; return this.
RVA(0x00142340, 0x1e)
void* CFileImageSurface::ScalarDelete(u32 flags) {
    // Qualified call -> direct (non-virtual) dispatch to the 0x142360 teardown copy,
    // matching retail's ??_G which calls the non-deleting dtor directly.
    this->CFileImageSurface::~CFileImageSurface();
    if (flags & 1) {
        RezFree(this);
    }
    return this;
}

// ---------------------------------------------------------------------------
// CFileImageSurface::~CFileImageSurface - the second compiled teardown
// copy, byte-identical to ~CFileImage (0x141350): the virtual destructor's implicit
// vptr stamp lands stamp-first, then the shared FreeSurfaces teardown runs and the
// owned CPtrArray member at +0x94 is destroyed (guarded -> the /GX EH frame). The
// implicit stamp reloc-masks against the shared 0x5ef7f0 surface vtable.
RVA(0x00142360, 0x53)
CFileImageSurface::~CFileImageSurface() {
    FreeSurfaces();
}

// ---------------------------------------------------------------------------
// The factory at 0x13e9a0 builds a CFileImageSurface from a source resolver.
// Modeling pieces (all reloc-masked):
//   - the source's slot-0 probe(magic, &out) - declared on a tiny polymorphic view;
//   - the global CObArray registry @0x653c88 + its grow index @0x653c90;
//   - the 0xc0 surface item (vtbl g_fileImageVtbl, CByteArray @+0x94), the same
//     shape as CDDrawPtrCollections::Create7f0_1.
// ---------------------------------------------------------------------------
inline void* operator new(u32, void* p) {
    return p;
} // placement new (construct in place)

class CRezImageSource {
public:
    virtual i32 Probe(void* magic, void** out); // slot 0 (@0x00)
};

// The data tag passed to the source probe (reloc-masked .rdata datum).
DATA(0x001ef888)
extern void* g_imageProbeTag; // 0x5ef888

// The created 0xc0 surface item: vptr @0, the slot-1 Load, a CByteArray @+0x94.
class CByteArrayMember {
public:
    CByteArrayMember(); // 0x1b4f0b (reloc-masked rel32)
};
class CRezSurfaceItem {
public:
    virtual void* Delete(u32 flags); // slot 0 (@0x00) scalar-deleting dtor
    virtual i32 Load(void* src);     // slot 1 (@0x04)

    char m_pad04[0x94 - 0x04]; // +0x04 (m_04/m_08/m_0c/m_7c zeroed)
    CByteArrayMember m_94;     // +0x94
    char m_pada8[0xc0 - 0x98]; // +0xa8/+0xb8 zeroed
};
class CImageSurfaceItemInit {
public:
    inline CImageSurfaceItemInit() {
        *(void**)this = &g_fileImageVtbl;
        *(i32*)((char*)this + 0x08) = 0;
        *(i32*)((char*)this + 0x0c) = 0;
        *(i32*)((char*)this + 0x04) = 0;
        *(i32*)((char*)this + 0x7c) = 0;
        *(i32*)((char*)this + 0xa8) = 0;
        *(i32*)((char*)this + 0xb8) = 0;
    }

    char m_pad00[0x94];
    CByteArrayMember m_94;
    char m_pada8[0xc0 - 0x95];
};

// The global image cache the new item is filed into.
class CImageCache {
public:
    void SetAtGrow(i32 index, CRezSurfaceItem* item); // 0x1b5144
};
DATA(0x00253c88)
extern CImageCache g_imageCache; // 0x653c88
extern i32 g_imageCacheIndex;    // 0x653c90

// The owner of the factory (this) is not touched by the body; modeled as an
// opaque shell so the call lowers to the retail __thiscall frame.
class CImageFactory {
public:
    i32 Build_13e9a0(CRezImageSource* src, i32 a2);
};

// ---------------------------------------------------------------------------
// Probe `src` (slot 0); if it yields a payload, allocate a 0xc0 surface
// item, construct it (CByteArray @+0x94, stamp g_fileImageVtbl, zero the scalar
// fields), Load the payload through slot 1, and on success file it into the global
// image cache - else virtual-delete it. /GX. ret 0xc.
// @early-stop
// rezalloc-placement-new-no-eh-frame wall (docs/patterns/rezalloc-placement-new-no-eh-
// frame.md), the same wall as the sibling Create7f0_1/CreateA factories: retail wraps
// `new`+throwing-member-ctor in a /GX frame; MSVC5 placement-new emits no
// ctor-in-flight EH state, so the body is byte-exact but the frame differs. Deferred
// to the final sweep.
RVA(0x0013e9a0, 0xcc)
i32 CImageFactory::Build_13e9a0(CRezImageSource* src, i32 a2) {
    void* payload = 0;
    if (src->Probe(&g_imageProbeTag, &payload) != 0) {
        CRezSurfaceItem* item = (CRezSurfaceItem*)new CImageSurfaceItemInit;
        if (item->Load(payload)) {
            g_imageCache.SetAtGrow(g_imageCacheIndex, item);
        } else if (item) {
            item->Delete(1);
        }
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::Clear (ret 4) - blank the surface. Build a zeroed 0x64-byte DDBLTFX
// on the stack (dwSize@+0x0 = 0x64, fill flags@+0x8 = 0x42 | (white ? 0xff0020 :
// 0)), Blt(NULL, NULL, NULL, 0x1020000, &fx) through the held surface, and on a
// non-zero (failed/lost) HRESULT colour-fill it white (0xff) or black (0) via Fill.
RVA(0x0013edb0, 0x78)
void CFileImage::Clear(i32 white) {
    CDDSurface* s = (CDDSurface*)this;
    i32 fx[0x19]; // DDBLTFX (0x64 bytes)
    i32* p = fx;
    for (i32 i = 0x19; i != 0; i--) {
        *p++ = 0;
    }
    fx[0] = 0x64;                         // dwSize @+0x0
    fx[2] = white ? (i32)0xff0062 : 0x42; // fill flags @+0x8
    i32 hr = s->m_8->vtbl->Blt(s->m_8, 0, 0, 0, 0x1020000, fx);
    if (hr != 0) {
        if (white != 0) {
            Fill(0xff);
        } else {
            Fill(0);
        }
    }
}

// ---------------------------------------------------------------------------
// CFileImage::SaveFile (ret 0x10) - the surface SAVE entry point. Bail (return 0)
// unless the surface is valid (slot-5 IsValid), `buf` is non-null and non-empty
// (*buf != 0), and `type` == 1. Then hand (buf, a3, a4) to the per-bit-depth
// dispatcher and return its result.
RVA(0x0013f910, 0x4a)
i32 CFileImage::SaveFile(char* buf, i32 type, void* a3, void* a4) {
    if (((CDDSurface*)this)->IsValid() == 0) {
        return 0;
    }
    if (buf == 0) {
        return 0;
    }
    if (*buf == 0) {
        return 0;
    }
    switch (type) {
        case 1:
            return SaveDispatch(buf, a3, a4);
        default:
            return 0;
    }
}

// ---------------------------------------------------------------------------
// CFileImage::SaveDispatch (ret 0xc) - pick the per-bit-depth file writer by the
// surface's raw bit depth m_a8 (8 -> Save8, 16 -> SaveRle16, 24 -> Save24),
// forwarding all three pass-through args; any other depth returns 0. Case bodies in
// retail .text order (24, 16, 8); the near case labels lower to MSVC's compare
// ladder.
RVA(0x00144350, 0x5f)
i32 CFileImage::SaveDispatch(void* a1, void* a2, void* a3) {
    switch (((CDDSurface*)this)->m_a8) {
        case 0x18:
            return Save24(a1, a2, a3);
        case 0x10:
            return SaveRle16(a1, a2, a3);
        case 8:
            return Save8(a1, a2, a3);
        default:
            return 0;
    }
}

// The live screen RGB-format unpack-shift table (file RVA 0x283ea0..0x283eb4 =
// VA 0x683ea0..). SaveRle16 uses them to expand a screen-native 16bpp pixel into
// an 8-bit-per-channel BGR triple (the inverse of the CLightFxRender Pack). Same
// differently-named symbols as elsewhere; reloc-masked.
DATA(0x00283ea0)
extern i32 g_rUp; // red   up-shift   (channel position in the 16bpp word)
DATA(0x00283ea4)
extern i32 g_gUp; // green up-shift
DATA(0x00283eac)
extern i32 g_rDown; // red   down-shift (scale 5/6-bit -> 8-bit)
DATA(0x00283eb0)
extern i32 g_gDown; // green down-shift
DATA(0x00283eb4)
extern i32 g_bDown; // blue  down-shift

// ---------------------------------------------------------------------------
// CFileImage::SaveRle16 (0x144640, ret 0xc) - the 16bpp surface -> 24bpp BMP file
// writer (DIRSURF.CPP). Bail unless the surface is valid (slot-5 IsValid), the
// name buffer `a1` is non-null and non-empty (*a1 != 0) and the surface is 16bpp
// (m_a8 == 0x10). Build a packed BITMAPFILEHEADER ("BM", bfSize = 3*w*h + 0x3a,
// bfOffBits = 0x3a) + a zeroed BITMAPINFOHEADER (biSize 0x28, biWidth/biHeight =
// surface w/h, biPlanes 1, biBitCount 0x18), operator-new a one-scanline 24bpp
// buffer, Lock the surface, open the CFileIO (mode 0x2001 / 0x1001 by a3), write
// the two headers, then walk the rows bottom-up expanding each 16bpp pixel into a
// BGR triple and writing the scanline. On any failure Unlock + free + close +
// return 0; on success return 1. The CFileIO stack object -> a /GX EH frame.
// @early-stop
// Two stacked walls (~52%): (1) the /GX shared-cleanup ladder - retail's per-reject
// unwind funclets converge on one Unlock/RezFree/~CFileIO tail that idiomatic C++
// scope-exit can't reproduce (docs/patterns/gx-state-machine-scalar-delete-cleanup.md);
// (2) register-allocation entropy in the 16->24bpp conversion inner loop. Logic is
// complete + correct; both are documented non-steerable plateaus.
RVA(0x00144640, 0x2be)
i32 CFileImage::SaveRle16(void* a1, void* a2, void* a3) {
    CDDSurface* s = (CDDSurface*)this;
    if (s->IsValid() == 0) {
        return 0;
    }
    if (a1 == 0) {
        return 0;
    }
    if (*(char*)a1 == 0) {
        return 0;
    }
    if (s->m_a8 != 0x10) {
        return 0;
    }

    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    bih.biSize = 0;
    bih.biWidth = 0;
    bih.biHeight = 0;
    *(i32*)&bih.biPlanes = 0;
    bih.biSizeImage = 0;
    bih.biXPelsPerMeter = 0;
    bih.biYPelsPerMeter = 0;
    bih.biClrUsed = 0;
    bih.biClrImportant = 0;

    strcpy((char*)&bfh, "BM");
    bfh.bfReserved1 = 0;
    bfh.bfReserved2 = 0;

    i32 height = *(i32*)(s->m_desc + 8);  // dwHeight
    i32 width = *(i32*)(s->m_desc + 0xc); // dwWidth
    bih.biHeight = height;
    bih.biWidth = width;
    bfh.bfSize = 3 * width * height + 0x3a;
    bih.biSize = 0x28;
    bih.biPlanes = 1;
    bih.biBitCount = 0x18;
    bfh.bfOffBits = 0x3a;

    u8* line = (u8*)operator new(3 * width * height + 0x3a);
    if (line == 0) {
        return 0;
    }

    u8* locked = (u8*)s->Lock(0);
    if (locked == 0) {
        RezFree(line);
        return 0;
    }

    CFileIO file;
    i32 ok;
    if (a3 != 0) {
        ok = file.Open((char*)a2, 0x2001, 0);
    } else {
        ok = file.Open((char*)a2, 0x1001, 0);
    }
    if (ok == 0) {
        s->m_8->vtbl->Unlock(s->m_8, 0);
        RezFree(line);
        return 0;
    }

    file.Seek(0, 2);
    file.Write(&bfh, 0xe);
    file.Write(&bih, 0x2c);

    for (i32 row = height - 1; row >= 0; row--) {
        u8* src = locked + row * *(i32*)(s->m_desc + 0x10);
        u8* dst = line;
        for (i32 x = 0; x < width; x++) {
            u16 px = *(u16*)src;
            src += 2;
            dst[0] = (u8)((u8)px << g_bDown);
            dst[1] = (u8)((u8)(px >> g_gUp) << g_gDown);
            dst[2] = (u8)((u8)(px >> g_rUp) << g_rDown);
            dst += 3;
        }
        file.Write(line, 3 * width);
    }

    s->m_8->vtbl->Unlock(s->m_8, 0);
    RezFree(line);
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::LoadKeyed (ret 0x18) - blit a source surface in with the control word
// forced to OR 0x40 (BlitSurf), and on success - unless the colour key is -1 -
// install it via FillPalette. Returns 1.
RVA(0x00148840, 0x47)
i32 CFileImage::LoadKeyed(void* surf, i32 width, i32 height, i32 a4, i32 a5, i32 key) {
    if (BlitSurf(surf, width, height, a4, a5 | 0x40) == 0) {
        return 0;
    }
    if (key != -1) {
        FillPalette((void*)key);
    }
    return 1;
}

// ---------------------------------------------------------------------------
// CFileImage::DecodeBmp
// `buf` is a whole .BMP file (packed BITMAPFILEHEADER + BITMAPINFOHEADER). Pull
// biWidth/biHeight/biBitCount, validate them against the destination surface's
// geometry (m_width/m_height) and require 8 or 24 bpp. `surf` is the palette context
// (m_palBitCount source bpp / m_palette palette / m_hasPalette have-palette). When the surface bpp
// differs from the file's, build a palette (for 8bpp: byte-reverse the BMP's
// in-file RGBQUADs into s_palBmp; for 24bpp reuse the surface palette) and blit
// through the remapping Blit; otherwise straight-copy via BlitDirect. The pixel
// data starts at buf + bfOffBits. Returns 1 on a successful blit, else 0.
RVA(0x00143fc0, 0x142)
void* CFileImage::DecodeBmp(char* surf, void* buf, u32 size) {
    CFileImage* pal = (CFileImage*)surf;
    BITMAPINFOHEADER* ih = (BITMAPINFOHEADER*)((char*)buf + 0xe);
    i32 width = ih->biWidth;
    i32 bitcount = ih->biBitCount;
    i32 height = ih->biHeight;
    if (m_width != width) {
        return 0;
    }
    if (m_height != height) {
        return 0;
    }
    if (bitcount != 8 && bitcount != 0x18) {
        return 0;
    }

    i32 remap = 0;
    i32 palBpp = pal->m_palBitCount;
    if (palBpp != bitcount) {
        remap = 1;
    }
    if (remap && palBpp == 8 && pal->m_hasPalette == 0) {
        return 0;
    }

    void* palette = 0;
    if (remap) {
        if (bitcount == 8) {
            u8* src = (u8*)buf + 0x36;
            u8* d = s_palBmp;
            do {
                d[0] = src[2];
                d[1] = src[1];
                d[2] = src[0];
                d[3] = 0;
                src += 4;
                d += 4;
            } while (d < s_palBmp + 0x400);
            palette = s_palBmp;
        }
    } else if (palBpp == 8 && pal->m_hasPalette != 0) {
        palette = pal->m_palette;
    }

    void* pixels = (char*)buf + ((BITMAPFILEHEADER*)buf)->bfOffBits;
    if (remap) {
        return Blit(pixels, bitcount, palette, 2) ? (void*)1 : (void*)0;
    }
    return BlitDirect(pixels, 2) ? (void*)1 : (void*)0;
}

// ---------------------------------------------------------------------------
// CFileImage::LoadBmp
// Open the file named by `path`; on failure return 0. GetLength(); if the length
// is zero return 0. `operator new` a buffer of that size; if it fails return 0.
// Read the file; if the read count != length, free + return 0. Else decode and
// return the decoder's result. The CFileIO dtor + buffer free run on every exit.
RVA(0x00144110, 0x156)
void* CFileImage::LoadBmp(char* name, char* path) {
    CFileIO file;

    if (!file.Open(path, 0, 0)) {
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

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    void* result = DecodeBmp(name, buf, len);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CFileImage::DecodePcx
// `buf` is a whole .PCX file (128-byte ZSoft header, pixels at +0x80). Geometry
// comes from the window (width = Xmax-Xmin+1 @ +8/+4, height = Ymax-Ymin+1 @
// +0xa/+6); NPlanes @ +0x41 picks 8bpp (1 plane) or 24bpp (3 planes). Validate vs
// the destination (m_width/m_height). When no remap is needed the planes are RLE-expanded
// straight into the surface (DecodeRun8/DecodeRun24); when the surface bpp differs
// the run is decoded into a scratch buffer (RunDecode1/RunDecode3) and then blit
// through the palette (built from the PCX trailing 768-byte VGA palette for 8bpp,
// or the surface palette for 24bpp). Returns 1 on success, 0 on failure.
RVA(0x00144ee0, 0x225)
void* CFileImage::DecodePcx(char* surf, void* buf, u32 size) {
    if (!buf) {
        return 0;
    }
    CFileImage* pal = (CFileImage*)surf;
    u8* hdr = (u8*)buf;
    i32 width = *(i16*)(hdr + 8) - *(i16*)(hdr + 4) + 1;
    i32 height = *(i16*)(hdr + 0xa) - *(i16*)(hdr + 6) + 1;
    u8 planes = hdr[0x41];

    i32 bitcount = 0;
    if (planes == 1) {
        bitcount = 8;
    } else if (planes == 3) {
        bitcount = 0x18;
    }
    if (bitcount == 0) {
        return 0;
    }
    if (m_width != width) {
        return 0;
    }
    if (m_height != height) {
        return 0;
    }

    i32 remap = 0;
    i32 palBpp = pal->m_palBitCount;
    if (palBpp != bitcount) {
        remap = 1;
    }
    if (remap && palBpp == 8 && pal->m_hasPalette == 0) {
        return 0;
    }

    void* palette = 0;
    if (remap) {
        if (bitcount == 8) {
            u8* src = (u8*)buf + size - 0x300;
            u8* d = s_palPcx;
            do {
                d[0] = *src++;
                d[1] = *src++;
                d[2] = *src++;
                d[3] = 0;
                d += 4;
            } while (d < s_palPcx + 0x400);
            palette = s_palPcx;
        } else if (palBpp == 8 && pal->m_hasPalette != 0) {
            palette = pal->m_palette;
        }
    }

    u8* pixels = (u8*)buf + 0x80;
    i32 ok;
    void* decoded = 0;
    if (!remap) {
        if (bitcount == 8) {
            ok = DecodeRun8(pixels);
        } else {
            ok = DecodeRun24(pixels);
        }
        if (!ok) {
            return 0;
        }
    } else {
        if (bitcount == 8) {
            decoded = RezAlloc(width * height);
            if (!decoded) {
                return 0;
            }
            ok = RunDecode1(decoded, pixels, width, height);
        } else {
            decoded = RezAlloc(width * height * 3);
            if (!decoded) {
                return 0;
            }
            ok = RunDecode3(decoded, pixels, width, height);
        }
        if (!ok) {
            RezFree(decoded);
            return 0;
        }
    }

    if (remap) {
        if (!Blit(decoded, bitcount, palette, 1)) {
            RezFree(decoded);
            return 0;
        }
    }
    if (decoded) {
        RezFree(decoded);
    }
    return (void*)1;
}

// ---------------------------------------------------------------------------
// CFileImage::LoadPcx
// Byte-identical to LoadBmp except for the per-format decode helper (DecodePcx).
RVA(0x00145110, 0x156)
void* CFileImage::LoadPcx(char* name, char* path) {
    CFileIO file;

    if (!file.Open(path, 0, 0)) {
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

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    void* result = DecodePcx(name, buf, len);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CFileImage::DecodePid
// `buf` is a .PID payload (libwap32 layout: flags @ +4, width @ +8, height @
// +0xc, run data @ +0x20). Width must be 4-aligned and the geometry must match
// the destination surface (this->m_width / m_height). `surf` is the palette context
// (m_palBitCount bpp / m_palette palette / m_hasPalette have-palette). When flags&0x80
// (EMBEDDED_PALETTE) the trailing 768-byte VGA palette is expanded into
// s_palPidData; otherwise the surface palette is used. The 8bpp run is expanded
// in place (DecodeRun8) or, when the surface bpp differs, into a scratch buffer
// (RunDecode1) and blit through the palette. flags&1 (TRANSPARENCY) installs the
// transparent colour (surf2) via FillPalette. Returns 1 on success, 0 on failure.
RVA(0x00145b10, 0x1b5)
void* CFileImage::DecodePid(char* surf, void* buf, u32 size, void* surf2) {
    CFileImage* pal = (CFileImage*)surf;
    u8* hdr = (u8*)buf;
    i32 flags = *(i32*)(hdr + 4);
    i32 width = *(i32*)(hdr + 8);
    i32 height = *(i32*)(hdr + 0xc);
    u8* data = hdr + 0x20;

    if (width & 3) {
        return 0;
    }
    if (m_width != width) {
        return 0;
    }
    if (m_height != height) {
        return 0;
    }

    void* palette = 0;
    if (pal->m_hasPalette != 0) {
        palette = pal->m_palette;
    }
    i32 remap = 0;
    i32 palBpp = pal->m_palBitCount;
    if (palBpp != 8) {
        remap = 1;
    }

    if (flags & PID_EMBEDDED_PALETTE) {
        if (size <= 0x300) {
            return 0;
        }
        u8* src = (u8*)buf + size - 0x300;
        u8* d = s_palPidData;
        do {
            d[0] = *src++;
            d[1] = *src++;
            d[2] = *src++;
            d[3] = 0;
            d += 4;
        } while (d < s_palPidData + 0x400);
        palette = s_palPidData;
    } else if (remap) {
        if (palette == 0) {
            return 0;
        }
        if (palBpp == 8 && pal->m_hasPalette == 0) {
            return 0;
        }
    }

    void* decoded = 0;
    if (!remap) {
        if (!DecodeRun8(data)) {
            return 0;
        }
    } else {
        decoded = RezAlloc(height * width);
        if (!decoded) {
            return 0;
        }
        if (!RunDecode1(decoded, data, width, height)) {
            RezFree(decoded);
            return 0;
        }
    }

    if (remap) {
        if (!Blit(decoded, 8, palette, 1)) {
            RezFree(decoded);
            return 0;
        }
    }
    if (decoded) {
        RezFree(decoded);
    }
    if (flags & PID_TRANSPARENCY) {
        FillPalette(surf2);
    }
    return (void*)1;
}

// ---------------------------------------------------------------------------
// CFileImage::LoadPid
// Like LoadBmp/LoadPcx, but: (1) it does NOT guard length==0 - it allocates the
// buffer for whatever GetLength() returns and only null-checks the allocation;
// (2) the decoder takes a fourth pass-through arg (a3).
RVA(0x00145cd0, 0x130)
void* CFileImage::LoadPid(char* name, char* path, void* a3) {
    CFileIO file;

    if (!file.Open(path, 0, 0)) {
        return 0;
    }

    u32 len = file.GetLength();
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

// ---------------------------------------------------------------------------
// CFileImage::DecodePcxData
// The low-level PID-ish run decoder shared by DecodePcxEx. `surf` is the source
// header (flags @ +4, width @ +8, height @ +0xc, run data @ +0x20); a4 is a
// control word whose high bits are toggled from flags&4 / flags&2; a5 the
// transparency colour. Width must be 4-aligned. The destination plane is set up
// via BlitSurf, then the run is expanded in place (DecodeRun8) or, when the
// surface bpp differs, into a scratch buffer (RunDecode1) and blit through the
// palette (built from the trailing 768-byte VGA palette when flags&0x80 is set,
// else the surface palette). flags&1 (TRANSPARENCY) installs a5 via FillPalette.
RVA(0x001457a0, 0x22c)
i32 CFileImage::DecodePcxData(void* surf, void* buf, i32 size, i32 a4, i32 a5) {
    u8* hdr = (u8*)buf; // the source PID/PCX header
    CFileImage* dst = (CFileImage*)surf;
    i32 flags = *(i32*)(hdr + 4);
    i32 w = *(i32*)(hdr + 8);
    i32 h = *(i32*)(hdr + 0xc);
    u8* data = hdr + 0x20;

    if (w & 3) {
        return 0;
    }
    if (flags & PID_SYSTEM_MEMORY) {
        a4 = (a4 & ~0x4000) | 0x800;
    } else if (flags & PID_VIDEO_MEMORY) {
        a4 = a4 & ~0x800;
    }

    void* palette = 0;
    if (dst->m_hasPalette) {
        palette = dst->m_palette;
    }
    i32 remap = 0;
    i32 palBpp = dst->m_palBitCount;
    if (palBpp != 8) {
        remap = 1;
    }

    if (flags & PID_EMBEDDED_PALETTE) {
        if ((u32)size <= 0x300) {
            return 0;
        }
        u8* src = hdr + size - 0x300;
        u8* d = s_palPcxData;
        do {
            d[0] = *src++;
            d[1] = *src++;
            d[2] = *src++;
            d[3] = 0;
            d += 4;
        } while (d < s_palPcxData + 0x400);
        palette = s_palPcxData;
    } else {
        if (remap) {
            if (palette == 0) {
                return 0;
            }
            if (palBpp == 8 && dst->m_hasPalette == 0) {
                return 0;
            }
        }
    }

    if (!BlitSurf(dst, w, h, 0, a4)) {
        return 0;
    }

    void* decoded = 0;
    if (!remap) {
        if (!DecodeRun8(data)) {
            return 0;
        }
    } else {
        decoded = RezAlloc(h * w);
        if (!decoded) {
            return 0;
        }
        if (!RunDecode1(decoded, data, w, h)) {
            RezFree(decoded);
            return 0;
        }
    }

    if (remap) {
        if (!Blit(decoded, 8, palette, 1)) {
            RezFree(decoded);
            return 0;
        }
    }
    if (decoded) {
        RezFree(decoded);
    }
    if (flags & PID_TRANSPARENCY) {
        FillPalette((void*)a5);
    }
    return 1;
}

// ===========================================================================
// CFileImage::DecodePcxEx
//
// Opens a PCX file, reads data, calls DecodePcxData.
// ===========================================================================
RVA(0x001459d0, 0x135)
i32 CFileImage::DecodePcxEx(char* name, char* path, void* a3, void* a4) {
    CFileIO file;

    if (!file.Open(path, 0, 0)) {
        return 0;
    }

    u32 len = file.GetLength();
    void* buf = operator new(len);
    if (!buf) {
        return 0;
    }

    if (file.Read(buf, len) != len) {
        operator delete(buf);
        return 0;
    }

    i32 result = DecodePcxData(name, buf, len, (i32)a3, (i32)a4);
    operator delete(buf);
    return result;
}

// ---------------------------------------------------------------------------
// CFileImage::ResolveEx
// The surface-blit format dispatcher: like Resolve but for the *Data decoders
// that blit straight into the destination surface. The control word is OR'd with
// 0x40 up front; `type` (1=BMP, 2=PCX, 4=PID) selects DecodeBmpData/DecodePcxData2/
// DecodePcxData, each handed (surf, buf, size, ctrl); PID also takes the
// transparency colour. After a successful decode the transparency colour is
// installed via FillPalette unless it is "no key" (-1) or the format already
// handled it (PID = type 4). Returns 1 on success, else 0. The case labels lower
// to the running-subtract chain (switch, bodies in retail .text order 4, 2, 1).
RVA(0x00148890, 0xad)
i32 CFileImage::ResolveEx(void* surf, void* buf, i32 type, u32 size, i32 ctrl, i32 trans) {
    if (size == 0) {
        return 0;
    }
    i32 c = ctrl | 0x40;
    switch (type) {
        case FMT_PID:
            if (!DecodePcxData(surf, buf, size, c, trans)) {
                return 0;
            }
            break;
        case FMT_PCX:
            if (!DecodePcxData2(surf, buf, size, c)) {
                return 0;
            }
            break;
        case FMT_BMP:
            if (!DecodeBmpData(surf, buf, size, c)) {
                return 0;
            }
            break;
        default:
            return 0;
    }
    if (trans != -1 && type != FMT_PID) {
        FillPalette((void*)trans);
    }
    return 1;
}

// ===========================================================================
// Class-metadata annotations (EOF-hosted). This TU's REZ-loader family is named
// distinctly (CRezImage / CRezImageSource / CRezSurfaceItem) so it no longer clashes
// with the RTTI CImage cluster in CImage.h. CFileImage stays the shared DIRSURF
// surface (partial model here, canonical in CFileImage.h). Only names
// first-represented in this TU are annotated below. CFileImageElement is a slot-0
// dtor view (no emitted vtable -> VTBL skip).
// ===========================================================================
SIZE_UNKNOWN(CFileImageElement);
SIZE_UNKNOWN(CImageExtLoader);
SIZE_UNKNOWN(CByteArrayMember);
SIZE_UNKNOWN(CImageSurfaceItemInit);
SIZE_UNKNOWN(CImageFactory);
