// MfcArchive.h - a minimal reloc-masked accessor over the real MFC CArchive, for
// the engine's CObject-array Serialize methods (CFaderArray, CRezBufferObject, ...).
// The four transfer entrypoints are the NAFXCW CArchive members reached by the
// inlined MFC CArray<T>::Serialize; their real mangled names differ but objdiff
// pairs the rel32 relocs by type, so a clean `call` to these declared-only methods
// reproduces the retail bytes. Cast `(MfcArchive*)&ar` at the Serialize body.
//
// Mode (+0x14): MFC CArchive::m_nMode; store==0, load==1, so IsStoring() is
// `(m_nMode & 1) == 0` (matches the retail `not; test 1; je` gate).
#ifndef WAP32_MFCARCHIVE_H
#define WAP32_MFCARCHIVE_H

#include <Ints.h>
#include <rva.h> // SIZE_UNKNOWN

struct MfcArchive {
    void WriteCount(u32 n);                    // 0x1c7334  ar.WriteCount / ar << count
    i32 ReadCount();                           // 0x1c7362  ar.ReadCount / ar >> count
    void WriteData(const void* p, i32 nBytes); // 0x1c7168  CArchive::Write
    void ReadData(void* p, i32 nBytes);        // 0x1c705a  CArchive::Read

    char _pad00[0x14];
    u32 m_nMode; // +0x14  store==0 / load==1 (bit0)

    i32 IsStoring() {
        return (m_nMode & 1) == 0;
    }
};
SIZE_UNKNOWN(MfcArchive); // partial reloc-masked accessor over the real MFC CArchive

#endif // WAP32_MFCARCHIVE_H
