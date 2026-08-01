#ifndef SRC_REZ_REZFILE_H
#define SRC_REZ_REZFILE_H

#include <rva.h>

#include <Rez/RezMgr.h>

#include <Rez/RezList.h>
#include <Rez/RezAlloc.h>

extern "C" void* Eng_fopen(const char* path, const char* mode);
extern "C" i32 RezFClose(void* fp);
extern "C" u32 RezFRead(void* buf, u32 size, u32 n, void* fp);
extern "C" i32 RezFSeek(void* fp, i32 off, i32 origin);
extern "C" u32 RezFWrite(void* buf, u32 size, u32 n, void* fp);
extern "C" i32 Eng_fflush(void* fp);

extern const char s_rb[];
extern const char s_rPlusB[];
extern const char s_wPlusB[];

class CRezFile : public CRezItmBase {
public:
    CRezFile(void* parent, char* nameSrc, CRezDir* dir);

    virtual ~CRezFile() OVERRIDE;

    virtual void Noop() OVERRIDE;

    virtual i32 Read(i32 a, i32 pos, u32 count, void* buf) OVERRIDE;

    virtual i32 Write(i32 a, i32 pos, u32 count, void* buf) OVERRIDE;

    virtual i32 Open(char* name, i32 readonly, i32 write) OVERRIDE;
    virtual i32 Close() OVERRIDE;

    virtual i32 Flush() OVERRIDE;

    virtual i32 Check() OVERRIDE;

    i32 OpenFile();

    i32 CloseFile();

    char* m_name;
    FILE* m_handle;
    CRezDir* m_dir;
};
SIZE(0x1c);

extern "C" const char g_wildcard[];

extern "C" i32 RezDirLookup(void* fp);

#endif // SRC_REZ_REZFILE_H
