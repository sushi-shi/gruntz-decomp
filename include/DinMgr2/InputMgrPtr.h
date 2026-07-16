// InputMgrPtr.h - the ONE declaration of the global input-manager singleton pointer in
// its real DirectInputMgr2* view (?g_inputMgr / _g_inputMgr, the reset-manager singleton
// at RVA 0x245570 / VA 0x645570). The definition + DATA(0x00245570) binding live in
// src/Gruntz/GruntzMgr.cpp (the run-state owner: `DirectInputMgr2* g_inputMgr = 0;`).
//
// Kept extern "C" so the DIR32 reference binds the definition's C-linkage name
// _g_inputMgr (a plain C++ extern mangles to ?g_inputMgr@@3PAVDirectInputMgr2@@A and
// leaves the reloc UNBOUND - see the InputDeviceConfig.cpp note). Consumers that use the
// DirectInputMgr2* view #include this instead of re-`extern`-ing it per-TU. Only a
// forward-decl is needed here; the includer pulls <DinMgr2/DirectInputMgr2.h> for the
// full type where it dereferences g_inputMgr. Emits no code -> matching-neutral.
//
// The void*/CSpawnOwner* view in RezSync.cpp is a DELIBERATELY-PARKED conflation (one
// object, two class names) with a divergent C++-linkage symbol; it is NOT folded here.
#ifndef DINMGR2_INPUTMGRPTR_H
#define DINMGR2_INPUTMGRPTR_H

class DirectInputMgr2;
extern "C" DirectInputMgr2* g_inputMgr; // *0x245570 reset-manager singleton (real view)

#endif // DINMGR2_INPUTMGRPTR_H
