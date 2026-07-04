// FileImageLoadByExt.cpp - CFileImage::LoadByExt (0x148940), the .BMP/.PCX/.PID
// file-extension dispatcher (== the DIRSURF.CPP surface). Re-homed from
// src/Stub/CFileImageDecode.cpp; uses the single-source CFileImage in <Image/Image.h>
// (no local view). The default-loader branch is CFileImage::Load (the RT_BITMAP loader
// at 0x144270, body in the ApiCallers TU) - the same __thiscall `this`. Only offsets /
// code bytes are load-bearing; helpers are reloc-masked externals.
#include <Mfc.h> // afx-first (Image.h uses MFC/Win32 types)

#include <Image/Image.h>

#include <rva.h>

// The engine's own strrchr / case-insensitive compare (cdecl C-linkage helpers).
extern "C" char* RezStrrchr(const char* s, i32 c);       // FUN_00120680 (_RezStrrchr)
extern "C" i32 RezStricmp(const char* a, const char* b); // FUN_0011fdf0 (_RezStricmp)

// CFileImage::LoadByExt - load an image by inspecting its file
// extension. Forces the IMAGEZ flag (|0x40), finds the extension, and dispatches
// to LoadFile2 (.BMP) / LoadFile (.PCX) / DecodePcxEx (.PID) or the default loader
// (Load @0x144270, the same __thiscall `this`). On a successful load (except the
// .PID path) it fills the palette from a4 when a4 != -1.
// @source: string-xref (.BMP/.PCX/.PID extension table)
//
// @early-stop
// scheduling wall (~99.9%): MSVC swaps the ORDER of the two independent tail loads at
// 0xdd - retail emits `mov esi,[esp+0x20]` (a4) then `mov eax,[esp+0x1c]` (doFill);
// the recompile emits them reversed. Same regs, same short-circuit, only the load
// schedule differs; all other code bytes are byte-identical (llvm-objdump -dr base vs
// target). A pure scheduler tie-break that flips on ANY change to the widely-included
// <Image/Image.h> symbol table: it had incidentally drifted to 100% at one baseline,
// and folding the CScanlineSurface/CImageSurfaceNode views onto CRezImage (adding the
// 7 surface methods to the shared header) re-triggered it. Not source-steerable
// (reordering the `&&` -> 96%); the correct shared-class shape is kept over the
// coin-flip byte-match (per the no-multiple-views mandate).
RVA(0x00148940, 0x102)
i32 CFileImage::LoadByExt(CDDrawPtrCollections* info, char* path, i32 flags, i32 key) {
    flags |= 0x40;
    i32 doFill = 1;
    char* ext = RezStrrchr(path, '.');
    if (ext != 0 && RezStricmp(ext, ".BMP") == 0) {
        if (LoadFile2(info, path, flags) == 0) {
            return 0;
        }
    } else if (ext != 0 && RezStricmp(ext, ".PCX") == 0) {
        if (LoadFile(info, path, flags) == 0) {
            return 0;
        }
    } else if (ext != 0 && RezStricmp(ext, ".PID") == 0) {
        if (DecodePcxEx(info, path, (void*)flags, (void*)key) == 0) {
            return 0;
        }
        doFill = 0;
    } else if (this->Load((i32)info, path, flags) == 0) {
        return 0;
    }
    if (key != -1 && doFill != 0) {
        FillPalette(key);
    }
    return 1;
}
