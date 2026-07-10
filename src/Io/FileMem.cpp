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

// The embedded CFileIO m_file presents its file ops virtually; they are reached
// through the shared CFileIODispatch view (FileStream.h) so a call lowers to the
// retail `mov eax,[m_file]; call [eax+N]` __thiscall dispatch without cl emitting
// CFileIO's full CFile vtable. (docs/patterns/dummy-virtual-slots.md.)

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

// ~CFileMemBase - base teardown. The RVA'd (scored) copy at 0x1578b0 lives in its own
// isolated TU src/Io/FileMemBaseDtor.cpp (delinker offset-0 pairing preserves its
// match; homing it into this multi-fn unit re-packs + craters CFileMem::~CFileMem).
// Empty here so cl emits ??_GCFileMemBase for the base vtable slot 0.
CFileMemBase::~CFileMemBase() {}

// ---------------------------------------------------------------------------
// GetName (slot 4, 0x157920): return a by-value copy of the stream's name.
RVA(0x00157920, 0x20)
CString CFileMemBase::GetName() {
    return m_name;
}

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

// CFileMemBase::Reset (0x00157a40) is now an inline member in the header.

// CFileMem::Reset (0x00157a50) is now an inline member in the header.

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
        if (!((CFileIODispatch*)&m_file)->Open(m_name, 0, 0)) {
            return 0;
        }
        m_length = ((CFileIODispatch*)&m_file)->GetLength();
        m_offset = 0;
        return 1;
    }

    if (!((CFileIODispatch*)&m_file)->Open(m_name, 0x1001, 0)) {
        return 0;
    }
    m_length = 0;
    m_offset = 0;
    return 1;
}

// ---------------------------------------------------------------------------
// CFileMem::Ready  (0x00165ef0)
// Poke the inner file's slot 21 (+0x54; CFileIO::Close) and report ready (1).
RVA(0x00165ef0, 0xf)
i32 CFileMem::Ready() {
    ((CFileIODispatch*)&m_file)->Close();
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
    if (((CFileIODispatch*)&m_file)->Read(buf, n) != n) {
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
    ((CFileIODispatch*)&m_file)->Write(buf, n);
    m_length += n;
    m_offset += n;
    return 1;
}

// Class-metadata (CFileMemBase / CFileMem SIZE + VTBL) lives atop their decls in
// FileMem.h; the shared CFileIODispatch dispatch view is in FileStream.h.
