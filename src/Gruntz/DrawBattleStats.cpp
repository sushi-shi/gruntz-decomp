// DrawBattleStats.cpp - the in-game BATTLE-STATZ scoreboard renderer (0x1ed30),
// a __thiscall HUD method (sibling of DrawDebugStats). Recovered from the $SG
// string set ("Fortz:"/"Killz:"/"Gruntz:"/"Toolz:"/"Toyz:"/"Powerupz:"/"Cursez:"/
// "BATTLE STATZ", "%d", "%s") + the draw call set.
//
// Structure (dump_target + decomp):
//   - Loop 1 (4 players): for each active player draw 6 numeric columns, each the
//     sum of a run of per-player stat dwords off g_mgr->m_7c at a column-specific
//     base/stride (col1 sum 4 @+0x348 str 0x10, col2 sum 7 @+0x2d8 str 0x1c,
//     col3 sum 10 @+0x238 str 0x28, col4 sum 22 @+0xd8 str 0x58, col5 one @+0x48
//     str 4, col6 = SumWinRow(player)); each column EngFmt's "%d" into the reused
//     CString, CopyRect's the column's source rect, and draws it.
//   - Loop 2 (7 categories): assign the row label then CopyRect+draw it.
//   - Colour loop (4 players): for each active player pick the team colour from a
//     17-entry index table, build the colour NAME string, and draw it in colour.
//   - Title: EngFmt "BATTLE STATZ" into a fixed rect and draw it.
//
// The reused CString (+ the colour-name CString temp) give it the /GX exception
// frame, so it lives in its own `eh` unit. CARCASS doctrine: g_mgr + the view are
// engine classes reached by raw this+offset; every callee is a reloc-masked
// external; the strings are $SG literals reloc-masked against the matched symbols;
// CopyRect is hoisted through a data function-pointer global (0x6c44bc).
#include <Mfc.h> // MFC CString (ctor 0x1b9b93 / dtor 0x1b9cde / op=(LPCTSTR) 0x1b9e74)
#include <Gruntz/CGameRegistry.h>
#include <Win32.h> // RECT

#include <rva.h>

// EngFmt (0x1b2cf5): __cdecl variadic sprintf-into-CString.
void EngFmt(CString* out, const char* fmt, ...);
// DrawStatText (0x1f00 -> 0x1154b0): __cdecl(ctx, text, rect, y, flag, b, g, r, a9).
extern "C" void
DrawStatText(void* ctx, CString* text, RECT* rc, i32 y, i32 flag, i32 b, i32 g, i32 r, i32 a9);
// GetColorName (0x3e54): NRV CString* into `out`.
CString* GetColorName(CString* out);

// CopyRect USER32 import hoisted through a data fn-ptr global (retail loads it once
// into ebp and calls it ~13x).
DATA(0x002c44bc)
extern void(__stdcall* g_pCopyRect)(RECT* dst, const RECT* src); // 0x6c44bc

// The per-column source-rect tables (RECT[] in .data). Indexed by player/category.
DATA(0x001e9178)
extern RECT g_col1Rects[]; // 0x5e9178
DATA(0x001e91b8)
extern RECT g_col2Rects[]; // 0x5e91b8
DATA(0x001e91f8)
extern RECT g_col3Rects[]; // 0x5e91f8
DATA(0x001e9238)
extern RECT g_col4Rects[]; // 0x5e9238
DATA(0x001e9278)
extern RECT g_col5Rects[]; // 0x5e9278
DATA(0x001e92b8)
extern RECT g_col6Rects[]; // 0x5e92b8
DATA(0x001e92f8)
extern RECT g_colorRects[]; // 0x5e92f8 (4 team-colour rects)
DATA(0x001e9338)
extern RECT g_labelRects[]; // 0x5e9338 (7 category-label rects)

// The per-player stat block reached through g_mgr->m_7c; SumWinRow (0x1230) folds
// the win-row totals for a player.
struct StatArray {
    i32 SumWinRow(i32 player); // 0x1230
};

DATA(0x0024556c)
extern CGameRegistry* g_mgr; // *0x64556c

class CBattleStatsView {
public:
    void DrawBattleStats(); // 0x1ed30

    char m_pad00[0xc];
    void* m_c; // +0x0c  draw context
};

static __inline i32 sumRun(StatArray* base, i32 off, i32 n) {
    i32* p = (i32*)((char*)base + off);
    i32 s = 0;
    i32 k;
    for (k = 0; k < n; k++) {
        s += p[k];
    }
    return s;
}

