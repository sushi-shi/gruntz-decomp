#ifndef GRUNTZ_GRUNTZ_CFADERMGR_H
#define GRUNTZ_GRUNTZ_CFADERMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/Fader.h>
#include <Ints.h>
#include <Wap32/Object.h>

#include <afxtempl.h>

GZ_ENUM_FORWARD(FaderKind);

struct CFaderArray : public CObject {
    virtual ~CFaderArray() OVERRIDE;
    virtual void Serialize(CArchive& ar) OVERRIDE;

    CFader** m_pData;
    i32 m_nSize;
    i32 m_nMaxSize;
    i32 m_nGrowBy;

    CFaderArray();
    i32 Add(CFader* fader);
    i32 GetSize() const;
    void RemoveAt(i32 index, i32 count = 1);
    void SetAtGrow(i32 index, CFader* fader);
    void SetSize(i32 size, i32 growBy = -1);
};
SIZE_UNKNOWN();

inline CFaderArray::CFaderArray() {
    m_pData = NULL;
    m_nGrowBy = 0;
    m_nMaxSize = 0;
    m_nSize = 0;
}

inline CFaderArray::~CFaderArray() {
    if (m_pData) {
        delete[] m_pData;
    }
}

inline i32 CFaderArray::Add(CFader* fader) {
    i32 index = m_nSize;
    SetAtGrow(index, fader);
    return index;
}

inline i32 CFaderArray::GetSize() const {
    return m_nSize;
}

inline void CFaderArray::RemoveAt(i32 index, i32 count) {
    i32 moveCount = m_nSize - (index + count);
    CFader** dst = m_pData + index;
    if (moveCount) {
        memcpy(dst, m_pData + (index + count), moveCount * sizeof(CFader*));
    }
    m_nSize -= count;
}

inline void CFaderArray::SetAtGrow(i32 index, CFader* fader) {
    if (index >= m_nSize) {
        SetSize(index + 1);
    }
    m_pData[index] = fader;
}

inline void CFaderArray::SetSize(i32 size, i32 growBy) {
    if (growBy != -1) {
        m_nGrowBy = growBy;
    }

    if (size == 0) {
        if (m_pData) {
            delete[] m_pData;
            m_pData = NULL;
        }
        m_nSize = m_nMaxSize = 0;
    } else if (m_pData == NULL) {
        m_pData = new CFader*[size];
        memset(m_pData, 0, size * sizeof(CFader*));
        m_nSize = m_nMaxSize = size;
    } else if (size <= m_nMaxSize) {
        if (size > m_nSize) {
            ConstructElements<CFader*>(m_pData + m_nSize, size - m_nSize);
        }
        m_nSize = size;
    } else {
        i32 grow = m_nGrowBy;
        if (grow == 0) {
            grow = m_nSize / 8;
            grow = grow < 4 ? 4 : (grow > 0x400 ? 0x400 : grow);
        }

        i32 newMax = size < m_nMaxSize + grow ? m_nMaxSize + grow : size;
        CFader** data = new CFader*[newMax];
        memcpy(data, m_pData, m_nSize * sizeof(CFader*));
        ConstructElements<CFader*>(&data[m_nSize], size - m_nSize);
        delete[] m_pData;
        m_pData = data;
        m_nSize = size;
        m_nMaxSize = newMax;
    }
}

class CFaderMgr {
public:
    CFaderMgr();
    ~CFaderMgr();
    i32 SetConfig(class CDDSurface* src, class CDDSurface* dst, class CDDrawPtrCollections* pool);
    void FreeAll();
    CFader* Add(FaderKind nFaderType, class CFxModeDesc* pInit);

    void Remove(CFader* pFader);
    void DeleteAll();

    void Trace(CString s);

    class CDDSurface* m_timerArgA;
    class CDDSurface* m_timerArgB;
    i32 m_active;
    i32 m_reserved0c;
    CFaderArray m_arr;

    class CDDrawPtrCollections* m_sharedPtrColl;
};
SIZE_UNKNOWN();

#endif // GRUNTZ_GRUNTZ_CFADERMGR_H
