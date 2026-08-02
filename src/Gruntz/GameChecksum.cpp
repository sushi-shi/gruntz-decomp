#include <rva.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/GruntzMgr.h>
#include <Gruntz/Multi.h>
#include <Gruntz/TriggerMgr.h>
#include <Ints.h>
#include <Net/NetMgr.h>

#include <stdlib.h>

// @early-stop
RVA(0x000c0590, 0x21c)
i32 CNetSession::Checksum() {
    i32 sum = 0;
    i32 idx = 0;
    do {
        i32 cnt = 15;
        do {

            CGrunt* obj = static_cast<CGrunt*>(m_session->m_mgr->m_cmdGrid->m_grid[idx]);
            if (obj != 0) {
                CGameObject* sub = obj->m_object;
                sum += obj->m_entranceCell.direction + obj->m_stamina + obj->m_toyTime
                       + obj->m_health + sub->m_screenY + sub->m_sortKey + sub->m_screenX
                       + obj->m_lastTilePx.m_x + obj->m_lastTilePx.m_y;
                i32 n = obj->m_entranceReason;
                i32 d = n;
                if (n > 0x16) {
                    d = obj->m_toolId;
                }
                sum += obj->m_vehiclePickupType + obj->m_entranceCommitted + obj->m_entranceActive
                       + obj->m_daFlag + d;
                i32 v = obj->m_entranceReason - 1;
                i32 r;
                if (static_cast<u32>(v) > 0x15) {
                    r = 0x17;
                } else {
                    switch (v) {
                        case 0:
                            r = 2;
                            break;
                        case 1:
                            r = 3;
                            break;
                        case 2:
                            r = 4;
                            break;
                        case 3:
                            r = 5;
                            break;
                        case 4:
                            r = 6;
                            break;
                        case 5:
                            r = 7;
                            break;
                        case 6:
                            r = 8;
                            break;
                        case 7:
                            r = 9;
                            break;
                        case 8:
                            r = 0xa;
                            break;
                        case 9:
                            r = 0xb;
                            break;
                        case 0xa:
                            r = 0xc;
                            break;
                        case 0xb:
                            r = 0xd;
                            break;
                        case 0xc:
                            r = 0xe;
                            break;
                        case 0xd:
                            r = 0xf;
                            break;
                        case 0xe:
                            r = 0x10;
                            break;
                        case 0xf:
                            r = 0x11;
                            break;
                        case 0x10:
                            r = 0x12;
                            break;
                        case 0x11:
                            r = 0x13;
                            break;
                        case 0x12:
                            r = 0x14;
                            break;
                        case 0x13:
                            r = 0x15;
                            break;
                        case 0x14:
                            r = 0x16;
                            break;
                        case 0x15:
                            r = 0x17;
                            break;
                        default:
                            r = 0x17;
                            break;
                    }
                }
                sum += obj->m_arrivalPhase + obj->m_neighborScanEnabled + obj->m_combatActive
                       + obj->m_neighborValid + obj->m_poweredUp + g_frameTime + r;
                sum += rand();
            }
            idx++;
        } while (--cnt);
    } while (idx < 0x3c);
    return sum;
}
