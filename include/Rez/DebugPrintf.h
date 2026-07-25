// DebugPrintf.h
#ifndef GRUNTZ_REZ_DEBUGPRINTF_H_H
#define GRUNTZ_REZ_DEBUGPRINTF_H_H

#include <Ints.h>
#include <stdio.h> // fclose comes from the real CRT header, not a hand-rolled decl

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

extern i32 g_debugPrintMode;

// File-scope prototypes moved from the .cpp: an unqualified
// declaration at file scope has EXTERNAL linkage, so it belongs in
// the owner header.
void DebugSetCursorXY(i32 x, i32 y);


// File-scope prototypes moved from the .cpp (external linkage
// belongs in the owner header).
void DebugSetCursor(i32, i32, i32); // 0x184fd0
int vsprintf(char* buf, const char* fmt, char* va); // 0x121770 (CRT)
void DebugSink_184df0(char* line);                  // 0x184df0 (1-byte sink)

#endif // GRUNTZ_REZ_DEBUGPRINTF_H_H
