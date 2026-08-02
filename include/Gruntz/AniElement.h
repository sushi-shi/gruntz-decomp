#ifndef GRUNTZ_CANIELEMENT_H
#define GRUNTZ_CANIELEMENT_H

#include <rva.h>

#include <Mfc.h>

#include <Gruntz/AniRecordView.h>
#include <Ints.h>
#include <Wap32/Object.h>

SIZE_UNKNOWN();

struct CAniSource {
    char m_pad00[0x8];
    i32 m_flags;
    i32 m_count;

    u32 m_namelen;
    char m_pad14[0xc];
    char m_data[1];
};
SIZE_UNKNOWN();

class CAniElement : public CObject {
public:
    CAniElement() {
        m_flags = 0;
        m_name = 0;
    }
    virtual ~CAniElement() OVERRIDE;
    CObject* AtChecked(i32 i) const;
    i32 Build(void* ctx, CAniSource* src, i32 flags);
    i32 Configure(void* ctx, void* entry, i32 flags);
    i32 LoadFile(void* ctx, void* filename, i32 unused);

    void DeleteAll();

    i32 m_flags;
    CObArray m_records;
    char* m_name;
    float m_scale;
    i32 m_total;
};
SIZE(0x28);

#endif // GRUNTZ_CANIELEMENT_H
