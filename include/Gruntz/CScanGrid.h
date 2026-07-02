// CScanGrid.h - the shared board grid view (g_mgrSettings->m_70, CBrickz-shape):
// m_8 row table, m_c/m_10 dims, m_60 dirty rect, m_70/m_74 its size. Scan TUs that
// only read the dims still adopt the full shape (extra members are unreferenced;
// the pointed-to CScanCell stays a forward decl). Placeholder name; offsets are
// load-bearing.
#ifndef GRUNTZ_GRUNTZ_CSCANGRID_H
#define GRUNTZ_GRUNTZ_CSCANGRID_H

#include <rva.h>

#include <Win32.h> // RECT

struct CScanCell;

struct CScanGrid {
    char _00[8];
    CScanCell** m_8; // +0x08 row table
    i32 m_c, m_10;   // +0x0c width, +0x10 height
    char _14[0x60 - 0x14];
    RECT m_60;      // +0x60 dirty rect
    i32 m_70, m_74; // +0x70/0x74 its size
};

#endif // GRUNTZ_GRUNTZ_CSCANGRID_H
