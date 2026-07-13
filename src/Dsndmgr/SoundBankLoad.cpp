// SoundBankLoad.cpp - CGruntzSoundInnerZ::Load (vtable slot 6, +0x18; retail RVA
// 0x138aa0), the load-a-file setup of an inner sound bank (Dsndmgr module,
// C:\Proj\Dsndmgr\, the 0x138xxx AIL/DirectSound region). Load(name, arg) either
// forwards to the special-name handler (slot 15, LoadSpecial) when `name` is the pooled
// ".." token, or opens `name` as a file, slurps it whole into the owned m_loadBuffer
// (operator new), and hands it to the one-time decode setup (slot 5, DecodeBuf). The
// destructible stack CFile forces the /GX EH frame.
#include <Dsndmgr/GruntzSoundZ.h> // the real CGruntzSoundInnerZ (+ MFC CFile via <Mfc.h>)
#include <rva.h>
#include <string.h> // strstr (0x120090)

// (Global scalar operator new - the NAFXCW allocator at 0x1b9b46 - comes from the
// real <Mfc.h> via CGruntzSoundZ.h; no local forward-decl needed.)

// The name compare against the pooled ".." token (0x120090 strstr, __cdecl 2-arg;
// _mbscmp). Reloc-masked rel32; the ".." is the shared $SG constant (0x5ee8ec) the
// ButeMgr parser also references, so reach it by symbol so the DIR32 pairs.
DATA(0x001ee8ec)
char g_dotDot[] = "."; // 0x5ee8ec  ".."

// Load: the special ".." name forwards to the slot-15 handler (LoadSpecial); otherwise
// open `name`, require >= 4 bytes, slurp it whole into m_loadBuffer, and run the
// one-time decode setup (slot 5, DecodeBuf). Each failure tears down the local CFile and
// returns 0; the CFile's throwing ctor forces the /GX frame. __thiscall, ret 8.
//
// path/name are the real `const char*` (source file / registration name); the whole
// CGruntzSoundInnerZ create/load chain (DecodeBuf/CreateBank/PlayCreate*) is now typed
// with real pointer/size types, so no homogenized-i32 leaf casts remain here.
//
// @early-stop
// /GX EH-frame wall: the ".."-token branch, the CFile open/GetLength/new/Read/decode
// chain, the slot-5/15 virtual dispatches and the m_loadBuffer store are byte-faithful,
// but the local CFile's stack-slot scheduling inside the /GX frame + the EH-state
// numbering (the documented eh-scoped-local wall) and the differently-named
// strstr/operator-new reloc operands diverge. Logic complete; final sweep.
RVA(0x00138aa0, 0x175)
i32 CGruntzSoundInnerZ::Load(const char* path, const char* name) {
    if (strstr(path, g_dotDot) != 0) {
        CFile file;
        if (!file.Open(path, 0, 0)) {
            return 0;
        }
        u32 length = file.GetLength();
        if (length < 4) {
            return 0;
        }
        m_loadBuffer = (char*)operator new(length);
        if (m_loadBuffer == 0) {
            return 0;
        }
        if (file.Read(m_loadBuffer, length) != length) {
            return 0;
        }
        return DecodeBuf(m_loadBuffer, length, name);
    }
    return LoadSpecial(path, name);
}
