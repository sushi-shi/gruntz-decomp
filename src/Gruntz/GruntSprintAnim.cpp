#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/GameRegMfcPtr.h> // g_gameReg at its REAL type (CGruntzMgr)
#include <Gruntz/GruntzMgr.h>
#include <Ints.h>
#include <Gruntz/GameRegistry.h>
#include <DDrawMgr/DDrawChildGroup.h> // the ONE CDDrawChildGroup (CreateSprite @0x1597b0)
#include <Gruntz/UserLogic.h>         // CGameObject (the created sprite)
#include <Mfc.h>                      // CString (the /GX directional-name temps) + Win32
#include <Gruntz/GameMode.h>          // the REAL owner: CBootyState (was the CGruntSprintAnim view)

#include <rva.h>

// ===========================================================================
// BuildGruntSprintAnimation @0x019920
// ===========================================================================
// @early-stop
// regalloc/scheduling wall (~80%): complete + correct, verified instruction-by-
// instruction vs retail. The prologue, /GX exception frame (sub esp,0x18; the EH
// state thread at [esp+0x30]/0x34/0x3c), the g_gameReg->m_spriteFactory lookup, the whole
// CString jump-table build ("GRUNTZ_NORMALGRUNT_" + dir + "_WALK"), the two
// operator+ temps, both CacheFirstFrame/ApplyLookupGeometry receivers, and the
// this=ebp / array=esi induction all match byte-for-byte (modulo reloc names).
// Residual: in the {m_58,m_50,m_4c} field-store block retail caches the sprite
// pointer in eax across the three stores (one `mov eax,[esi]`), while this /O2
// recompile re-reads m_204[i-1] (=[esi]) per store because the store to
// spr->m_58 may alias the array slot - MSVC's reload-vs-cache choice here is a
// register-pressure artifact (all four callee-saved regs are pinned to
// this/array/const-1/counter, so `spr` is memory-homed; homing it in a `spr`
// local instead flips array<->spr and regresses to ~73%). Plus the inline
// jump-table base (jmpl *0x1c4(,%eax,4) vs our $L DIR32 reloc) and the loop-bound
// strength-reduction form ((i-1)<8 vs i<=8) are documented scoring/scheduling
// walls. See docs/patterns/pin-local-for-callee-saved-reg.md +
// jumptable-data-overlap.md + zero-register-pinning.md.
RVA(0x00019920, 0x1c2)
i32 CBootyState::BuildGruntSprintAnimation() {
    i32 h = g_gameReg->m_spriteFactory->GetSel(0, 0);
    if (!h) {
        return 0;
    }

    for (i32 i = 1; i <= 8; i++) {
        m_sprintSprites[i - 1] =
            g_gameReg->m_world->m_childGroup->CreateSprite(0, 0, 0, 2, "SimpleAnimation", 3);
        if (m_sprintSprites[i - 1] == 0) {
            return 0;
        }

        CString dir;
        switch (i - 1) {
            case 0:
                dir = "NORTH";
                break;
            case 1:
                dir = "NORTHEAST";
                break;
            case 2:
                dir = "EAST";
                break;
            case 3:
                dir = "SOUTHEAST";
                break;
            case 4:
                dir = "SOUTH";
                break;
            case 5:
                dir = "SOUTHWEST";
                break;
            case 6:
                dir = "WEST";
                break;
            case 7:
                dir = "NORTHWEST";
                break;
        }

        m_sprintSprites[i - 1]->ApplyName("GRUNTZ_NORMALGRUNT_" + dir + "_WALK");
        m_sprintSprites[i - 1]->ApplyLookupGeometry("GAME_GRUNTSPRINT", 0);
        m_sprintSprites[i - 1]->m_drawActive = 1;
        m_sprintSprites[i - 1]->m_drawFillCmd = 0xa;
        m_sprintSprites[i - 1]->m_drawFillArg = h;

        i32 outX, outY;
        GenMenuRandPos(i, &outX, &outY);
        m_sprintSprites[i - 1]->m_screenX = outX;
        m_sprintSprites[i - 1]->m_screenY = outY;
    }
    return 1;
}

SIZE_UNKNOWN(CGameRegistry);
SIZE_UNKNOWN(CGsSoundTable);
