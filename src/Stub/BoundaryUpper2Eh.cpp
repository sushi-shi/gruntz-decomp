// BoundaryUpper2Eh.cpp - upper-half (RVA >= 0x133370) engine_boundary backlog,
// /GX EH-frame siblings of BoundaryUpper2.cpp. /GX destructors whose destructible
// base/member subobjects force the MSVC unwind frame. Only OFFSETS + code shape are
// load-bearing.
#include <Mfc.h> // real MFC CString (embedded m_24 name member; ctor 0x1b9b93 / op= 0x1b9e74)
#include <Rez/RezFile.h>
#include <Rez/RezList.h>
#include <Ints.h>
#include <rva.h>

// The Rez heap free (0x1b9b82, __cdecl). C++ linkage (NOT extern "C") so MSVC5
// treats it as potentially-throwing and keeps the /GX base-subobject unwind frame.
void RezFree(void* p);

// (0x13c9b0 CRezDir13c9b0 + 0x13cb80 CRezDir13cb80 /GX dtors re-homed to
// src/Rez/RezMgr.cpp next to CRezDir - CObjListBase-family destructors sharing the
// 0x13c520 base-subobject dtor; kept distinct placeholder identities.)

// ---------------------------------------------------------------------------
// 0x1333b0 - CInputDevBase's standalone /GX base-subobject destructor (the middle
// level of the DirectInput device chain): stamp base vftable B @0x5ef680, ReleaseBase,
// stamp grand-base C @0x5ef670, BaseDtorC. Kept manual-vptr here: the LEAF dtors
// (keyboard 0x133300 / mouse 0x1334f0 / joystick 0x133460) are now modeled real-
// polymorphic in src/DinMgr2/DirectInputMgr2.cpp (cl inlines this base unwind into
// each), but this standalone base dtor can't be bound here without dup'ing DinMgr2's
// inline CInputDevBase (its emitted COMDAT is unbound). Grand-base ~G @0x133370 is
// matched standalone in BoundaryUpper2.cpp.
// ---------------------------------------------------------------------------
struct DevCfgChain {
    char _vft0[4];      // +0x00 foreign object vptr (reduced view; not owned/dispatched)
    void ReleaseBase(); // 0x1342b0
    void BaseDtorC();   // 0x134d50
    void DtorD1();
};
SIZE_UNKNOWN(DevCfgChain);
// @early-stop
// eh-dtor-needs-base-subobject wall (docs/patterns/eh-dtor-needs-base-subobject.md):
// body is byte-correct (stamp B / ReleaseBase / stamp C / BaseDtorC) but retail wraps
// it in a /GX frame with [esp+0x10] try-level stamps (0 / -1) from real base-subobject
// dtors, unreachable under this manual-vptr shape. DROPPED 42.7% -> 34.2% by an
// UNRELATED change: the device chain's base/root vtables 0x5ef680/0x5ef670 are now
// named ??_7CInputDevBase / ??_7CInputDevRoot (cl-emitted via VTBL in DirectInputMgr2.cpp),
// so this manual `&g_deviceConfigVtblB/C` stamp reloc-name-mismatches them. Binding a
// real ~CInputDevBase here would dup DinMgr2's inline base dtor - deferred to a final
// sweep that reunifies the whole chain in one TU.
RVA(0x001333b0, 0x55)
void DevCfgChain::DtorD1() {
    // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
    ReleaseBase();
    // vptr install dropped -> compiler-emitted vtable (% ok per drive-to-0)
    BaseDtorC();
}

// (0x17e7c0 CFxModeT1() re-homed to src/Gruntz/FxModeDesc.cpp as a real CFxModeDesc-
// derived class in <Gruntz/FxModeT1.h> - the type-1 FxMode variant with the CString
// name member. The CFxBase17e7c0/CFxModeT1 views are dissolved.)

// --- vtable catalog (view/base classes bound to their unit vtable rva) ---
