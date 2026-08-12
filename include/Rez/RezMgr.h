#ifndef SRC_REZ_REZMGR_H
#define SRC_REZ_REZMGR_H

#include <rva.h>

#include <Mfc.h>

#include <Bute/Hash.h>
#include <Enums.h>
#include <Gruntz/String.h>
#include <Rez/RezList.h>

class CRezDir;

class CSymParser;

class CRezItmBase {
public:
    CRezItmBase(void* parent);

    virtual void Noop();
    virtual ~CRezItmBase();
    virtual i32 Read(i32 off, i32 base, u32 count, void* buf) = 0;
    virtual i32 Write(i32 base, i32 off, u32 count, void* buf) = 0;
    virtual i32 Open(char* name, i32 readonly, i32 write) = 0;
    virtual i32 Close() = 0;
    virtual i32 Flush() = 0;
    virtual i32 Check() = 0;

    CRezItmBase* m_next;
    CRezItmBase* m_prev;
    CSymParser* m_parent;
};

class CRezItm : public CRezItmBase {
public:
    CRezItm(void* parent);
    virtual ~CRezItm() OVERRIDE;

    virtual i32 Read(i32 off, i32 base, u32 count, void* buf) OVERRIDE;

    virtual i32 Write(i32 base, i32 off, u32 count, void* buf) OVERRIDE;

    virtual i32 Open(char* filename, i32 readonly, i32 write) OVERRIDE;

    virtual i32 Close() OVERRIDE;

    virtual i32 Flush() OVERRIDE;

    virtual i32 Check() OVERRIDE;

    FILE* m_fp;
    char* m_readBuf;
    i32 m_readonly;
    i32 m_reserved1c;
    i32 m_pos;
};

class CRezDir : public CRezItmBase {
public:
    CRezDir(void* parent, i32 maxOpen);
    virtual ~CRezDir() OVERRIDE;

    virtual i32 Read(i32 off, i32 base, u32 count, void* buf) OVERRIDE;
    virtual i32 Write(i32 base, i32 off, u32 count, void* buf) OVERRIDE;
    virtual i32 Open(char* name, i32 readonly, i32 write) OVERRIDE;
    virtual i32 Close() OVERRIDE;
    virtual i32 Flush() OVERRIDE;
    virtual i32 Check() OVERRIDE;

    CRezList m_openList;
    CRezList m_closedList;
    i32 m_openCount;
    i32 m_maxOpen;
    i32 m_readonly;
    i32 m_write;
};

class CRezDirNode;
class CSymParser;

class CRezDirNode {
public:
    i32 Load(i32 childFlag);

    char _vft0[4];
    char m_pad04[0x0c - 0x04];
    i32 m_off;
    u32 m_size;
    CRezDirNode* m_subdir;
    CSymParser* m_src;
    char m_pad1c[0x38 - 0x1c];
    CHashBase m_kids;
    char m_pad40[8];
    u8* m_buf;
};

#endif // SRC_REZ_REZMGR_H
