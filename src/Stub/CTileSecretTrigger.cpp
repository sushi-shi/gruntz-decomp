// CTileSecretTrigger.cpp - MIGRATED to src/Gruntz/UserLogic.cpp.
//
// CTileSecretTrigger : CTileTrigger : CUserLogic. Its 1-arg ctor (0x10fa60) and
// its folded /GX EH-frame destructor (0x11540) are now reconstructed together,
// polymorphically, in src/Gruntz/UserLogic.cpp - the dtor's EH frame only folds
// with a real non-trivial base subobject (a manual-vptr stub model is frameless;
// see docs/patterns/eh-dtor-needs-base-subobject.md), so both must live in the
// MFC-capable game-logic TU, not in this MFC-free stub aggregate.
