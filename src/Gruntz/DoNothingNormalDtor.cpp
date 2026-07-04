// CDoNothingNormalDtor.cpp - CDoNothingNormal's /GX leaf destructor (C:\Proj\Gruntz).
//
// The ctor stub still lives in src/Stub/CDoNothingNormal.cpp (stub-world base);
// this TU hosts the leaf dtor against the matched <Gruntz/UserLogic.h> teardown.
#include <Gruntz/DoNothingNormalDtor.h>

// CDoNothingNormal::~CDoNothingNormal @0x0000f8a0 - folds the bare CUserLogic
// teardown: store the CUserLogic vptr (0x5e705c), inline-destruct the +0x18 link
// (the embedded ~EngStr call 0x16d2a0), store the CUserBase vptr (0x5e70b4). The
// destructible link forces the /GX EH frame. Byte-identical to ~CDoNothing
// @0x0000f770.
RVA(0x0000f8a0, 0x44)
CDoNothingNormal::~CDoNothingNormal() {}

// Realize ??_7CDoNothingNormal@@6B@ (0x1e859c): retail's dtor folds straight to the
// CUserLogic teardown and never references the leaf vtable (so ~CDoNothingNormal only
// emits the base ??_7CUserLogic/??_7CUserBase restamps), and the logic-worker ctor
// stamps the leaf vtable vptr-MIDDLE - neither anchors the leaf COMDAT. A spurious
// `new CDoNothingNormal` references the implicit vptr-FIRST leaf ctor, whose stamp
// (the escaping object keeps it) emits ??_7CDoNothingNormal. Unpaired (no RVA) ->
// matching-neutral; it does NOT touch the 0xf8a0 dtor codegen.
void* operator new(u32);
CDoNothingNormal* RealizeCDoNothingNormal();
CDoNothingNormal* RealizeCDoNothingNormal() {
    return new CDoNothingNormal();
}

#include <rva.h>
