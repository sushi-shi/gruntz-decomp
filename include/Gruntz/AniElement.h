#ifndef GRUNTZ_CANIELEMENT_H
#define GRUNTZ_CANIELEMENT_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/AniRecordView.h>
#include <Ints.h>
#include <Utils/MapTyped.h>
#include <Wap32/Object.h>

struct CRezItm;

struct CAniSource {
    char m_pad00[0x8];
    i32 m_flags;
    i32 m_count;

    u32 m_namelen;
    char m_pad14[0xc];
    char m_data[1];
};

class CAniElement : public CObject {
public:
    inline CObject* GetAt(i32 i) const;
    CAniElement() {
        m_flags = 0;
        m_name = NULL;
    }
    virtual ~CAniElement() OVERRIDE;
    CObject* AtChecked(i32 i) const;
    inline CAniRecordView* RecordAt(i32 index) const;
    i32 Build(SoundCueRegistry* ctx, CAniSource* src, i32 flags);
    i32 Configure(SoundCueRegistry* ctx, CRezItm* entry, i32 flags);
    i32 LoadFile(SoundCueRegistry* ctx, const char* filename, i32 unused);

    void DeleteAll();

    i32 m_flags;
    CObArray m_records;
    char* m_name;
    float m_scale;
    i32 m_durationMs;
};

#define DELETE_ANI_ELEMENT_CONTENTS(index)                                                         \
    for (index = 0; index < m_records.GetSize(); index++) {                                        \
        CObject* item = m_records.GetAt(index);                                                    \
        if (item != NULL) {                                                                        \
            delete (static_cast<CAniRecordView*>(item));                                           \
        }                                                                                          \
    }                                                                                              \
    if (m_name != NULL) {                                                                          \
        delete[] m_name;                                                                           \
        m_name = NULL;                                                                             \
    }                                                                                              \
    m_records.SetSize(0, -1)

#endif // GRUNTZ_CANIELEMENT_H
