// SaveScreenshot.cpp - SaveScreenshot (0x114ff0). The in-game "dump the screen to
// a Gruntz<NNNN>.BMP" helper (the F12 screen-grab). __cdecl(arg1, bute, owner,
// arg4, arg5, name, arg7):
//   - bails returning 0 on any of arg1==0 / bute==0 / owner==0 / owner->m_30==0.
//   - when no explicit file name is supplied (name==0) it reads + bumps the
//     "Screen Dump Count" key in the registry/config object (bute->GetInt /
//     WriteInt) and formats "Gruntz%04i.BMP" into a local buffer.
//   - grabs the back-surface (owner->m_30->m_1c), asks it to build a capture image
//     sized to the screen (g_mgrSettings->m_modeW x m_modeH), blits the screen rect into
//     it, and saves it to the BMP path; the surface frees the image on both paths.
// Frameless (no destructible C++ local) - lives in a base /O2 unit. Only offsets /
// code bytes are load-bearing; the surface/image/config callees are reloc-masked
// __thiscall externals modeled with no body.
#include <rva.h>

#include <Mfc.h> // afx-first (TU pulls MFC via unified CObject; superset of Win32.h) // wsprintfA
#include <Gruntz/GameRegistry.h>
namespace Utils {
    class RegistryHelper {
    public:
        unsigned long GetValueDword(char* k, unsigned long d);
        i32 SetValueDword(char* k, unsigned long v);
    };
} // namespace Utils

// The registry/config object (arg2). GetInt/WriteInt are 2-arg __thiscall helpers.
struct ScrConfig {
    // GetInt @0x1395d0 IS Utils::RegistryHelper::GetValueDword; cast at the call.
    // WriteInt @0x139460 IS Utils::RegistryHelper::SetValueDword; cast at the call.
};

// The capture image built by the surface (returned by CreateImage). Capture()
// blits a screen rect into it; Save() writes it to a BMP path.
struct ScrImage {
    // FUN_0013eef0 __thiscall: capture into the image (ret BOOL nonzero on error).
    i32 Capture(void* descA, i32 a1, void* descB, i32 flags, i32 z);
    // FUN_0013f910 __thiscall: write the image to a path (ret result).
    i32 Save(char* path, i32 a1, i32 a2, i32 a3);
};

// The back/render surface (owner->m_30->m_1c).
struct ScrSurface {
    // FUN_00142e60 __thiscall, ret 0x18: build a capture image.
    ScrImage* CreateImage(i32 a1, i32 a2, i32 bpp, i32 a4, i32 a5);
    void DeleteImage(ScrImage* img); // FUN_00142160 __thiscall
};

struct ScrSurfHost { // owner->m_30 points here; +0x1c is the surface
    char m_pad00[0x1c];
    ScrSurface* m_1c; // +0x1c
};

struct ScrOwner { // arg3
    char m_pad00[0x30];
    ScrSurfHost* m_30; // +0x30
};

// The global game/manager settings singleton (*0x64556c); m_modeW/m_modeH = the screen
// capture width/height.
DATA(0x0024556c)
extern CGameRegistry* g_mgrSettings;

// @source: decomp-xref
// @early-stop
// regalloc-coloring wall (~96.8%): frame/instruction-selection/constants/strings
// are byte-faithful (frame 0xa8, all locals + the descriptor block at the retail
// offsets), but MSVC colored the two callee-saved registers oppositely - retail
// pins the persistent zero (and the reused return temp) in esi and cnt/img in edi,
// while this /O2 recompile pins them edi<->esi swapped; that propagates through
// every null-compare/push. The descriptor fill also CSEs the g_mgrSettings m_modeW/m_modeH
// loads where retail re-reads each twice (a scheduling choice). Logic complete; the
// coloring/scheduling is the allocator's, not source-steerable. See
// docs/patterns/zero-register-pinning.md + pin-local-for-callee-saved-reg.md.
RVA(0x00114ff0, 0x1b3)
i32 SaveScreenshot(
    i32 arg1,
    ScrConfig* bute,
    ScrOwner* owner,
    i32 arg4,
    i32 arg5,
    char* name,
    i32 arg7
) {
    char nameBuf[0x80];
    i32 descB[6];
    i32 descA[4];

    if (arg1 == 0) {
        return 0;
    }
    if (bute == 0) {
        return 0;
    }
    if (owner == 0) {
        return 0;
    }
    if (owner->m_30 == 0) {
        return 0;
    }
    if (name == 0) {
        i32 cnt = ((Utils::RegistryHelper*)bute)->GetValueDword("Screen Dump Count", 0) + 1;
        ((Utils::RegistryHelper*)bute)->SetValueDword("Screen Dump Count", cnt);
        wsprintfA(nameBuf, "Gruntz%04i.BMP", cnt);
        name = nameBuf;
    }

    ScrSurface* surf = owner->m_30->m_1c;
    if (surf == 0) {
        return 0;
    }
    ScrImage* img = surf->CreateImage(arg4, arg5, 0x10, 0, -1);
    if (img == 0) {
        return 0;
    }

    CGameRegistry* mgr = g_mgrSettings;
    descA[0] = 0;
    descA[1] = 0;
    descA[2] = 0;
    descA[3] = 0;
    descB[0] = 0;
    descB[1] = 0;
    descB[2] = 0;
    descB[3] = 0;
    descA[2] = mgr->m_modeW;
    descB[5] = mgr->m_modeH;
    descA[3] = mgr->m_modeH;
    descB[4] = mgr->m_modeW;
    descB[2] = arg4;
    descB[3] = arg5;
    if (img->Capture(descB, arg1, descA, 0x1000000, 0)) {
        surf->DeleteImage(img);
        return 0;
    }
    i32 r = img->Save(name, 1, 0, arg7);
    surf->DeleteImage(img);
    return r;
}
SIZE_UNKNOWN(ScrConfig);
SIZE_UNKNOWN(ScrImage);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(ScrOwner);
SIZE_UNKNOWN(ScrSurfHost);
SIZE_UNKNOWN(ScrSurface);
