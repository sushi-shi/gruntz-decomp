#ifndef SRC_IO_FILEMEM_H
#define SRC_IO_FILEMEM_H

#include <rva.h>

#include <Mfc.h>

#include <Enums.h>
#include <Ints.h>
#include <Io/FileStream.h>

class CFileMemBase {
public:
    // Kept OUT OF LINE deliberately: retail HAS a standalone body at 0x157850
    // AND expands it at other sites - the /Ob1 inline-budget divergence
    // (docs/patterns/ob1-inline-budget-divergence.md), not a mis-model. Moving it
    // inline is fuzzy-neutral (87.75 either way, measured) and un-claims four
    // retail functions, so the OOL definition stays.
    CFileMemBase();

    // The same seed, INLINE: LoadRecordFile (0x156ad0) expands the whole ctor in
    // place, where SnapshotChildren/RestoreChildren call the pinned 0x157850.
    enum EInlineSeed {
        INLINE_SEED
    };
    CFileMemBase(EInlineSeed) {
        SeedFields();
    }
    virtual ~CFileMemBase() {
        Close();
    }
    virtual i32 SetName(const char* name, i32 a, i32 b);
    virtual void Close() {
        Reset();
    }

    virtual void Reset();
    virtual CString GetName();
    virtual i32 GetLength() = 0;
    virtual i32 GetOffset() = 0;
    virtual i32 WantRead();
    virtual i32 WantCreate();
    virtual i32 Open() = 0;
    virtual i32 Ready() = 0;
    virtual i32 Read(void* buf, i32 n) = 0;
    virtual i32 Write(const void* buf, i32 n) = 0;

    void SeedFields() {
        m_4 = 0;
        m_mode = 0;
        m_name.Empty();
    }

    i32 m_4;
    i32 m_mode;
    CString m_name;
};
SIZE(0x10);

class CFileMem : public CFileMemBase {
public:
    CFileMem() {
        Reset();
    }
    CFileMem(EInlineSeed t) : CFileMemBase(t) {
        SeedMemFields();
    }
    virtual ~CFileMem() OVERRIDE {
        Close();
    }
    virtual void Close() OVERRIDE {
        Reset();
    }
    virtual void Reset() OVERRIDE;

    void SeedMemFields() {
        m_length = 0;
        m_offset = 0;
        SeedFields();
    }
    virtual i32 GetLength() OVERRIDE;
    virtual i32 GetOffset() OVERRIDE;
    virtual i32 Open() OVERRIDE;
    virtual i32 Ready() OVERRIDE;
    virtual i32 Read(void* buf, i32 n) OVERRIDE;
    virtual i32 Write(const void* buf, i32 n) OVERRIDE;

    CFile m_file;
    i32 m_length;
    i32 m_offset;
};
SIZE(0x28);

#endif // SRC_IO_FILEMEM_H
