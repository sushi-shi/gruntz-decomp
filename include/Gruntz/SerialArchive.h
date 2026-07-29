#ifndef GRUNTZ_SERIALARCHIVE_H
#define GRUNTZ_SERIALARCHIVE_H

class CFileMemBase;

// The `mode` argument every Serialize/SyncState/ValidateByType/BroadcastCmd in the game
// takes. PROVEN arms only: CTileTriggerSwitchLogic::ValidateByType (0x113860) is the
// minimal witness - `cmp edx,4 / je <write>` then `cmp edx,7 / jne <skip>` - and the same
// 4=Write / 7=Read pair drives every body in the family. The rest of the space (the
// factory dispatches 1..0xa at SerialObjectFactory 0xd2a0) is deliberately NOT enumerated.
typedef enum SerialMode {
    SERIAL_SAVE = 4, // writer arm: CFileMemBase::Write (archive vtable +0x30)
    SERIAL_LOAD = 7, // reader arm: CFileMemBase::Read  (archive vtable +0x2c)
} SerialMode;

// --- the TU's extern surface (moved out of the .cpp; addresses/thunk
// VAs are reloc-masked at use) ---
// UNDEFINED DATA: a char[] datum here is a STRING (or a run of them); its
// extent is not boundable from the named-symbol gaps (the unnamed $SG literals
// in between get swallowed). Inline the literal at its use site instead.
extern "C" char g_syncErrMsgBuf[];

#endif // GRUNTZ_SERIALARCHIVE_H
