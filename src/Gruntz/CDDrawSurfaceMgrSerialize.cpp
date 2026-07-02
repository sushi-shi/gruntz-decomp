// CDDrawSurfaceMgrSerialize.cpp - CDDrawSurfaceMgr::SnapshotChildren (0x156020),
// the 1285-B child blit-param serializer/dispatcher carved out of the discovered
// backlog. Split into its own /GX ("eh") TU because the function is a deep EH
// state machine (~14 unwind states over a CFile-backed serializer temp + an
// inline strlen/strcpy + a CTime header), which a base-flagged TU would un-frame.
//
// `this` (CDDrawSurfaceMgr) is reached at ebp: the run-callback is stored into
// m_3c (+0x3c) on entry; the dispatch walks the m_08 (Hermiona, +0x08) and m_24
// (Remus, +0x24) children through five blit modes (push 1/3/4/5), invoking the
// optional run-callback (m_3c) before each child op.
//
// The serializer temp S (at [esp+0x10]) is a CFile-backed stream: a name CString
// @+0x0c, an embedded CFileIO stream @+0x10 (built by a separate ctor call), and
// read/write byte cursors @+0x20/+0x24. The 0x120-B header record (CTime stamp +
// the inline-strcpy name) is the local right after it @[esp+0x3c]. Field/method
// names are placeholders; OFFSETS, vtable slots, sizes, store order and the
// ordered call sequence are load-bearing. Engine callees are reloc-masked
// external __thiscall/__cdecl.
//
// @early-stop
// big-SEH wall (~74.6% fuzzy; docs/patterns/gx-state-machine-scalar-delete-cleanup.md
// + big-seh-fuzzy-desync.md + eh-state-numbering-base.md): a 1285-B /GX function
// with a multi-way fall-through reject ladder over a CString/CFile serializer temp.
// The whole carcass (every offset, the embedded-stream Init, the CTime header
// build, the inline strcpy + Probe result, the ordered child-op call sequence) is
// reproduced, but at EACH reject retail destroys the serializer temp via the
// scalar-deleting destructor with a re-stamped vtable (mov [esp+0x10],0x5efe30;
// call ds:0x5efe3c) + an even/odd __ehfuncinfo state pair before a shared ~T tail,
// while idiomatic C++ scope-exit emits only the simple ~Serializer per return — so
// the long fail ladder desyncs. Also the success path closes via 0x157980 (not the
// simple dtor) and cb is pinned in esi (retail) vs ebx (ours). Not source-steerable;
// deferred to the final sweep once the serializer class + child blit-op classes are
// fully modeled for a leaf-first redo.

#include <rva.h>
#include <Mfc.h>
#include <Globals.h>

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr (partial view): only the members these two methods touch.
typedef i32(__cdecl* SnapRunCallback)(void* mgr, void* ser, i32 mode, i32, i32);

class CDDrawSurfaceMgr {
public:
    i32 SnapshotChildren(SnapRunCallback cb, i32 arg1, char* name, i32 arg3);
    // The load counterpart (0x156530): same Serializer temp + child-op ladder,
    // but reads the stream back (Read instead of Write) and replays modes 2/6/7/8.
    i32 RestoreChildren(SnapRunCallback cb, char* name, i32 arg3);

    char m_pad00[0x08]; // +0x00..+0x07 (vptr + slot)
    void* m_08;         // +0x08  Hermiona child (the m_08 blit-op target)
    char m_pad0c[0x24 - 0x0c];
    void* m_24; // +0x24  Remus child (the m_24 blit-op target = CGameLevel)
    char m_pad28[0x3c - 0x28];
    SnapRunCallback m_3c; // +0x3c  run-callback
};

// ---------------------------------------------------------------------------
// The embedded CFileIO stream at Serializer+0x10. Its ctor (0x1befd7) is invoked
// on &S+0x10 ([esp+0x20]); modeled so calling it through the member gives that
// this-adjust with no cast.
struct SnapStream {
    void Init(); // 0x1befd7  CFileIO()
    char m_pad[0x10];
};

