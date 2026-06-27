// m5_PaletteLerp.cpp - the palette colour-interpolation "tick" (RVA 0x1480a0).
//
// A DDrawMgr palette-fade object: each tick lerps the live palette (m_0c) from a
// captured source (m_18) toward either a target palette (m_14) or a fixed RGB
// (m_1c..m_1e), proportionally to elapsed/duration, then pushes the changed range
// to the owning palette object's SetEntries slot. Field names are placeholders;
// only offsets + code bytes are load-bearing.
#include <rva.h>

#include <Ints.h>

// Retail caches the timeGetTime entry point in a game-owned global pointer and
// calls through it (ff 15), not via the import thunk.
DATA(0x006c4650)
extern u32(__stdcall* g_pTimeGetTime)();

struct PalObj {
    struct Vtbl {
        void* s0[6];                                              // slots 0x00..0x14
        void(__stdcall* SetEntries)(PalObj*, i32, i32, i32, void*); // +0x18
    };
    Vtbl* vptr;
};

struct PaletteLerp {
    char m_pad00[4];
    PalObj* m_04; // +0x04 owning palette object (vtable SetEntries at +0x18)
    char m_pad08[4];
    u8* m_0c; // +0x0c live palette bytes (destination)
    char m_pad10[4];
    u8* m_14; // +0x14 target palette bytes (0 -> use fixed colour)
    u8* m_18; // +0x18 captured source palette bytes
    u8 m_1c;  // +0x1c fixed target R
    u8 m_1d;  // +0x1d fixed target G
    u8 m_1e;  // +0x1e fixed target B
    char m_pad1f[1];
    i32 m_20; // +0x20 duration (ms)
    i32 m_24; // +0x24 start time (ms)
    i32 m_28; // +0x28 last applied elapsed
    i32 m_2c; // +0x2c first colour index
    i32 m_30; // +0x30 colour count
    i32 m_34;       // +0x34 active flag
    void Finish();  // 0x148250 completion handler (snap to final + retire)
    i32 Tick();
};

RVA(0x001480a0, 0x1a7)
i32 PaletteLerp::Tick() {
    if (m_34 == 0) {
        return 0;
    }
    u32 dt = g_pTimeGetTime() - m_24;
    if (dt >= (u32)m_20) {
        Finish();
        return 0;
    }
    if (m_14 != 0) {
        if (dt != (u32)m_28) {
            i32 i = m_2c;
            if (i < m_2c + m_30) {
                do {
                    m_0c[i * 4] =
                        (char)((i32)(((u32)m_14[i * 4] - (u32)m_18[i * 4]) * dt) / m_20) + m_18[i * 4];
                    m_0c[i * 4 + 1] =
                        (char)((i32)(((u32)m_14[i * 4 + 1] - (u32)m_18[i * 4 + 1]) * dt) / m_20) +
                        m_18[i * 4 + 1];
                    m_0c[i * 4 + 2] =
                        (char)((i32)(((u32)m_14[i * 4 + 2] - (u32)m_18[i * 4 + 2]) * dt) / m_20) +
                        m_18[i * 4 + 2];
                    i++;
                } while (i < m_2c + m_30);
            }
            m_04->vptr->SetEntries(m_04, 0, m_2c, m_30, m_0c + m_2c * 4);
        }
    } else {
        if (dt != (u32)m_28) {
            i32 i = m_2c;
            if (i < m_2c + m_30) {
                do {
                    m_0c[i * 4] =
                        (char)((i32)(((u32)m_1c - (u32)m_18[i * 4]) * dt) / m_20) + m_18[i * 4];
                    m_0c[i * 4 + 1] =
                        (char)((i32)(((u32)m_1d - (u32)m_18[i * 4 + 1]) * dt) / m_20) + m_18[i * 4 + 1];
                    m_0c[i * 4 + 2] =
                        (char)((i32)(((u32)m_1e - (u32)m_18[i * 4 + 2]) * dt) / m_20) + m_18[i * 4 + 2];
                    i++;
                } while (i < m_2c + m_30);
            }
            m_04->vptr->SetEntries(m_04, 0, m_2c, m_30, m_0c + m_2c * 4);
        }
    }
    m_28 = dt;
    return 1;
}
