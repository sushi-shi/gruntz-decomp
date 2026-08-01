

#include <Gruntz/Grunt.h>
#include <Io/FileMem.h>
#include <rva.h>
#include <string.h>

#include <Mfc.h>

RVA(0x00056da0, 0xc7)
i32 CGruntCellRec::SerializeStrings(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }

    char buf[0x80];
    i32 i;
    for (i = 0; i < 5; i++) {
        memset(buf, 0, sizeof(buf));
        strcpy(buf, m_names[i]);
        ar->Write(buf, sizeof(buf));
    }

    ar->Write(&m_14, 0x10);
    ar->Write(&m_24, 0x10);
    ar->Write(&m_34, 0x10);
    ar->Write(&m_dirX, 0x20);
    return 1;
}

RVA(0x00056eb0, 0x94)
i32 CGruntCellRec::DeserializeStrings(CFileMemBase* ar) {
    if (ar == 0) {
        return 0;
    }

    char buf[0x80];
    i32 i;
    for (i = 0; i < 5; i++) {
        ar->Read(buf, sizeof(buf));
        m_names[i] = buf;
    }

    ar->Read(&m_14, 0x10);
    ar->Read(&m_24, 0x10);
    ar->Read(&m_34, 0x10);
    ar->Read(&m_dirX, 0x20);
    return 1;
}
