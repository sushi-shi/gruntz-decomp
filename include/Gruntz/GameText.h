#ifndef SRC_GRUNTZ_GAMETEXT_H
#define SRC_GRUNTZ_GAMETEXT_H

#include <rva.h>
#include <Gruntz/String.h>

#include <Wap32/zBitVec.h>

extern CString g_brickText1; // 0x00245524


// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
#include <Gruntz/ActReg.h> // CActReg (for the extern below)
extern CActReg g_actRegCaption;

extern char g_msgCaption[];
#endif // SRC_GRUNTZ_GAMETEXT_H
