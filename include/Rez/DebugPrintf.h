// DebugPrintf.h
#ifndef GRUNTZ_REZ_DEBUGPRINTF_H_H
#define GRUNTZ_REZ_DEBUGPRINTF_H_H

#include <Ints.h>

// --- C-linkage carriers for the TU's extern-C definitions (the defs
// inherit the linkage from these decls; the .cpp wrappers are gone) ---
extern "C" int vsprintf(char* buf, const char* fmt, char* va);
extern "C" void DebugSink_184df0(char* line);
// The variadic debug-print family is extern "C" in retail (callers use the
// C-linkage `_Rez*` name). The defs inherit C linkage from these decls, so cl
// emits `_RezDebugPrintfCh` etc. directly - no SYMBOL(_Rez..) override (that only
// renamed the delink TARGET, leaving the base obj C++-mangled -> objdiff could
// not pair them and scored the byte-exact bodies 0%).
extern "C" void RezAssertFail(char* fmt, ...);
extern "C" void RezDebugPrintfXY(i32 x, i32 y, char* fmt, ...);
extern "C" void RezDebugPrintfCh(i32 channel, char* fmt, ...);
extern "C" void RezDebugPrintfChXY(i32 channel, i32 x, i32 y, char* fmt, ...);

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
extern "C" i32 fclose(void* fp); // 0x11f780 (CRT fclose, library row _fclose)

extern i32 g_debugPrintMode;
#endif // GRUNTZ_REZ_DEBUGPRINTF_H_H
