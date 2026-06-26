// GruntDataRecord.cpp - SerializeStrings (0x56da0), the per-record string/field
// writer the big grunt-data Serialize (0x53f90) calls over its 0x68-byte record
// array. See include/Gruntz/GruntDataRecord.h for the record layout + the writer
// (Write @ vtable slot +0x30). The five names are written as fixed 0x80-byte
// zero-padded fields (memset + strcpy inline to rep stos / repne scas + rep movs
// at /O2 /Oi); the four trailing fixed blocks are written verbatim.
#include <Gruntz/GruntDataRecord.h>
#include <rva.h>
#include <string.h> // memset / strcpy (inlined to rep stos / rep movs at /O2 /Oi)

#include <Mfc.h> // CString::operator= (the owned name members are CStrings)

// ---------------------------------------------------------------------------
// GruntDataRecord::SerializeStrings (0x56da0, __thiscall, 1 stdcall arg).
RVA(0x00056da0, 0xc7)
i32 GruntDataRecord::SerializeStrings(DataWriter* ar) {
    if (ar == 0) {
        return 0;
    }

    char buf[0x80];
    i32 i;
    for (i = 0; i < 5; i++) {
        memset(buf, 0, sizeof(buf));
        strcpy(buf, m_str[i]);
        ar->Write(buf, sizeof(buf));
    }

    ar->Write(m_14, 0x10);
    ar->Write(m_24, 0x10);
    ar->Write(m_34, 0x10);
    ar->Write(m_48, 0x20);
    return 1;
}

// ---------------------------------------------------------------------------
// GruntDataRecord::DeserializeStrings (0x56eb0, __thiscall, 1 stdcall arg). Read
// each 0x80-byte name field into a temp and assign it to the owned CString member
// (CString::operator= @0x1b9e74, reloc-masked), then read the four fixed blocks.
RVA(0x00056eb0, 0x94)
i32 GruntDataRecord::DeserializeStrings(DataWriter* ar) {
    if (ar == 0) {
        return 0;
    }

    char buf[0x80];
    i32 i;
    for (i = 0; i < 5; i++) {
        ar->Read(buf, sizeof(buf));
        *(CString*)&m_str[i] = buf;
    }

    ar->Read(m_14, 0x10);
    ar->Read(m_24, 0x10);
    ar->Read(m_34, 0x10);
    ar->Read(m_48, 0x20);
    return 1;
}
