#ifndef GRUNTZ_GRUNTZ_SAVESLOTCTRLID_H
#define GRUNTZ_GRUNTZ_SAVESLOTCTRLID_H

#include <Enums.h>

// The load-game dialog's per-save-slot buttons: three contiguous banks of ten,
// laid out 0x490 + 0x0a * bank + slot.
//
// Both halves are proved by the code, not inferred. The SLOT is the `idx = N`
// each arm assigns; the BANK is the action the switch runs afterwards:
//
//   0x490..0x499  VerifySlot(slot), then load it            -> LOAD
//   0x49a..0x4a3  RunModalDialog("GAME_INFO", ...)          -> INFO
//   0x4a4..0x4ad  RunModalDialog("GAME_DELETE", ...)        -> DELETE
//
// A control-id bag, never a variable's type: the carriers are MFC's `int nID`
// (WM_COMMAND's wParam, GetDlgItem), same as DialogCtrlId in <Gruntz/Dialogs.h>,
// which holds the other half of this 0x4xx-0x5xx dialog space.
GZ_ENUM_CONST_BEGIN(SaveSlotCtrlId)
    CTRL_SAVESLOT_NAME = 0x40d,
    // The SAVE dialog's own per-slot buttons. CSaveGame translates each one to the
    // load dialog's slot control of the same index (0x435 + n -> 0x490 + n), which
    // is what pins the band: the mapping is linear across all ten and the target is
    // already CTRL_SAVESLOT_LOAD0..9.
    CTRL_SAVEDLG_SLOT0 = 0x435,
    CTRL_SAVEDLG_SLOT1 = 0x436,
    CTRL_SAVEDLG_SLOT2 = 0x437,
    CTRL_SAVEDLG_SLOT3 = 0x438,
    CTRL_SAVEDLG_SLOT4 = 0x439,
    CTRL_SAVEDLG_SLOT5 = 0x43a,
    CTRL_SAVEDLG_SLOT6 = 0x43b,
    CTRL_SAVEDLG_SLOT7 = 0x43c,
    CTRL_SAVEDLG_SLOT8 = 0x43d,
    CTRL_SAVEDLG_SLOT9 = 0x43e,

    CTRL_SAVESLOT_LOAD0 = 0x490,
    CTRL_SAVESLOT_LOAD1 = 0x491,
    CTRL_SAVESLOT_LOAD2 = 0x492,
    CTRL_SAVESLOT_LOAD3 = 0x493,
    CTRL_SAVESLOT_LOAD4 = 0x494,
    CTRL_SAVESLOT_LOAD5 = 0x495,
    CTRL_SAVESLOT_LOAD6 = 0x496,
    CTRL_SAVESLOT_LOAD7 = 0x497,
    CTRL_SAVESLOT_LOAD8 = 0x498,
    CTRL_SAVESLOT_LOAD9 = 0x499,

    CTRL_SAVESLOT_INFO0 = 0x49a,
    CTRL_SAVESLOT_INFO1 = 0x49b,
    CTRL_SAVESLOT_INFO2 = 0x49c,
    CTRL_SAVESLOT_INFO3 = 0x49d,
    CTRL_SAVESLOT_INFO4 = 0x49e,
    CTRL_SAVESLOT_INFO5 = 0x49f,
    CTRL_SAVESLOT_INFO6 = 0x4a0,
    CTRL_SAVESLOT_INFO7 = 0x4a1,
    CTRL_SAVESLOT_INFO8 = 0x4a2,
    CTRL_SAVESLOT_INFO9 = 0x4a3,

    CTRL_SAVESLOT_DELETE0 = 0x4a4,
    CTRL_SAVESLOT_DELETE1 = 0x4a5,
    CTRL_SAVESLOT_DELETE2 = 0x4a6,
    CTRL_SAVESLOT_DELETE3 = 0x4a7,
    CTRL_SAVESLOT_DELETE4 = 0x4a8,
    CTRL_SAVESLOT_DELETE5 = 0x4a9,
    CTRL_SAVESLOT_DELETE6 = 0x4aa,
    CTRL_SAVESLOT_DELETE7 = 0x4ab,
    CTRL_SAVESLOT_DELETE8 = 0x4ac,
    CTRL_SAVESLOT_DELETE9 = 0x4ad,

    CTRL_SAVESLOT_PREVIEW_TITLE = 0x4b3,
    CTRL_SAVESLOT_PREVIEW_IMAGE = 0x51d
GZ_ENUM_CONST_END(SaveSlotCtrlId)

#endif // GRUNTZ_GRUNTZ_SAVESLOTCTRLID_H
