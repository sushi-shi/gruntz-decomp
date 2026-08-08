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

extern CString g_str645514;
extern CString g_str645518;
extern CString g_str64551c;
extern CString g_str645520;
extern CString g_str64552c;
extern CString g_str645530;

extern char g_msgCaption[];
#endif // SRC_GRUNTZ_GAMETEXT_H
