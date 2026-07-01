// GruntInfoText.cpp - the full-screen level/grunt info-text panel painter
// (retail 0x0d95f0, graduated out of the ApiCallers stub pool). __thiscall on an
// unrecovered owning object: reads m_1c (level/stage number), m_20 (tool/mode
// selector) and m_c (the EngStr render surface). Builds four text lines via MFC
// CString + string resources, strips the level-rez basename off the game
// registry's level name, then paints them into four fixed 640-wide rects with
// EngStr_DrawText. The four destructible CString locals force the /GX EH frame.
//
// Field/host names are placeholders (m_<hexoffset>); only OFFSETS + code bytes
// are load-bearing. Engine callees / globals are reloc-masked (no body).
#include <Mfc.h> // CString + <windows.h> (RECT / SetRect / wsprintfA)

#include <Ints.h>
#include <rva.h>

// The game-manager singleton at *0x64556c (dedup winner name `_g_64556c`); only
// the mode discriminator (m_134), the single-page flag (m_130) and the level-name
// query used here are modeled.
struct WwdGameRegInfo {
    char m_pad0[0x130];
    i32 m_130; // +0x130 single-/multi-page flag
    i32 m_134; // +0x134 mode discriminator (1 story, 2/3 alt, else empty)
    // QueryLevelName - __thiscall accessor returning the level's rez path by value
    // (retail FUN_004928c0 via ILT 0x2531). External / reloc-masked.
    CString QueryLevelName();
};
extern "C" WwdGameRegInfo* g_64556c;

// The engine empty-string sentinel (0x6293f4) and the multi-page flag
// (DAT_006455f0); both reloc-masked DIR32 data referents.
extern "C" char g_emptyString[]; // 0x6293f4
extern "C" i32 g_6455f0;         // 0x6455f0

// EngStr text-draw forwarder (__cdecl, 0x115440) - obj, string, rect, a font
// selector, then five trailing style args. Reloc-masked rel32 callee.
extern "C" void EngStr_DrawText(
    void* obj,
    const CString* str,
    const RECT* rc,
    i32 a3,
    i32 a4,
    i32 a5,
    i32 a6,
    i32 a7,
    i32 a8
);

// CRT scanners (retail 0x120120 / 0x120680); declared as plain calls so the
// optimizer does not inline them (retail keeps the call).
extern "C" char* strchr(const char*, i32);
extern "C" char* strrchr(const char*, i32);

// Placeholder host for the retail 0x0d95f0 method (real owning class unrecovered).
struct GruntInfoTextHost {
    char m_pad0[0xc];
    void* m_c; // +0x0c EngStr render surface
    char m_pad10[0x1c - 0x10];
    i32 m_1c; // +0x1c level/stage number
    i32 m_20; // +0x20 tool/mode selector
    i32 winapi_0d95f0_wsprintfA();
};

