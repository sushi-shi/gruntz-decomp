#ifndef SRC_REZ_REZMGR_H
#define SRC_REZ_REZMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Gruntz/String.h>
#include <Rez/RezList.h>

class CRezDir;

class CRezArchive;

class CRezItmBase {
public:
    CRezItmBase(CRezArchive* parent);

    virtual void Noop();
    virtual ~CRezItmBase();
    virtual i32 Read(i32 off, i32 base, u32 count, void* buf) = 0;
    virtual i32 Write(i32 base, i32 off, u32 count, void* buf) = 0;
    virtual i32 Open(char* name, b32 readonly, b32 write) = 0;
    virtual i32 Close() = 0;
    virtual i32 Flush() = 0;
    virtual i32 Check() = 0;

    CRezItmBase* m_next;
    CRezItmBase* m_prev;
    CRezArchive* m_parent;
};

class CRezItm : public CRezItmBase {
public:
    CRezItm(CRezArchive* parent);
    virtual ~CRezItm() OVERRIDE;

    virtual i32 Read(i32 off, i32 base, u32 count, void* buf) OVERRIDE;

    virtual i32 Write(i32 base, i32 off, u32 count, void* buf) OVERRIDE;

    virtual i32 Open(char* filename, b32 readonly, b32 write) OVERRIDE;

    virtual i32 Close() OVERRIDE;

    virtual i32 Flush() OVERRIDE;

    virtual i32 Check() OVERRIDE;

    FILE* m_fp;
    char* m_readBuf;
    b32 m_readonly;
    i32 m_reserved1c;
    i32 m_pos;
};

class CRezDir : public CRezItmBase {
public:
    CRezDir(CRezArchive* parent, i32 maxOpen);
    virtual ~CRezDir() OVERRIDE;

    virtual i32 Read(i32 off, i32 base, u32 count, void* buf) OVERRIDE;
    virtual i32 Write(i32 base, i32 off, u32 count, void* buf) OVERRIDE;
    virtual i32 Open(char* name, b32 readonly, b32 write) OVERRIDE;
    virtual i32 Close() OVERRIDE;
    virtual i32 Flush() OVERRIDE;
    virtual i32 Check() OVERRIDE;

    CRezList m_openList;
    CRezList m_closedList;
    i32 m_openCount;
    i32 m_maxOpen;
    b32 m_readonly;
    i32 m_write;
};

#endif // SRC_REZ_REZMGR_H