// The CFile-backed serializer temp S (at [esp+0x10]). Modeled as a real class so
// `mov ecx,&S; call` falls out with no casts; every method is a reloc-masked
// external __thiscall. The own ctor (0x157850) builds the name CString + stamps
// the vtbl; the embedded stream @+0x10 is then built by a separate Init() call.
class Serializer {
public:
    Serializer();                          // 0x157850 own ctor (vtbl + name CString)
    ~Serializer();                         // 0x1578b0 non-deleting dtor (failure teardown)
    void Reset();                          // 0x157a50 init/reset (vtbl=live, empty name)
    void Close();                          // 0x157980 close stream + destroy (success teardown)
    i32 SetSink(void* sink, i32 a, i32 b); // 0x165e30
    i32 Begin();                           // 0x165e60  open the stream
    void Read(void* buf, i32 len);         // 0x165f00  (load path)
    i32 Write(void* buf, i32 len);         // 0x165f50
    i32 End();                             // 0x165ef0  flush the stream

    char m_pad00[0x10];  // +0x00..+0x0f (vtbl, ints, name CString)
    SnapStream m_stream; // +0x10..+0x1f
    char m_pad20[0x0c];  // +0x20..+0x2b (cursors + tail)
};

// The child blit-op targets. m_08 child (Hermiona) carries ops 0x15abc0 /
// 0x15acb0 / 0x15ac20 / 0x15b020; the m_24 child (Remus) carries 0x160f70.
struct HermionaChild {
    i32 Probe();                                 // 0x15abc0  (__thiscall, no arg)
    i32 BlitA(Serializer* s, i32 arg);           // 0x15acb0
    i32 BlitB(Serializer* s, i32 mode, i32 arg); // 0x15ac20
    i32 BlitC(Serializer* s, i32 arg);           // 0x15b020
    void Refresh();                              // 0x159ef0  (load path, no arg)
    i32 LoadA(Serializer* s, i32 n, i32 arg);    // 0x15ad30  (load path)
    i32 LoadB(Serializer* s, i32 n, i32 arg);    // 0x15b0e0  (load path)
};
struct RemusChild {
    i32 BlitD(Serializer* s, i32 mode, i32 a, i32 b); // 0x160f70
    i32 MainPlaneQueryB();                            // 0xcee10 (load success tail)
};

// CTime helpers come from MFC (<Mfc.h>): CTime::CTime() (0x1b30b1) and
// CTime::GetLocalTm(struct tm*) (0x1b30f0).

// The global record id mirrored into the serialized header (header[0x114]).

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::SnapshotChildren (0x156020, __thiscall, /GX)
RVA(0x00156020, 0x505)
i32 CDDrawSurfaceMgr::SnapshotChildren(SnapRunCallback cb, i32 arg1, char* name, i32 arg3) {
    if (cb == 0) {
        return 0;
    }
    m_3c = cb;

    Serializer S;
    S.m_stream.Init();
    S.Reset();

    if (S.SetSink((void*)cb, 0, 0) == 0) {
        return 0;
    }
    if (S.Begin() == 0) {
        return 0;
    }

    // Build the 0x120-byte header record (CTime stamp + the name strcpy).
    CTime now;
    char header[0x120];
    memset(header, 0, sizeof(header));
    *(i32*)(header + 0x04) = 1;
    *(i32*)(header + 0x08) = now.GetLocalTm(0)->tm_mon + 1;
    *(i32*)(header + 0x0c) = now.GetLocalTm(0)->tm_mday;
    *(i32*)(header + 0x0c) = now.GetLocalTm(0)->tm_year + 0x76c;
    strcpy(header + 0x10, name);
    i32 probe = ((HermionaChild*)m_08)->Probe();
    *(u32*)(header + 0x114) = g_61ab14;
    *(i32*)(header + 0x118) = probe;
    S.Write(header, 0x120);

    // ---- dispatch the five blit modes over the children ----
    if (m_3c && cb(this, &S, 1, 0, 0) == 0) {
        return 0;
    }
    if (((HermionaChild*)m_08)->BlitA(&S, arg3) == 0) {
        return 0;
    }
    if (m_3c && cb(this, &S, 3, 0, 0) == 0) {
        return 0;
    }
    if (((HermionaChild*)m_08)->BlitB(&S, 3, arg3) == 0) {
        return 0;
    }
    if (((RemusChild*)m_24)->BlitD(&S, 3, 0, 0) == 0) {
        return 0;
    }
    if (m_3c && cb(this, &S, 4, 0, 0) == 0) {
        return 0;
    }
    if (((HermionaChild*)m_08)->BlitC(&S, arg3) == 0) {
        return 0;
    }
    if (((RemusChild*)m_24)->BlitD(&S, 4, 0, 0) == 0) {
        return 0;
    }
    if (m_3c && cb(this, &S, 5, 0, 0) == 0) {
        return 0;
    }
    if (((HermionaChild*)m_08)->BlitB(&S, 5, arg3) == 0) {
        return 0;
    }
    if (((RemusChild*)m_24)->BlitD(&S, 5, 0, 0) == 0) {
        return 0;
    }

    S.End();
    S.Close();
    return 1;
}

