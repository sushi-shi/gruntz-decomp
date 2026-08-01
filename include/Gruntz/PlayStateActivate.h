#ifndef GRUNTZ_PLAYSTATEACTIVATE_H
#define GRUNTZ_PLAYSTATEACTIVATE_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>

class CGruntzMgr;
class CStatusBarMgr;

void UpdateMgrScroll(CGruntzMgr* pm, class CStatusBarMgr* bar, i32 snapFlag);

#endif // GRUNTZ_PLAYSTATEACTIVATE_H
