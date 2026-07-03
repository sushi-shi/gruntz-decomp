// SoundBankLoad.cpp - CGruntzSoundInnerZ::Load (vtable slot 6, +0x18; retail RVA
// 0x138aa0), the load-a-file setup of an inner sound bank (Dsndmgr module,
// C:\Proj\Dsndmgr\, the 0x138xxx AIL/DirectSound region). Load(name, arg) either
// forwards to the special-name handler (slot 15, LoadSpecial) when `name` is the pooled
// ".." token, or opens `name` as a file, slurps it whole into the owned m_loadBuffer
// (operator new), and hands it to the one-time decode setup (slot 5, DecodeBuf). The
// destructible stack CFile forces the /GX EH frame.
#include <Dsndmgr/CGruntzSoundZ.h> // the real CGruntzSoundInnerZ (+ MFC CFile via <Mfc.h>)
#include <rva.h>

// The engine throwing allocator (global operator new @0x1b9b46, NAFXCW). Reloc-masked.
void* operator new(u32 n);

// The name compare against the pooled ".." token (0x120090, __cdecl 2-arg; strcmp/
// _mbscmp). Reloc-masked rel32; the ".." is the shared $SG constant (0x5ee8ec) the
// ButeMgr parser also references, so reach it by symbol so the DIR32 pairs.
extern "C" i32 SbNameCmp(const char* a, const char* b); // 0x120090
DATA(0x001ee8ec)
extern char g_dotDot[]; // 0x5ee8ec  ".."

// Load: the special ".." name forwards to the slot-15 handler (LoadSpecial); otherwise
// open `name`, require >= 4 bytes, slurp it whole into m_loadBuffer, and run the
// one-time decode setup (slot 5, DecodeBuf). Each failure tears down the local CFile and
// returns 0; the CFile's throwing ctor forces the /GX frame. __thiscall, ret 8.
//
// a1/a2 are `const char*` name / config args passed as i32 by the shared
// homogenized-i32 CGruntzSoundInnerZ virtual convention (CreateBank2/PlayCreate all
// forward i32; retyping the slot signatures cascades cross-TU, so the two leaf casts
// stay - the same pattern CGruntzSoundZ::Play_138840 uses).
//
// @early-stop
// /GX EH-frame wall: the ".."-token branch, the CFile open/GetLength/new/Read/decode
// chain, the slot-5/15 virtual dispatches and the m_loadBuffer store are byte-faithful,
// but the local CFile's stack-slot scheduling inside the /GX frame + the EH-state
// numbering (the documented eh-scoped-local wall) and the differently-named
// SbNameCmp/operator-new reloc operands diverge. Logic complete; final sweep.
RVA(0x00138aa0, 0x175)
i32 CGruntzSoundInnerZ::Load(i32 a1, i32 a2) {
    const char* name = (const char*)a1;
    if (SbNameCmp(name, g_dotDot) != 0) {
        CFile file;
        if (!file.Open(name, 0, 0)) {
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
        return DecodeBuf((i32)m_loadBuffer, length, a2);
    }
    return LoadSpecial(name, a2);
}
