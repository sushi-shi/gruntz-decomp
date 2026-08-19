#ifndef SRC_GRUNTZ_GAMETEXT_H
#define SRC_GRUNTZ_GAMETEXT_H

#include <rva.h>

#include <Gruntz/String.h>
#include <Wap32/zBitVec.h>

// The eight Questz area names, indexed (levelId - 1) / 4.
extern CString g_areaNames[8];

class CWinApp;
extern CWinApp g_gruntzWinApp;

extern CString g_brickText1;
extern CString g_brickText2;
extern CString g_brickText3;
extern CString g_brickText4;
extern CString g_brickText5;
extern CString g_brickText6;
extern CString g_brickText7;
extern CString g_brickText8;

extern char g_msgCaption[];
#endif // SRC_GRUNTZ_GAMETEXT_H
