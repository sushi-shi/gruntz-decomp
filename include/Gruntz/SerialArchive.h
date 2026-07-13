// SerialArchive.h - the shared WAP32 serialization stream every CUserLogic-derived
// Serialize override is handed (CMenuSparkle, CMovingLogic, CMgrSettings,
// CSerialObjRef, ...).
//
// IT IS THE REAL CFileMemBase (<Io/FileMem.h>, VTBL 0x1efe68) - the abstract
// memory/file stream base whose slot 11 (+0x2c) Read and slot 12 (+0x30) Write are
// exactly the two slots every Serialize body dispatches (both __purecall in the base:
// an interface, which is why the archive is only ever passed in, never constructed).
//
// PROOF (the TU disproved its own view): CDDrawSurfaceMgr::SnapshotChildren
// (0x156020) builds ONE stack-local stream and, in the SAME function, drives it as a
// CFileMem - Reset (0x157a50), SetName (0x165e30), Open (0x165e60), Write (0x165f50),
// Read (0x165f00), Ready (0x165ef0), every one of them CFileMem/CFileMemBase's own
// slot RVA - and then passes `(CSerialArchive*)&S` straight into
// ForEachSerialize_15b020. One object, two names.
//
// So the 13-slot `CSerialArchive` placeholder (11 filler slots + Read/Write) is
// dissolved onto the canonical class; the ~20 per-TU archive views it had already
// absorbed (CMsSerialArchiveVtbl / CMgrArchiveVtbl / Serializer / SyncStream /
// CWwdArchive / CRecReader / ...) land on the real stream with it.
#ifndef GRUNTZ_SERIALARCHIVE_H
#define GRUNTZ_SERIALARCHIVE_H

// Pointer-only for most includers, so this stays a fwd decl + typedef: pulling the
// real <Io/FileMem.h> (-> Mfc.h + FileStream.h) into all ~119 archive TUs is a large
// decl-count ripple for nothing. The ~40 TUs that DISPATCH on the stream
// (`ar->Read/Write`) include <Io/FileMem.h> themselves.
// NB: never fwd-declare this under the OLD placeholder name - an elaborated
// `struct CSerialArchive;` re-declares a DISTINCT class and silently out-ranks the
// typedef under MSVC5 (no C2371 at the use site; the typedef just loses).
class CFileMemBase;
typedef CFileMemBase CSerialArchive;

#endif // GRUNTZ_SERIALARCHIVE_H
