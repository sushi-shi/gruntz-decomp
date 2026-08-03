#pragma once
#include <Ints.h>
#include <rva.h>
#include <Wap32/Object.h>

#include <stddef.h>

struct RezElem40 {
    RezElem40();
    RECT m_startRect;
    RECT m_endRect;
    i32 m_reserved20;
    float m_scale;
};
SIZE(0x28);

struct CRezBufferObject : public CObject {
    RezElem40* m_pData;
    i32 m_nSize;
    i32 m_nMaxSize;
    i32 m_nGrowBy;

    CRezBufferObject() {
        m_pData = NULL;
        m_nGrowBy = 0;
        m_nMaxSize = 0;
        m_nSize = 0;
    }
    virtual ~CRezBufferObject() OVERRIDE;
    virtual void Serialize(CArchive& ar) OVERRIDE;

    void SetSize(i32 nNewSize, i32 nGrowBy);
};
SIZE_UNKNOWN();
