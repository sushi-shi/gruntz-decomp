// BSecObj10fCtor.cpp - CBSecObj10f::CBSecObj10f (0x16f680), the empty out-of-line
// ctor of CButeSection's +0x10f sub-object: a 3-byte `mov eax,ecx; ret` COMDAT that
// just returns `this`. The linker placed this orphan COMDAT in the gap between
// MovingLogic (ends 0x16f67a) and Blowfish (0x16f6c0), so it is homed here as a
// dedicated 1-function TU. Binds CButeSection's ctor CALL @0x1702da to the real RVA
// (was reloc-UNBOUND while the ctor stayed a declared-only external).
#include <Bute/ButeSection.h>

RVA(0x0016f680, 3)
CBSecObj10f::CBSecObj10f() {}
