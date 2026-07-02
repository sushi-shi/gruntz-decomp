// FileMem.cpp - CFileMem, the engine's CFile-derived save/pack file stream
// (wraps a CFileIO physical file behind a named, position-tracked CFile-style
// interface). Built /O1 + /GX (mfc profile): MFC-derived CFile-clone code, and
// the embedded CString member forces the C++ EH frame in ctor/dtor.
//
// REAL POLYMORPHIC (ALL-VTABLES phase): CFileMemBase (abstract, vtable 0x5efe68)
// + CFileMem (concrete, vtable 0x5efe30) are modeled as a 13-slot CFile-style
// virtual interface. cl auto-emits ??_7CFileMemBase / ??_7CFileMem and stamps the
// vptr in the ctor/dtor - the manual g_fileMem*Vtbl stamps are gone.
#include <Io/FileMem.h>
#include <rva.h>

// The inner CFileIO presents its file ops virtually. MSVC5 rejects a __thiscall
// fn-ptr typedef (C4234), so model the dispatch with a polymorphic VIEW class:
// dummy virtuals place the real slots at the right indices, and a call through it
// emits `mov eax,[file];call [eax+N]` with the thiscall convention for free.
// (See docs/patterns/dummy-virtual-slots.md.) The view is reinterpreted over the
// inner file's vptr; it is never constructed, so no vtable is emitted for it.
struct CFileIOView {
    virtual void v0();
    virtual void v1();
    virtual void v2();
    virtual void v3();
    virtual void v4();
    virtual void v5();
    virtual void v6();
    virtual void v7();
    virtual void v8();
    virtual void v9();
    virtual i32 Open(const char* name, u32 flags, void* err); // +0x28 (slot 10)
    virtual void v11();
    virtual void v12();
    virtual void v13();
    virtual i32 GetLength();                    // +0x38 (slot 14)
    virtual i32 Read(void* buf, u32 n);         // +0x3c (slot 15)
    virtual void Write(const void* buf, u32 n); // +0x40 (slot 16)
    virtual void v17();
    virtual void v18();
    virtual void v19();
    virtual void v20();
    virtual i32 Status(); // +0x54 (slot 21)
};

// ---------------------------------------------------------------------------
// CFileMemBase::CFileMemBase  (0x00157850)
// Base sub-object ctor: cl auto-stamps the base vptr (??_7CFileMemBase), then
// zero the two scalar fields + Empty the name. The CString member (with a dtor)
// installs the MSVC5 EH unwind frame.
RVA(0x00157850, 0x54)
CFileMemBase::CFileMemBase() {
    m_4 = 0;
    m_8 = 0;
    m_name.Empty();
}

// ~CFileMemBase - base teardown (unpinned; real body 0x157960 not reconstructed).
// Empty so cl emits ??_GCFileMemBase for the base vtable slot 0.
CFileMemBase::~CFileMemBase() {}

// ---------------------------------------------------------------------------
// CFileMem::~CFileMem  (0x00157980)
// cl stamps the derived vtable at entry, run Reset() (derived), destruct the
// inner CFileIO, call the base Reset(), then cl folds the base vtable restamp +
// the CString member dtor on unwind.
// @early-stop
// EH-dtor scheduling wall (~59%): the teardown logic is byte-faithful, but the
// virtual-dtor auto vtable restamps + the /GX trylevel store sequencing
// (1/0/2/-1) + the member-dtor dispatch differ from retail's manual sequence.
// Now real-polymorphic (ALL-VTABLES phase); deferred to the final sweep.
RVA(0x00157980, 0x74)
CFileMem::~CFileMem() {
    Reset();
    m_file.~CFileIO();
    CFileMemBase::Reset();
}

// ---------------------------------------------------------------------------
// CFileMemBase::Reset  (0x00157a40)  (base vtable slot +0xc / slot 3)
// Zero the two scalar fields, tail-jump CString::Empty.
RVA(0x00157a40, 0x10)
void CFileMemBase::Reset() {
    m_4 = 0;
    m_8 = 0;
    m_name.Empty();
}

