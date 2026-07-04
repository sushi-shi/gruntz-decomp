// PaletteLerp.cpp - the palette colour-interpolation "tick" (RVA 0x1480a0).
//
// A DDrawMgr palette-fade object: each tick lerps the live palette (m_livePalette) from a
// captured source (m_sourcePalette) toward either a target palette (m_targetPalette) or a fixed RGB
// (m_fixedR..m_fixedB), proportionally to elapsed/duration, then pushes the changed range
// to the owning palette object's SetEntries slot. Field names are placeholders;
// only offsets + code bytes are load-bearing.
#include <rva.h>

#include <Ints.h>
#include <Win32.h> // WINAPI (windows.h) for the g_pTimeGetTime import-pointer type

#include <ddraw.h> // the real IDirectDrawPalette (DX6 SDK, vendor/directx6/Include)

// Retail caches the timeGetTime entry point in a game-owned global pointer and
// calls through it (ff 15), not via the import thunk. CANONICAL: _g_pTimeGetTime
// @ RVA 0x2c4650 (pinned in cplay/globals) - extern "C" so the reloc binds the one
// symbol per RVA at whole-game link (was a C++-mangled decl pinned at the VA-as-RVA
// 0x6c4650, an orphan symbol_names row).
extern "C" u32(WINAPI* g_pTimeGetTime)();

// The owning palette object is the real DirectDraw palette COM interface
// (IDirectDrawPalette, from the DX6 SDK's <ddraw.h>): only SetEntries (slot 6, +0x18)
// is invoked, so `m_paletteObj->SetEntries(...)` lowers to the same `mov eax,[iface];
// call [eax+0x18]` COM dispatch the hand-rolled ...Z view did (slot order is frozen
// across DX3/5/6). COM => __stdcall with the interface pointer as the hidden first arg.
struct PaletteLerp {
    char m_pad00[4];
    IDirectDrawPalette* m_paletteObj; // +0x04 owning DirectDraw palette (SetEntries at +0x18)
    char m_pad08[4];
    u8* m_livePalette; // +0x0c live palette bytes (destination)
    char m_pad10[4];
    u8* m_targetPalette; // +0x14 target palette bytes (0 -> use fixed colour)
    u8* m_sourcePalette; // +0x18 captured source palette bytes
    u8 m_fixedR;         // +0x1c fixed target R
    u8 m_fixedG;         // +0x1d fixed target G
    u8 m_fixedB;         // +0x1e fixed target B
    char m_pad1f[1];
    i32 m_durationMs;      // +0x20 duration (ms)
    i32 m_startTimeMs;     // +0x24 start time (ms)
    i32 m_lastElapsedMs;   // +0x28 last applied elapsed
    i32 m_firstColorIndex; // +0x2c first colour index
    i32 m_colorCount;      // +0x30 colour count
    i32 m_active;          // +0x34 active flag
    void Finish();         // 0x148250 completion handler (snap to final + retire)
    i32 Tick();
};
SIZE_UNKNOWN(PaletteLerp);

RVA(0x001480a0, 0x1a7)
i32 PaletteLerp::Tick() {
    if (m_active == 0) {
        return 0;
    }
    u32 dt = g_pTimeGetTime() - m_startTimeMs;
    if (dt >= (u32)m_durationMs) {
        Finish();
        return 0;
    }
    if (m_targetPalette != 0) {
        if (dt != (u32)m_lastElapsedMs) {
            i32 i = m_firstColorIndex;
            if (i < m_firstColorIndex + m_colorCount) {
                do {
                    m_livePalette[i * 4] =
                        (char)((i32)(((u32)m_targetPalette[i * 4] - (u32)m_sourcePalette[i * 4])
                                     * dt)
                               / m_durationMs)
                        + m_sourcePalette[i * 4];
                    m_livePalette[i * 4 + 1] = (char)((i32)(((u32)m_targetPalette[i * 4 + 1]
                                                             - (u32)m_sourcePalette[i * 4 + 1])
                                                            * dt)
                                                      / m_durationMs)
                                               + m_sourcePalette[i * 4 + 1];
                    m_livePalette[i * 4 + 2] = (char)((i32)(((u32)m_targetPalette[i * 4 + 2]
                                                             - (u32)m_sourcePalette[i * 4 + 2])
                                                            * dt)
                                                      / m_durationMs)
                                               + m_sourcePalette[i * 4 + 2];
                    i++;
                } while (i < m_firstColorIndex + m_colorCount);
            }
            m_paletteObj->SetEntries(
                0,
                m_firstColorIndex,
                m_colorCount,
                (LPPALETTEENTRY)(m_livePalette + m_firstColorIndex * 4)
            );
        }
    } else {
        if (dt != (u32)m_lastElapsedMs) {
            i32 i = m_firstColorIndex;
            if (i < m_firstColorIndex + m_colorCount) {
                do {
                    m_livePalette[i * 4] =
                        (char)((i32)(((u32)m_fixedR - (u32)m_sourcePalette[i * 4]) * dt)
                               / m_durationMs)
                        + m_sourcePalette[i * 4];
                    m_livePalette[i * 4 + 1] =
                        (char)((i32)(((u32)m_fixedG - (u32)m_sourcePalette[i * 4 + 1]) * dt)
                               / m_durationMs)
                        + m_sourcePalette[i * 4 + 1];
                    m_livePalette[i * 4 + 2] =
                        (char)((i32)(((u32)m_fixedB - (u32)m_sourcePalette[i * 4 + 2]) * dt)
                               / m_durationMs)
                        + m_sourcePalette[i * 4 + 2];
                    i++;
                } while (i < m_firstColorIndex + m_colorCount);
            }
            m_paletteObj->SetEntries(
                0,
                m_firstColorIndex,
                m_colorCount,
                (LPPALETTEENTRY)(m_livePalette + m_firstColorIndex * 4)
            );
        }
    }
    m_lastElapsedMs = dt;
    return 1;
}
