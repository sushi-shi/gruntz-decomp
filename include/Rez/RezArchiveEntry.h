#ifndef REZ_REZARCHIVEENTRY_H
#define REZ_REZARCHIVEENTRY_H

#include <rva.h>

#include <Enums.h>
#include <Ints.h>
#include <Rez/RezHash.h>
#include <Rez/RezTypeTag.h>

#include <stddef.h>

class CRezDir;
class CRezTyp;

class CBaseRezFile;

struct CRezItm {
public:
    char* GetName() {
        return m_sName;
    }

    GZ_ENUM_RETURN(RezTypeTag, u32) GetType();
    u32 GetSize() {
        return m_nSize;
    }
    CRezDir* GetParentDir() {
        return m_pParentDir;
    }
    i32 GetTime() {
        return m_nTime;
    }
    u32 GetSeekPos() {
        return m_nCurPos;
    }
    void SetTime(i32 time) {
        m_nTime = time;
    }
    u32 DirectRead_GetFileOffset() {
        return m_nFilePos;
    }
    char* Load();
    i32 UnLoad();

    char* GetDir();

    char* GetPath(char* destination, i32 size);
    i32 Seek(u32 position);
    i32 Get(u8* destination, u32 position, u32 byteCount);
    i32 Get(u8* destination);
    i32 Get(void* destination, u32 position, u32 byteCount) {
        i32 result = Get(static_cast<u8*>(destination), position, byteCount);
        return result;
    }
    i32 Get(void* destination) {
        i32 result = Get(static_cast<u8*>(destination));
        return result;
    }
    u32 Read(u8* destination, u32 byteCount, u32 seekPosition = 0xffffffffu);
    u32 Read(void* destination, u32 byteCount, u32 seekPosition = 0xffffffffu) {
        u32 result = Read(static_cast<u8*>(destination), byteCount, seekPosition);
        return result;
    }
    char GetChar();
    i32 IsLoaded();
    i32 EndOfRes();

private:
    friend class CRezTyp;
    friend class CRezDir;
    friend class CRezMgr;

    CRezItm();

    void InitRezItm(
        CRezDir* directory,
        const char* name,
        void* resourceId,
        CRezTyp* type,
        void* comment,
        i32 size,
        i32 dataOffset,
        i32 time,
        i32 keyCount,
        void* keys,
        CBaseRezFile* storage
    );
    void TermRezItm();

    char* m_sName;
    CRezTyp* m_pType;
    i32 m_nTime;

    u32 m_nSize;
    CRezDir* m_pParentDir;

    i32 m_nFilePos;
    i32 m_nCurPos;

    CRezItmHashByName m_heName;
    CBaseRezFile* m_pRezFile;
    char* m_pData;
};

#define BEGIN_FILE_IMAGE_PARSE(source, format, bytes)                                              \
    FileImageFormat format;                                                                        \
    switch (static_cast<u32>(source->GetType())) {                                                 \
        case IMGTAG_PMB:                                                                           \
            format = FMT_BMP;                                                                      \
            break;                                                                                 \
        case IMGTAG_XCP:                                                                           \
            format = FMT_PCX;                                                                      \
            break;                                                                                 \
        case IMGTAG_DIR:                                                                           \
            format = FMT_RID;                                                                      \
            break;                                                                                 \
        case IMGTAG_DIP:                                                                           \
            format = FMT_PID;                                                                      \
            break;                                                                                 \
        default:                                                                                   \
            return 0;                                                                              \
    }                                                                                              \
    char* bytes = source->Load();                                                                  \
    if (bytes == NULL) {                                                                           \
        return 0;                                                                                  \
    }

#endif // REZ_REZARCHIVEENTRY_H