// @source: string-xref
// @early-stop
// induction-variable strength-reduction wall (~85%): the whole structure, all
// externs/strings, the /GX frame size, the register roles (this=edi, CopyRect=ebp,
// g_mgr re-read per use, the inlined sum loops, loops 2/3 + title) match retail.
// The residual is the 6-column player loop: retail strength-reduces the per-column
// offsets into a specific induction-variable/register layout (col2 offset pinned in
// ebx, col1+rect index coalesced into esi, cols 3/4/5 in spilled accumulators);
// the /O2 recompile derives an equivalent-but-differently-registered set, which
// cascades a scheduling/operand-byte drift through loop 1 (verified llvm-objdump -dr:
// identical opcodes/logic, differing scratch regs edx/ecx/eax + IV register choice).
// Documented regalloc wall, not source-steerable (cf. docs/patterns/
// loop-invariant-multiply-strength-reduce-vs-memreread.md).
RVA(0x0001ed30, 0x549)
void CBattleStatsView::DrawBattleStats() {
    CString s;
    RECT rc;
    void(__stdcall * copyRect)(RECT*, const RECT*) = g_pCopyRect;
    i32 i;
    i32 c;

    // Loop 1: 6 numeric stat columns per active player.
    for (i = 0; i < 4; i++) {
        if (*(i32*)((char*)g_mgr + 0x178 + i * 0x238) != 0) {
            EngFmt(&s, "%d", sumRun((StatArray*)g_mgr->m_7c, 0x348 + i * 0x10, 4));
            copyRect(&rc, &g_col1Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            EngFmt(&s, "%d", sumRun((StatArray*)g_mgr->m_7c, 0x2d8 + i * 0x1c, 7));
            copyRect(&rc, &g_col2Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            EngFmt(&s, "%d", sumRun((StatArray*)g_mgr->m_7c, 0x238 + i * 0x28, 10));
            copyRect(&rc, &g_col3Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            EngFmt(&s, "%d", sumRun((StatArray*)g_mgr->m_7c, 0xd8 + i * 0x58, 22));
            copyRect(&rc, &g_col4Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            EngFmt(&s, "%d", *(i32*)((char*)(StatArray*)g_mgr->m_7c + 0x48 + i * 4));
            copyRect(&rc, &g_col5Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);

            EngFmt(&s, "%d", ((StatArray*)g_mgr->m_7c)->SumWinRow(i));
            copyRect(&rc, &g_col6Rects[i]);
            DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);
        }
    }

    // Loop 2: category labels.
    for (c = 0; c <= 6; c++) {
        switch (c) {
            case 0:
                s = "Fortz:";
                break;
            case 1:
                s = "Killz:";
                break;
            case 2:
                s = "Gruntz:";
                break;
            case 3:
                s = "Toolz:";
                break;
            case 4:
                s = "Toyz:";
                break;
            case 5:
                s = "Powerupz:";
                break;
            case 6:
                s = "Cursez:";
                break;
        }
        copyRect(&rc, &g_labelRects[c]);
        DrawStatText(m_c, &s, &rc, 0x78, 1, 0xff, 0xff, 0, 1);
    }

    // Colour loop: team-colour name per active player, drawn in that colour.
    for (i = 0; i < 4; i++) {
        if (*(i32*)((char*)g_mgr + 0x178 + i * 0x238) != 0) {
            i32 color;
            switch (*(i32*)((char*)g_mgr + 0x158 + i * 0x238)) {
                case 0:
                    color = 0x80ff;
                    break;
                case 1:
                    color = 0xff00;
                    break;
                case 2:
                    color = 0xff0000;
                    break;
                case 3:
                    color = 0xff;
                    break;
                case 4:
                    color = 0x800080;
                    break;
                case 5:
                    color = 0xffff;
                    break;
                case 6:
                    color = 0x8000ff;
                    break;
                case 8:
                    color = 0x800000;
                    break;
                case 9:
                    color = 0x8000;
                    break;
                case 10:
                    color = 0x808000;
                    break;
                case 11:
                    color = 0x80;
                    break;
                case 12:
                    color = 0xff00ff;
                    break;
                case 13:
                    color = 0x8080;
                    break;
                case 14:
                    color = 0x808080;
                    break;
                case 15:
                    color = 0xffff00;
                    break;
                case 16:
                    color = 0xffffff;
                    break;
                default:
                    color = 0;
                    break;
            }
            CString cn;
            EngFmt(&s, "%s", (const char*)*GetColorName(&cn));
            copyRect(&rc, &g_colorRects[i]);
            DrawStatText(
                m_c,
                &s,
                &rc,
                0x64,
                0,
                color & 0xff,
                (color >> 8) & 0xff,
                (color >> 0x10) & 0xff,
                1
            );
        }
    }

    // Title.
    EngFmt(&s, "BATTLE STATZ");
    rc.left = 0x96;
    rc.top = 0xf;
    rc.right = 0x280;
    rc.bottom = 0x73;
    DrawStatText(m_c, &s, &rc, 0x82, 1, 0xff, 0xff, 0, 1);
}

SIZE_UNKNOWN(StatArray);
SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CBattleStatsView);
