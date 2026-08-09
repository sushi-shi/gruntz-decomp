#ifndef SRC_REZ_REZFILE_H
#define SRC_REZ_REZFILE_H

#include <rva.h>

#include <Enums.h>
#include <Rez/RezList.h>
#include <Rez/RezMgr.h>

extern const char s_rb[];
extern char s_rPlusB[];
extern char s_wPlusB[];

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

extern "C" char g_wildcard[];

#endif // SRC_REZ_REZFILE_H
