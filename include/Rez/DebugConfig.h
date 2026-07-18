// DebugConfig.h - THE canonical shape of CDebugConfig, the debug-output config
// singleton (VA 0x6bf848). InitFromEnv reads %DPRINTF%, upper-cases it, maps the
// first matching device keyword to the mode word (+0x94), and hands the parsed
// string to the embedded debug-channel set (+0x08 CRangeSet). Defined in
// src/Rez/DebugPrintf.cpp (owner TU).
//
// Field names are placeholders; only offsets + code bytes are load-bearing.
#ifndef GRUNTZ_REZ_DEBUGCONFIG_H
#define GRUNTZ_REZ_DEBUGCONFIG_H

#include <rva.h>

class CDebugConfig {
public:
    CDebugConfig* InitFromEnv(); // 0x185000
};
SIZE_UNKNOWN(CDebugConfig);

#endif // GRUNTZ_REZ_DEBUGCONFIG_H
