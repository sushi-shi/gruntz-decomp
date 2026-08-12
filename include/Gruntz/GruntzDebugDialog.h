#ifndef GRUNTZ_GRUNTZ_GRUNTZDEBUGDIALOG_H
#define GRUNTZ_GRUNTZ_GRUNTZDEBUGDIALOG_H

#include <Mfc.h>

#include <Ints.h>

i32 CALLBACK DebugGruntTypeDialogProc(HWND, UINT, WPARAM, LPARAM);

extern i32 g_debugGruntPlayer, g_debugGruntTool, g_debugGruntToy;
extern i32 g_debugGruntAiType, g_debugGruntColumn, g_debugGruntRow;
extern i32 g_debugGruntColor, g_debugGruntRadius;
extern i32 g_debugGruntMoveLeft, g_debugGruntMoveRight;
extern i32 g_debugGruntMoveTop, g_debugGruntMoveBottom;

#endif // GRUNTZ_GRUNTZ_GRUNTZDEBUGDIALOG_H
