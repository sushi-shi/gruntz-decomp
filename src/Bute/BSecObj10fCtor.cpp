// BSecObj10fCtor.cpp - CButeTail::CButeTail (0x16f680), the empty out-of-line ctor
// of CButeMgr's +0x10f tail object: a 3-byte `mov eax,ecx; ret` COMDAT that just
// returns `this`. The linker placed this orphan COMDAT in the gap between
// MovingLogic (ends 0x16f67a) and Blowfish (0x16f6c0), so it is homed here as a
// dedicated 1-function TU. Binds CButeMgr::CButeMgr's ctor CALL @0x1702da to the
// real RVA. (Ex CBSecObj10f - the CButeSection twin's tail class - dissolved into
// the one CButeTail.)
#include <Bute/ButeMgr.h>

RVA(0x0016f680, 3)
CButeTail::CButeTail() {}
