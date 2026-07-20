#include <Dsndmgr/GruntzSoundZ.h> // the real CGruntzSoundInnerZ (+ MFC CFile via <Mfc.h>)
#include <rva.h>
#include <string.h> // strstr (0x120090)

DATA(0x001ee8ec)
char g_dot[] = "."; // 0x5ee8ec

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
    if (strstr(path, g_dot) != 0) {
        CFile file;
        if (!file.Open(path, 0, 0)) {
            return 0;
        }
        u32 length = file.GetLength();
        if (length < 4) {
            return 0;
        }
        m_loadBuffer = static_cast<char*>(operator new(length));
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
