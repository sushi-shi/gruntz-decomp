#include <rva.h>

#include <Mfc.h>

#include <DDrawMgr/DDrawChildGroup.h>
#include <Gruntz/GameMode.h>
#include <Gruntz/GameRegistry.h>
#include <Gruntz/GameRegMfcPtr.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/SpriteRefTable.h>
#include <Gruntz/UserLogic.h>
#include <Ints.h>

// @early-stop
RVA(0x00019920, 0x1f0)
i32 CBootyState::BuildGruntSprintAnimation() {
    CShadeTable* h = g_gameReg->m_spriteFactory->GetSel(0, 0);
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