// @early-stop
// Logic + control-flow byte-exact through the whole switch/mode/QueryLevelName
// chain (all if/else fall-through polarities matched); residual ~11% is a
// register-allocation rotation in the tail SetRect/EngStr_DrawText render block:
// retail threads the RECT/CString/m_c temps through eax/ecx/edx where cl picks
// edx/eax/ecx (a consistent 3-register rotation, identical push order + opcodes,
// only the ModRM reg field differs), plus the basename ptr pushed in-branch vs
// post-merge. Pure allocator coin-flip; no source spelling flips the rotation.
// The 3 `jmpl *table(,%eax,4)` rows are jump-table DIR32 displacement artifacts
// (code bytes identical, reloc-masked). Verified base-vs-target with llvm-objdump -dr.
RVA(0x000d95f0, 0x756)
i32 GruntInfoTextHost::winapi_0d95f0_wsprintfA() {
    CString s0; // tool/mode name  (line 1)
    CString s1; // stage/status    (line 2)
    CString s2; // grunt-type name / level basename (line 3)
    CString s3; // footer          (line 4)

    switch (m_20) {
    case 1:
        s0.LoadString(0x81ae);
        break;
    case 2:
        s0.LoadString(0x81af);
        break;
    case 3:
        s0.LoadString(0x81b0);
        break;
    case 4:
        s0.LoadString(0x81b1);
        break;
    case 5:
        s0.LoadString(0x81b2);
        break;
    case 6:
        s0.LoadString(0x81b3);
        break;
    case 7:
        s0.LoadString(0x81b4);
        break;
    case 8:
        s0.LoadString(0x81b5);
        break;
    default:
        s0 = g_emptyString;
    }

    i32 mode = g_64556c->m_134;
    if (mode == 1) {
        if (g_64556c->m_130 != 0) {
            s1.LoadString(0x81a0);
        } else {
            i32 stage = m_1c;
            if (stage > 0x24) {
                switch (stage) {
                case 0x25:
                    s1.LoadString(0x81a2);
                    break;
                case 0x26:
                    s1.LoadString(0x81a3);
                    break;
                case 0x27:
                    s1.LoadString(0x81a4);
                    break;
                case 0x28:
                    s1.LoadString(0x81a5);
                    break;
                default:
                    s1 = g_emptyString;
                }
            } else {
                s1.Format("Stage %d", ((stage - 1) % 4) + 1);
            }
            switch (m_1c) {
            case 1:
                s2.LoadString(0x8177);
                break;
            case 2:
                s2.LoadString(0x8178);
                break;
            case 3:
                s2.LoadString(0x8179);
                break;
            case 4:
                s2.LoadString(0x817a);
                break;
            case 5:
                s2.LoadString(0x817b);
                break;
            case 6:
                s2.LoadString(0x817c);
                break;
            case 7:
                s2.LoadString(0x817d);
                break;
            case 8:
                s2.LoadString(0x817e);
                break;
            case 9:
                s2.LoadString(0x817f);
                break;
            case 10:
                s2.LoadString(0x8180);
                break;
            case 0xb:
                s2.LoadString(0x8181);
                break;
            case 0xc:
                s2.LoadString(0x8182);
                break;
            case 0xd:
                s2.LoadString(0x8183);
                break;
            case 0xe:
                s2.LoadString(0x8184);
                break;
            case 0xf:
                s2.LoadString(0x8185);
                break;
            case 0x10:
                s2.LoadString(0x8186);
                break;
            case 0x11:
                s2.LoadString(0x8187);
                break;
            case 0x12:
                s2.LoadString(0x8188);
                break;
            case 0x13:
                s2.LoadString(0x8189);
                break;
            case 0x14:
                s2.LoadString(0x818a);
                break;
            case 0x15:
                s2.LoadString(0x818b);
                break;
            case 0x16:
                s2.LoadString(0x818c);
                break;
            case 0x17:
                s2.LoadString(0x818d);
                break;
            case 0x18:
                s2.LoadString(0x818e);
                break;
            case 0x19:
                s2.LoadString(0x818f);
                break;
            case 0x1a:
                s2.LoadString(0x8190);
                break;
            case 0x1b:
                s2.LoadString(0x8191);
                break;
            case 0x1c:
                s2.LoadString(0x8192);
                break;
            case 0x1d:
                s2.LoadString(0x8193);
                break;
            case 0x1e:
                s2.LoadString(0x8194);
                break;
            case 0x1f:
                s2.LoadString(0x8195);
                break;
            case 0x20:
                s2.LoadString(0x8196);
                break;
            default:
                s2.Format(g_emptyString);
                break;
            case 0x25:
                s2.LoadString(0x8197);
                break;
            case 0x26:
                s2.LoadString(0x8198);
                break;
            case 0x27:
                s2.LoadString(0x8199);
                break;
            case 0x28:
                s2.LoadString(0x819a);
            }
            if (g_6455f0 != 0) {
                s1.LoadString(0x81ac);
                s2.LoadString(0x81ad);
            }
        }
    } else if (mode == 3) {
        if (g_64556c->m_130 != 0) {
            s1.LoadString(0x819f);
        } else {
            s1.LoadString(0x819e);
        }
    } else if (mode == 2) {
        if (g_64556c->m_130 != 0) {
            s1.LoadString(0x819d);
        } else {
            s1.LoadString(0x819c);
        }
    } else {
        s0.Format(g_emptyString);
        s2.Format(g_emptyString);
        s1.Format(g_emptyString);
    }

    if (g_64556c->QueryLevelName().GetLength() != 0) {
        char buf[128];
        wsprintfA(buf, g_64556c->QueryLevelName());
        if (strchr(buf, '.')) {
            *strchr(buf, '.') = 0;
        }
        char* base;
        if (strrchr(buf, '\\') != 0) {
            base = strrchr(buf, '\\') + 1;
        } else {
            base = buf;
        }
        s2 = base;
    }

    s3.LoadString(0x819b);

    RECT r1;
    RECT r2;
    RECT r3;
    RECT r4;
    SetRect(&r1, 0, 0, 0x280, 0x38);
    SetRect(&r2, 0, 0x2b, 0x280, 0x59);
    SetRect(&r3, 0, 0x176, 0x280, 0x1a2);
    SetRect(&r4, 0, 0x1b8, 0x280, 0x1e0);
    EngStr_DrawText(m_c, &s0, &r1, 0x78, 0, 0, 0, 0, 1);
    EngStr_DrawText(m_c, &s1, &r2, 0x6e, 0, 0, 0, 0, 1);
    EngStr_DrawText(m_c, &s2, &r3, 0x6e, 0, 0, 0, 0, 1);
    EngStr_DrawText(m_c, &s3, &r4, 0x6e, 0, 0, 0, 0, 1);
    return 1;
}

SIZE_UNKNOWN(GruntInfoTextHost);
SIZE_UNKNOWN(WwdGameRegInfo);
