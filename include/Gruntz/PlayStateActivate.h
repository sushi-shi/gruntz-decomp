// PlayStateActivate.h - the PlayStateActivate TU's external declarations.
#ifndef GRUNTZ_PLAYSTATEACTIVATE_H
#define GRUNTZ_PLAYSTATEACTIVATE_H

#include <Mfc.h> // afx.h FIRST (umbrella for any Win32 types below)
#include <Ints.h>
#include <rva.h>

class CGruntzMgr;
class CStatusBarMgr;

void UpdateMgrScroll(CGruntzMgr* pm, class CStatusBarMgr* bar, i32 snapFlag); // reloc-masked

#endif // GRUNTZ_PLAYSTATEACTIVATE_H
