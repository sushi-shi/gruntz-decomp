#include <StdAfx.h>
#include <Utils/Text.h>

#include <Ints.h>

#include <Gruntz/Grunt.h>
#include <Gruntz/SerialArchive.h>
#include <Io/FileMem.h>

#include <string.h>

i32 CGruntCellRec::SerializeStrings(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];
    i32 i;
    for (i = 0; i < 5; i++) {
        memset(buf, 0, sizeof(buf));
        if (!copyTextToBuffer((m_names[i]), buf, sizeof(buf))) return 0;
        ar->Write(buf, sizeof(buf));
    }

    ar->Write(&m_rects[0], sizeof(m_rects[0]));
    ar->Write(&m_rects[1], sizeof(m_rects[1]));
    ar->Write(&m_rects[2], sizeof(m_rects[2]));
    ar->Write(&m_motion, sizeof(m_motion));
    return 1;
}

i32 CGruntCellRec::DeserializeStrings(CFileMemBase* ar) {
    if (ar == NULL) {
        return 0;
    }

    char buf[SERIAL_NAME_LEN];
    i32 i;
    for (i = 0; i < 5; i++) {
        ar->Read(buf, sizeof(buf));
        m_names[i] = buf;
    }

    ar->Read(&m_rects[0], sizeof(m_rects[0]));
    ar->Read(&m_rects[1], sizeof(m_rects[1]));
    ar->Read(&m_rects[2], sizeof(m_rects[2]));
    ar->Read(&m_motion, sizeof(m_motion));
    return 1;
}
