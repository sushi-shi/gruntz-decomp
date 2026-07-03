// RezFile.cpp - CRezFile, the WAP32 RezMgr managed open-file wrapper (a single
// archive FILE* whose handle lifetime is bounded by an LRU cache, CRezFileMgr).
// Four methods at 0x13cc00/0x13cca0/0x13cdc0/0x13ce70: a stream Read, its Write
// counterpart, the lazy (re)open and the gated Close. See include/Rez/RezFile.h
// for the layout + the call-graph evidence. Distinct from CRezItm (RezMgr.h): this
// node keeps the filename @+0x10, the FILE* @+0x14 and the manager @+0x18.
#include <rva.h>

#include <Rez/RezFile.h>

// CloseAllOpen (0x13ca80): drain the open-handle list. The list head sits at
// CRezFileMgr+0x14 (m_openList.m_head); Close() each file until the list empties
// (each Close moves the node off the open list, advancing the head). Returns 1.
RVA(0x0013ca80, 0x1d)
i32 CRezFileMgr::CloseAllOpen() {
    // The list stores CRezFile nodes; retrieve the head as its concrete type (the
    // typed intrusive-list access - CRezFile's node base is at offset 0, so this is a
    // zero-offset static downcast, matching-neutral). Close() is a direct call.
    while (m_openList.m_head != 0) {
        ((CRezFile*)m_openList.m_head)->Close();
    }
    return 1;
}

// Read (0x13cc00): ensure the handle is open, seek to `pos` (retrying through the
// manager's gate), then fread `count` bytes into buf (retrying short reads through
// the same gate). Returns the bytes read (== count) or 0. `a` is the base-class
// signature's unused leading param. The `count <= 0` guard (unsigned) lowers to
// retail's `test;jbe` (docs/patterns/unsigned-zero-guard-le-not-eq.md).
RVA(0x0013cc00, 0x9f)
i32 CRezFile::Read(i32 a, i32 pos, u32 count, void* buf) {
    (void)a;
    if (count <= 0) {
        return 0;
    }
    if (m_handle == 0) {
        Open();
    }
    while (RezFSeek(m_handle, pos, 0) != 0) {
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
    }
    u32 got = RezFRead(buf, 1, count, m_handle);
    while (got != count) {
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
        got = RezFRead(buf, 1, count, m_handle);
    }
    return got;
}

// Write (0x13cca0): the write counterpart of Read (RezFWrite for RezFRead); same
// open/seek/retry gating. Returns the bytes written (== count) or 0.
RVA(0x0013cca0, 0x9f)
i32 CRezFile::Write(i32 a, i32 pos, u32 count, void* buf) {
    (void)a;
    if (count <= 0) {
        return 0;
    }
    if (m_handle == 0) {
        Open();
    }
    while (RezFSeek(m_handle, pos, 0) != 0) {
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
    }
    u32 put = RezFWrite(buf, 1, count, m_handle);
    while (put != count) {
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
        put = RezFWrite(buf, 1, count, m_handle);
    }
    return put;
}

// Flush (0x13cd60): fflush the handle, retrying through the manager's gate on
// failure. Returns 1 (no handle / flushed) or 0 (the gate gave up). Same clean
// `(fflush == 0)` neg/sbb/inc bool-normalize as Close (no list/handle teardown).
RVA(0x0013cd60, 0x49)
i32 CRezFile::Flush() {
    if (m_handle != 0) {
        i32 ok = (Eng_fflush(m_handle) == 0);
        while (!ok) {
            if (m_mgr->m_gate->Retry() == 0) {
                return 0;
            }
            ok = (Eng_fflush(m_handle) == 0);
        }
        return ok;
    }
    return 1;
}

// Open (0x13cdc0): lazily (re)open the handle. If already open, return 1. If the
// cache is over its cap, evict the LRU (the open-list tail) via Close(). Then fopen
// with the flag-selected mode ("w+b"/"rb"/"r+b"), retrying through the manager's
// gate on failure; on success move the node from the closed list to the open list
// and bump the open count. Returns 1 (opened) / 0 (gave up or an invalid w+ro mix).
RVA(0x0013cdc0, 0xad)
i32 CRezFile::Open() {
    if (m_handle != 0) {
        return 1;
    }
    if (m_mgr->m_openCount > m_mgr->m_maxOpen) {
        // Typed intrusive-list access: the LRU eviction candidate (the open list's
        // tail) is a CRezFile (zero-offset static downcast; see CloseAllOpen).
        CRezFile* lru = (CRezFile*)m_mgr->m_openList.m_tail;
        if (lru != 0) {
            lru->Close();
        }
    }
    for (;;) {
        if (m_mgr->m_write) {
            if (m_mgr->m_readonly) {
                return 0;
            }
            m_handle = Eng_fopen(m_name, s_wPlusB);
        } else if (m_mgr->m_readonly) {
            m_handle = Eng_fopen(m_name, s_rb);
        } else {
            m_handle = Eng_fopen(m_name, s_rPlusB);
        }
        if (m_handle != 0) {
            break;
        }
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
        if (m_handle != 0) {
            break;
        }
    }
    m_mgr->m_closedList.Remove(this);
    m_mgr->m_openList.AddHead(this);
    m_mgr->m_openCount++;
    return 1;
}

// Close (0x13ce70): fclose the handle (retrying through the manager's gate); then
// drop the open count, move the node back to the closed list, and null the handle.
// Returns 1 (no handle / closed) or 0 (the gate gave up). BYTE-EXACT (100%): the
// anticipated esi<->edi coin-flip did not materialize here - MSVC5 pins the same
// registers as retail, so this method needs no early-stop marker.
RVA(0x0013ce70, 0x7c)
i32 CRezFile::Close() {
    if (m_handle == 0) {
        return 1;
    }
    i32 ok = (RezFClose(m_handle) == 0);
    while (!ok) {
        if (m_mgr->m_gate->Retry() == 0) {
            return 0;
        }
        ok = (RezFClose(m_handle) == 0);
    }
    m_mgr->m_openCount--;
    m_mgr->m_openList.Remove(this);
    m_mgr->m_closedList.AddHead(this);
    m_handle = 0;
    return ok;
}