// ---------------------------------------------------------------------------
// CFileMem::Reset  (0x00157a50)  (derived vtable slot +0xc / slot 3)
// Zero the position pair + the two base scalars, tail-jump CString::Empty.
RVA(0x00157a50, 0x16)
void CFileMem::Reset() {
    m_length = 0;
    m_offset = 0;
    m_4 = 0;
    m_8 = 0;
    m_name.Empty();
}

// ---------------------------------------------------------------------------
// CFileMemBase::SetName  (0x00165e30)  (slot 1, shared by both vtables)
// m_name = name; m_8 = a; m_4 = b; return 1.  (__thiscall, 3 stack args)
RVA(0x00165e30, 0x27)
i32 CFileMemBase::SetName(const char* name, i32 a, i32 b) {
    m_name = name;
    m_8 = a;
    m_4 = b;
    return 1;
}

// ---------------------------------------------------------------------------
// CFileMem::Open  (0x00165e60)
// If the name is empty, fail. Otherwise dispatch on the base virtual at slot
// +0x1c (a read-vs-create predicate): when set, open the inner file read-only
// and record its length; else open it for create (flags 0x1001) and zero the
// position pair. Returns 1 on a successful open, 0 on failure.
RVA(0x00165e60, 0x82)
i32 CFileMem::Open() {
    if (m_name.GetLength() == 0) {
        return 0;
    }

    if (WantRead()) {
        if (!((CFileIOView*)&m_file)->Open((const char*)m_name, 0, 0)) {
            return 0;
        }
        m_length = ((CFileIOView*)&m_file)->GetLength();
        m_offset = 0;
        return 1;
    }

    if (!((CFileIOView*)&m_file)->Open((const char*)m_name, 0x1001, 0)) {
        return 0;
    }
    m_length = 0;
    m_offset = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CFileMem::Ready  (0x00165ef0)
// Poke the inner file's status slot (+0x54) and report ready (1).
RVA(0x00165ef0, 0xf)
i32 CFileMem::Ready() {
    ((CFileIOView*)&m_file)->Status();
    return 1;
}

// ---------------------------------------------------------------------------
// CFileMem::Read  (0x00165f00)
// Read n bytes through the inner CFileIO (vtable +0x3c), advancing only the
// consumed-count m_offset by n. buf==0 -> early no-op (eax left = buf = 0); n==0 ->
// return 0. Fails (0) if the inner Read returns fewer than n bytes; else 1.
RVA(0x00165f00, 0x48)
i32 CFileMem::Read(void* buf, i32 n) {
    if (buf == 0) {
        return 0; // (no-op path; eax left = buf = 0 in retail)
    }
    if (n == 0) {
        return 0;
    }
    if (((CFileIOView*)&m_file)->Read(buf, n) != n) {
        return 0;
    }
    m_offset += n;
    return 1;
}

// ---------------------------------------------------------------------------
// CFileMem::Write  (0x00165f50)
// Write n bytes through the inner CFileIO (vtable +0x40), advancing the position
// pair by n. buf==0 -> early no-op; n==0 -> return 0. Returns 1 on a write.
RVA(0x00165f50, 0x45)
i32 CFileMem::Write(const void* buf, i32 n) {
    if (buf == 0) {
        return 0; // (no-op path; eax left untouched in retail)
    }
    if (n == 0) {
        return 0;
    }
    ((CFileIOView*)&m_file)->Write(buf, n);
    m_length += n;
    m_offset += n;
    return 1;
}

// ===========================================================================
// Class-metadata annotations (EOF-hosted; /O1+/GX MFC TU). CFileMemBase/CFileMem
// are real polymorphic (SIZE_UNKNOWN + VTBL live atop the class in FileMem.h);
// CFileIOView is a dummy-virtual reinterpret view (never constructed -> no
// emitted vtable).
// ===========================================================================
SIZE_UNKNOWN(CFileIOView);
