#include <Gruntz/GruntDataRecord.h>
#include <Io/FileMem.h> // the serialize stream (CSerialArchive == the real CFileMemBase)
#include <rva.h>
#include <string.h> // memset / strcpy (inlined to rep stos / rep movs at /O2 /Oi)

#include <Mfc.h> // CString::operator= (the owned name members are CStrings)

RVA(0x00056da0, 0xc7)
i32 GruntDataRecord::SerializeStrings(CSerialArchive* ar) {
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

RVA(0x00056eb0, 0x94)
i32 GruntDataRecord::DeserializeStrings(CSerialArchive* ar) {
    if (ar == 0) {
        return 0;
    }

    char buf[0x80];
    i32 i;
    for (i = 0; i < 5; i++) {
        ar->Read(buf, sizeof(buf));
        *reinterpret_cast<CString*>(&m_str[i]) = buf;
    }

    ar->Read(m_14, 0x10);
    ar->Read(m_24, 0x10);
    ar->Read(m_34, 0x10);
    ar->Read(m_48, 0x20);
    return 1;
}
