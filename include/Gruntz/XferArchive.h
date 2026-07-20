// XferArchive.h - the shared, single definition of CXferArchive, the type-name
// serialization archive-record that ProjTypeXfer (0x16e4f0) drives. Formerly a
// TypeKeyColl.cpp-local view + two forward-decl copies (StateDispatch.cpp /
// MsgDispatch.cpp, whose state dispatchers hit it via the default case). Folded
// here (wave 3). Identity is a placeholder (no recovered symbol for the record
// class); only OFFSETS + the vtable-slot conventions are load-bearing.
//
// NOTE: Xfer0c/Xfer10/Xfer14 are the archive vtable's per-field transfer hooks
// (slots +0x0c/+0x10/+0x14); modeled non-virtual here to preserve the current
// match - the PMF/vtable -> real-virtual conversion is worker C's domain.
#ifndef SRC_GRUNTZ_XFERARCHIVE_H
#define SRC_GRUNTZ_XFERARCHIVE_H

#include <Ints.h>
#include <rva.h>

// CXferArchive + CXferField are GONE (retail-arbitrated, llvm-objdump @0x16e4f0):
// the "archive" IS the CUserLogic object (the dispatchers pass ctl->m_logic in),
// the id is [logic+0x14]->m_1c (the aux union's REAL +0x1c - the slice@0 model was
// wrong), and the hooks are VIRTUAL dispatches through the logic's real slots:
// [3] XferName(name), [4] FireActivation(id), [5] FinalizeStep((i32)name).

// 0x16e4f0 - resolve the logic's type name + activate it. Body in TypeKeyColl.cpp.
i32 ProjTypeXfer(class CUserLogic* logic);

#endif // SRC_GRUNTZ_XFERARCHIVE_H