// ---------------------------------------------------------------------------
// CDDrawSurfaceMgr::RestoreChildren (0x156530, __thiscall, /GX) - the load
// counterpart of SnapshotChildren. Opens the same CFileMem-backed serializer over
// `name`, reads back the 0x120-byte header (publishing header[0x114] -> g_61ab14),
// then replays the run-callback (m_3c, REQUIRED here - a null m_3c rejects) and the
// child load-ops over the m_08 (Hermiona) + m_24 (Remus/CGameLevel) children for
// modes 2/6/7/8. Success closes via End()/MainPlaneQueryB()/Close(). Field/method
// names are placeholders; OFFSETS, vtable slots, sizes, store order and the ordered
// call sequence are load-bearing. Engine callees are reloc-masked external.
//
// @early-stop
// big-SEH wall (same as SnapshotChildren above; docs/patterns/big-seh-fuzzy-desync.md
// + gx-state-machine-scalar-delete-cleanup.md + eh-state-numbering-base.md): a 1367-B
// /GX function with a multi-way fall-through reject ladder over the CFileMem serializer
// temp. The whole carcass (every offset, the embedded-stream Init, the 0x120 header
// Read, the g_61ab14 publish, the ordered child load-op call sequence, the inline-vs-
// out-of-line ~Serializer split) is reproduced, but at each reject retail destroys the
// temp via the re-stamped scalar-deleting vtable (mov [esp+0xc],0x5efe30; call
// ds:0x5efe3c) under an even/odd __ehfuncinfo state pair before a shared ~T tail, while
// idiomatic scope-exit C++ emits the simple dtor per return -> the long fail ladder
// desyncs and the trylevel state numbers diverge. Not source-steerable; deferred to the
// final sweep once the serializer + child classes are fully modeled (leaf-first redo).
RVA(0x00156530, 0x557)
i32 CDDrawSurfaceMgr::RestoreChildren(SnapRunCallback cb, char* name, i32 arg3) {
    if (name == 0) {
        return 0;
    }
    m_3c = cb;

    Serializer S;
    S.m_stream.Init();
    S.Reset();

    if (S.SetSink(name, 1, 0) == 0) {
        return 0;
    }
    if (S.Begin() == 0) {
        return 0;
    }

    char header[0x120];
    S.Read(header, 0x120);

    if (m_3c == 0 || m_3c(this, &S, 2, arg3, (i32)header) == 0) {
        return 0;
    }
    g_61ab14 = *(u32*)(header + 0x114);
    ((HermionaChild*)m_08)->Refresh();
    if (((HermionaChild*)m_08)->LoadA(&S, *(i32*)(header + 0x110), arg3) == 0) {
        return 0;
    }
    if (m_3c == 0 || m_3c(this, &S, 6, arg3, (i32)header) == 0) {
        return 0;
    }
    if (((HermionaChild*)m_08)->BlitB(&S, 6, arg3) == 0) {
        return 0;
    }
    if (((RemusChild*)m_24)->BlitD(&S, 6, 0, 0) == 0) {
        return 0;
    }
    if (m_3c == 0 || m_3c(this, &S, 7, arg3, (i32)header) == 0) {
        return 0;
    }
    if (((HermionaChild*)m_08)->LoadB(&S, *(i32*)(header + 0x110), arg3) == 0) {
        return 0;
    }
    if (((RemusChild*)m_24)->BlitD(&S, 7, 0, 0) == 0) {
        return 0;
    }
    if (m_3c == 0 || m_3c(this, &S, 8, arg3, (i32)header) == 0) {
        return 0;
    }
    if (((HermionaChild*)m_08)->BlitB(&S, 8, arg3) == 0) {
        return 0;
    }
    if (((RemusChild*)m_24)->BlitD(&S, 8, 0, 0) == 0) {
        return 0;
    }

    S.End();
    ((RemusChild*)m_24)->MainPlaneQueryB();
    S.Close();
    return 1;
}

SIZE_UNKNOWN(RemusChild);
SIZE_UNKNOWN(Serializer);
SIZE_UNKNOWN(SnapStream);
