#include <Dsndmgr/GruntzSoundZ.h>
#include <rva.h>
#include <string.h>

DATA(0x001ee8ec)
char g_dot[] = ".";

RVA(0x00138aa0, 0x175)
i32 CGruntzSoundInnerZ::Load(const char* path, const char* name) {
    if (strstr(path, g_dot) == 0) {
        return LoadSpecial(path, name);
    }
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
