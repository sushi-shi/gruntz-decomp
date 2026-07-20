#include <rva.h>
#include <Gruntz/GruntzCmdMgr.h>

#include <Net/KeyedList.h>

RVA(0x000c5280, 0x49)
CKeyedList::~CKeyedList() {
    Clear();
}
