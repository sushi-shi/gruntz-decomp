// DebugPrintf.h
#ifndef GRUNTZ_REZ_DEBUGPRINTF_H_H
#define GRUNTZ_REZ_DEBUGPRINTF_H_H

#include <Ints.h>


// --- C-linkage carriers for the TU's extern-C definitions (the defs
// inherit the linkage from these decls; the .cpp wrappers are gone) ---
extern "C" int vsprintf(char* buf, const char* fmt, char* va);
extern "C" void DebugSink_184df0(char* line);

#endif // GRUNTZ_REZ_DEBUGPRINTF_H_H
