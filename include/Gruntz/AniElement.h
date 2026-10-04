#ifndef GRUNTZ_CANIELEMENT_H
#define GRUNTZ_CANIELEMENT_H

#include <rva.h>

#include <Gruntz/AniRecordView.h>
#include <Ints.h>
#include <Utils/MapTyped.h>
#include <Wap32/Object.h>

struct CRezItm;

struct CAniSource {
    // @identity-TODO: Build starts with m_flags and reaches frame data through
    // m_data; neither retained header span is interpreted by the ANI readers.
    char m_pad00[0x8];
    i32 m_flags;
    i32 m_recordCount;

    u32 m_nameLengthBytes;
    char m_pad14[0xc];
    char m_data[1];
};

// @identity-TODO: the original class spelling is not established.
class CAnimationSequence : public CObject {
public:
    inline CObject* GetAt(i32 i) const;
    CAnimationSequence() {
        m_flags = 0;
        m_name = NULL;
    }
    virtual ~CAnimationSequence() OVERRIDE;
    CObject* AtChecked(i32 i) const;
    inline CAniFrameRecord* RecordAt(i32 index) const;
    i32 Build(SoundCueRegistry* soundRegistry, CAniSource* source, i32 flags);
    i32 LoadResource(SoundCueRegistry* soundRegistry, CRezItm* entry, i32 flags);
    i32 LoadFile(SoundCueRegistry* soundRegistry, const char* filename, i32 unused);

    void DeleteAll();

    i32 GetDurationMs() const {
        return m_durationMs;
    }

    i32 m_flags;
    CObArray m_records;
    char* m_name;
    float m_durationScale;
    i32 m_durationMs;
};

#define DELETE_ANIMATION_SEQUENCE_CONTENTS(index)                                                  \
    for (index = 0; index < m_records.GetSize(); index++) {                                        \
        CObject* item = m_records.GetAt(index);                                                    \
        if (item != NULL) {                                                                        \
            delete (static_cast<CAniFrameRecord*>(item));                                          \
        }                                                                                          \
    }                                                                                              \
    if (m_name != NULL) {                                                                          \
        delete[] m_name;                                                                           \
        m_name = NULL;                                                                             \
    }                                                                                              \
    m_records.RemoveAll()

#endif // GRUNTZ_CANIELEMENT_H
