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

// The field descriptor `ar->m_14` points at: its first word (as sliced by the
// archive) is the type id. (Kept exactly as the TypeKeyColl.cpp view - m_1c lives
// at offset 0 of the sliced record; the name is a placeholder, offset is load-bearing.)
SIZE_UNKNOWN(CXferField);
struct CXferField {
    i32 m_1c; // type id (offset 0 of the record slice)
};

// The archive record ProjTypeXfer drives.
SIZE_UNKNOWN(CXferArchive);
struct CXferArchive {
    void Xfer0c(void* name); // vtbl +0x0c  per-field name transfer hook
    void Xfer10(i32 id);     // vtbl +0x10  per-field id transfer hook
    void Xfer14(void* name); // vtbl +0x14  per-field name transfer hook
    char _vft0[4];           // +0x00  engine vptr (reduced view; not dispatched)
    char pad_04[0x14 - 0x04];
    CXferField* m_14; // +0x14  field descriptor (its +0x1c is the type id)
};

// 0x16e4f0 - serialize the type-name table entry resolved from `ar`. Body lives in
// TypeKeyColl.cpp; reloc-masked from the dispatchers' default case.
i32 ProjTypeXfer(CXferArchive* ar);

#endif // SRC_GRUNTZ_XFERARCHIVE_H
