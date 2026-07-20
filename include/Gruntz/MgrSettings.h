#ifndef GRUNTZ_CMGRSETTINGS_H
#define GRUNTZ_CMGRSETTINGS_H

#include <Mfc.h> // real MFC CMapStringToOb / CObject (Lookup 0x1b8008, reloc-masked)
class CImage; // the resolved m_38 frame element (<Image/CImage.h>)
#include <Gruntz/GameRegistry.h>
#include <Gruntz/SerialArchive.h> // shared CSerialArchive stream (Read @ +0x2c / Write @ +0x30)
#include <Ints.h>
#include <rva.h>

#include <DDrawMgr/DDrawWorkerRegistry.h>

#include <Gruntz/SerialCounter.h>

SIZE_UNKNOWN(CMgrSettings);
class CMgrSettings {
public:
    i32 Serialize(CSerialArchive* arc, i32 mode, i32 a3, i32 a4); // 0x109e00

    i32 m_00, m_04, m_08, m_0c;          // +0x00..+0x0c (m_0c unstreamed)
    double m_10, m_18, m_20, m_28, m_30; // +0x10..+0x30
    CImage* m_38; // +0x38 resolved frame reference (a CImageSet frame; ex void*)
};

#endif // GRUNTZ_CMGRSETTINGS_H
