// BuildGruntzCrcInfo.cpp - CNetSession::BuildGruntzCrcInfo (0xbf1d0). A network
// CRC/sync-diagnostic dump: __thiscall(this) walks the level's 4x15 placed-grunt
// roster and, for each present grunt, formats a
// "[p=%d][g=%d][health=%d]...[rnd=%d]" line (18 fields incl. a per-grunt
// type->weapon-id switch and a random nonce) and appends it to a CString seeded
// with "crc info for all gruntz:\n----...". The assembled report is handed to the
// owning CMulti's ReportVersionMsg.
//
// /GX EH-framed: the local CString (its ~CString must run on the normal + unwind
// exits) gives the body the exception frame, so it lives in an `eh` unit. Only
// offsets / strings / code bytes are load-bearing; the CString ops, wsprintfA,
// the rand nonce and ReportVersionMsg are reloc-masked engine calls.
//
// IDENTITY RECOVERED + FOLD DONE (2026-07-14). Every ex-view is a real class now:
//   * this           IS CNetSession (<Net/NetMgr.h>): the sync-session neighbour
//     of the surrounding netcmdslot cluster; +0x04 is CNetSession::m_session (a
//     CMulti*). PROVEN: the WriteLog receiver `this->m_4` is called with 0xb7e30
//     == CMulti::ReportVersionMsg (via ILT thunk 0x1af0), so this->m_4 IS a CMulti,
//     and CNetSession is the one class whose +0x04 is a CMulti (Init(void*,CMulti*,
//     void*)). RVA 0xbf1d0 also sits inside the netcmdslot RVA band.
//   * CrcSink        IS CMulti (m_session): its WriteLog IS CMulti::ReportVersionMsg.
//   * CrcLevelHolder IS CGruntzMgr (m_session->Mgr(), == CMulti::m_4/CState::m_4);
//     the flat grunt array `@+0x68` is CGruntzMgr::m_cmdGrid (a CTriggerMgr).
//   * the grunt roster IS CTriggerMgr::m_grid[player*15 + g] (the 4x15 placed-cell
//     grid at CTriggerMgr+0x1c; CTmCell == CGrunt), each cell a CGrunt*.
//   * CrcGrunt       IS CGrunt (<Gruntz/Grunt.h>): m_170=m_entranceReason,
//     m_3ec=m_health, m_3f0=m_stamina, m_3f4=m_toyTime, m_218=m_combatActive,
//     m_21c=m_neighborValid, m_220=m_poweredUp, m_450=m_arrivalPhase, m_444=
//     m_entranceCell.reason; m_198/m_19c/m_224/m_358 keep their canonical hex names.
//   * CrcGruntPos    IS CGruntHud (CGrunt::m_10; x @+0x5c / y @+0x60).
// BuildGruntzCrcInfo has NO rel32 caller (a debug/CRC-sync dump reached only via the
// thunk band), so the owner was not caller-recoverable - it was recovered from the
// +0x04 CMulti receiver and the RVA neighbourhood instead.
#include <Mfc.h> // real MFC CString + <windows.h> wsprintfA (afx-first)
#include <rva.h>
#include <stdlib.h> // rand (0x11fee0), the per-grunt random nonce

#include <Net/NetMgr.h>       // CNetSession (this; m_session)
#include <Gruntz/Multi.h>     // CMulti (ReportVersionMsg, Mgr())
#include <Gruntz/GruntzMgr.h> // CGruntzMgr (m_cmdGrid)
#include <Gruntz/TriggerMgr.h> // CTriggerMgr (m_grid; CTmCell == CGrunt)
#include <Gruntz/Grunt.h>     // CGrunt + CGruntHud (the dumped fields)

extern "C" char g_emptyString[]; // 0x6293f4 (== "")

// @source: decomp-xref
// @early-stop
// /GX EH-state megafunction wall: the body is reconstructed in full (the seeded
// CString, the flat grunt-array double walk, the 0x16-case type->weapon switch,
// the 18-field wsprintf line + two appends, the ReportVersionMsg handoff and the
// destructor). The byte residual is the documented /GX exception-state numbering +
// jump-table base reloc typing around the CString temp (cf. the sibling
// megafunctions RollingBall/TerrainTileLoader): not source-steerable. See
// docs/patterns/big-seh-fuzzy-desync.md + eh-state-numbering-base.md.
RVA(0x000bf1d0, 0x249)
void CNetSession::BuildGruntzCrcInfo() {
    char szLine[0x100];
    szLine[0] = g_emptyString[0];
    memset(szLine + 1, 0, sizeof(szLine) - 1);

    CString info("crc info for all gruntz:\n------------------------\n");

    for (i32 player = 0; player < 4; player++) {
        for (i32 g = 0; g < 0xf; g++) {
            // the 4x15 placed-grunt cell grid on the world command manager
            CGrunt* grunt = m_session->Mgr()->m_cmdGrid->m_grid[player * 0xf + g];
            if (grunt == 0) {
                continue;
            }
            i32 rnd = rand();
            i32 type = grunt->m_entranceReason;
            i32 wp;
            switch (type) {
                case 1:
                    wp = 2;
                    break;
                case 2:
                    wp = 9;
                    break;
                case 3:
                    wp = 0xe;
                    break;
                case 4:
                    wp = 6;
                    break;
                case 5:
                    wp = 0xb;
                    break;
                case 6:
                    wp = 0x13;
                    break;
                case 7:
                    wp = 0x11;
                    break;
                case 8:
                    wp = 0xf;
                    break;
                case 9:
                    wp = 5;
                    break;
                case 10:
                    wp = 0x15;
                    break;
                case 0xb:
                    wp = 7;
                    break;
                case 0xc:
                    wp = 0x10;
                    break;
                case 0xd:
                    wp = 8;
                    break;
                case 0xe:
                    wp = 0xa;
                    break;
                case 0xf:
                    wp = 0xd;
                    break;
                case 0x10:
                    wp = 4;
                    break;
                case 0x11:
                    wp = 0x14;
                    break;
                case 0x12:
                    wp = 0x12;
                    break;
                case 0x13:
                    wp = 0x16;
                    break;
                case 0x15:
                    wp = 3;
                    break;
                case 0x16:
                    wp = 0xc;
                    break;
                default:
                    wp = 0x17;
                    break;
            }
            i32 tool = type;
            if (type > 0x16) {
                tool = grunt->m_19c;
            }
            wsprintfA(
                szLine,
                "[p=%d][g=%d][health=%d][x=%d][y=%d][dir=%d][stm=%d][ttl=%d][tool=%d]"
                "[toy=%d][da=%d][wp=%d][iic=%d][qat=%d][qax=%d][ia=%d][iad=%d][rnd=%d]\n",
                player,
                g,
                grunt->m_health,
                grunt->m_10->m_5c,
                grunt->m_10->m_60,
                grunt->m_entranceCell.reason,
                grunt->m_stamina,
                grunt->m_toyTime,
                tool,
                grunt->m_198,
                grunt->m_224,
                wp,
                grunt->m_poweredUp,
                grunt->m_neighborValid,
                grunt->m_arrivalPhase,
                grunt->m_combatActive,
                grunt->m_358,
                rnd
            );
            info += "\n";
            info += szLine;
        }
    }
    m_session->ReportVersionMsg((char*)(const char*)info, 0);
}
