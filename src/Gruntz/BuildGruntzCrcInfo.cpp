#include <rva.h>

#include <Mfc.h>

#include <EmptyString.h>
#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/PickupType.h>
#include <Gruntz/TriggerMgr.h>
#include <Net/NetMgr.h>

#include <stdlib.h>

// @early-stop
RVA(0x000bf1d0, 0x2a4)
void CNetSession::BuildGruntzCrcInfo() {
    char szLine[0x100];
    szLine[0] = g_emptyString[0];
    memset(szLine + 1, 0, sizeof(szLine) - 1);

    CString info("crc info for all gruntz:\n------------------------\n");

    for (i32 player = 0; player < 4; player++) {
        for (i32 g = 0; g < 0xf; g++) {

            CGrunt* grunt = m_session->Mgr()->m_cmdGrid->m_grid[player * 0xf + g];
            if (grunt == NULL) {
                continue;
            }
            i32 rnd = rand();
            PickupType type = grunt->m_entranceReason;
            i32 wp;
            switch (type) {
                case PICKUP_BOMB:
                    wp = 2;
                    break;
                case PICKUP_BOOMERANG:
                    wp = 9;
                    break;
                case PICKUP_BRICK:
                    wp = 0xe;
                    break;
                case PICKUP_CLUB:
                    wp = 6;
                    break;
                case PICKUP_GAUNTLETZ:
                    wp = 0xb;
                    break;
                case PICKUP_GLOVEZ:
                    wp = 0x13;
                    break;
                case PICKUP_GOOBER:
                    wp = 0x11;
                    break;
                case PICKUP_GRAVITYBOOTZ:
                    wp = 0xf;
                    break;
                case PICKUP_GUNHAT:
                    wp = 5;
                    break;
                case PICKUP_NERFGUN:
                    wp = 0x15;
                    break;
                case PICKUP_ROCK:
                    wp = 7;
                    break;
                case PICKUP_SHIELD:
                    wp = 0x10;
                    break;
                case PICKUP_SHOVEL:
                    wp = 8;
                    break;
                case PICKUP_SPRING:
                    wp = 0xa;
                    break;
                case PICKUP_SPY:
                    wp = 0xd;
                    break;
                case PICKUP_SWORD:
                    wp = 4;
                    break;
                case PICKUP_TIMEBOMB:
                    wp = 0x14;
                    break;
                case PICKUP_TOOB:
                    wp = 0x12;
                    break;
                case PICKUP_WAND:
                    wp = 0x16;
                    break;
                case PICKUP_WELDER:
                    wp = 3;
                    break;
                case PICKUP_WINGZ:
                    wp = 0xc;
                    break;
                default:
                    wp = 0x17;
                    break;
            }
            PickupType tool = type;
            if (type > PICKUP_EQUIPPABLE_LAST) {
                tool = grunt->m_toolId;
            }
            wsprintfA(
                szLine,
                "[p=%d][g=%d][health=%d][x=%d][y=%d][dir=%d][stm=%d][ttl=%d][tool=%d]"
                "[toy=%d][da=%d][wp=%d][iic=%d][qat=%d][qax=%d][ia=%d][iad=%d][rnd=%d]\n",
                player,
                g,
                grunt->m_health,
                grunt->m_object->m_screenX,
                grunt->m_object->m_screenY,
                grunt->m_entranceCell.direction,
                grunt->m_stamina,
                grunt->m_toyTime,
                tool,
                grunt->m_vehiclePickupType,
                grunt->m_daFlag,
                wp,
                grunt->m_poweredUp,
                grunt->m_neighborValid,
                grunt->m_arrivalPhase,
                grunt->m_combatActive,
                grunt->m_neighborScanEnabled,
                rnd
            );
            info += "\n";
            info += szLine;
        }
    }
    m_session->ReportVersionMsg(const_cast<char*>(static_cast<const char*>(info)), 0);
}
