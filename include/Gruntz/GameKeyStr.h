// GameKeyStr.h - the global key-string builder (a REAL recovered type,
// ?g_levelStr@@3UGameKeyStr@@A / ?g_pathStr...). The engine keeps two file-scope
// instances (g_pathStr @0x62c25c, g_levelStr @0x62c260) that the custom-world and
// powerup-icon-key paths build "<dir>\...\<name>.WWD"-style keys through. Formerly
// a per-TU view in CustomWorldInfoDlg.cpp (Set/Append/Reset) + BoundaryLowerThunks.cpp
// (Free1b9b93); folded here (wave 3). All methods are reloc-masked externals.
#ifndef SRC_GRUNTZ_GAMEKEYSTR_H
#define SRC_GRUNTZ_GAMEKEYSTR_H

#include <Ints.h>
#include <rva.h>

SIZE_UNKNOWN(GameKeyStr);
struct GameKeyStr {
    char* m_str;          // +0x00  the built C-string
    void Set(char* s);    // 0x1b9e74 __thiscall
    void Append(char* s); // 0x1ba0c8 __thiscall
    void Reset();         // 0x1b9c69 __thiscall
    void Free1b9b93();    // 0x1b9b93 (reloc-masked)
};

#endif // SRC_GRUNTZ_GAMEKEYSTR_H
