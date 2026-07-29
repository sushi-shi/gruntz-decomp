#ifndef SRC_GRUNTZ_GAMETEXT_H
#define SRC_GRUNTZ_GAMETEXT_H

#include <rva.h>
#include <Gruntz/String.h>

#include <Wap32/zBitVec.h>

extern CString g_brickText1; // 0x00245524
// The other six slots of the same debug-text band (0x245514..0x245530). They were
// DEFINED in GameText.cpp with nothing declaring them, so no consumer could reach them;
// CPlay::DrawDebugStatsFull @0xcf0a0 TextOuts all eight down the left edge under
// g_debugDisplayFlags & 0x8, in this y order: brickText1, brickText2, 64552c, 645530,
// 645514, 645518, 64551c, 645520 (16 px apart). Their roles are still unrecovered - the
// only reader is that dev overlay - so the hex names stay until something names them.
extern CString g_str645514; // 0x00245514
extern CString g_str645518; // 0x00245518
extern CString g_str64551c; // 0x0024551c
extern CString g_str645520; // 0x00245520
extern CString g_str64552c; // 0x0024552c
extern CString g_str645530; // 0x00245530

extern char g_msgCaption[];
#endif // SRC_GRUNTZ_GAMETEXT_H
