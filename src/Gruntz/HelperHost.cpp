// HelperHost.cpp - the two non-virtual CDDrawWorker helpers Ghidra grouped under a
// made-up class "HelperHost" (0x164790 / 0x166040). They are really the worker
// hierarchy's reset/arm helper (CDDrawWorkerBase::Helper_164790, inherited by both
// subtypes) and CDDrawWorkerB's named-object frame fetch
// (CDDrawWorkerB::Helper_166040), so they live on the real classes from
// <DDrawMgr/CDDrawWorkerNode.h> - no separate "HelperHost" object. Both hang off the
// worker's +0x0c owner context (CDDrawWorkerCtx). Self-contained except the MFC
// CMapStringToOb::Lookup (0x1b8008) named-object lookup in Helper_166040. Names are
// placeholders; offsets + code bytes are load-bearing.
#include <DDrawMgr/CDDrawWorkerNode.h>

#include <Mfc.h> // real MFC CMapStringToOb / CObject (Lookup 0x1b8008, reloc-masked)
#include <Ints.h>
#include <rva.h>

// The worker's +0x0c owner context (CDDrawWorkerCtx): a sub-manager ptr at +0x10
// (whose +0x10 is the string->object map) and an int at +0x24 (primes m_3c).
struct CDDrawWorkerCtxMap {
    char pad_00[0x10];
    CMapStringToOb m_10; // +0x10  named-object map
};
struct CDDrawWorkerCtx {
    char pad_00[0x10];
    CDDrawWorkerCtxMap* m_10; // +0x10
    char pad_14[0x24 - 0x14];
    i32 m_24; // +0x24
};

// The object Lookup yields, viewed as a bounded element array.
struct CDDrawWorkerObj {
    char pad_00[0x14];
    void** m_14; // +0x14  element array
    char pad_18[0x64 - 0x18];
    i32 m_64; // +0x64  lo index
    i32 m_68; // +0x68  hi index
};

// ===========================================================================
// reset/arm the helper from (a, b); seeds m_3c off the owner context.
// ===========================================================================
// @early-stop
// regalloc/scheduling wall (topic:regalloc): logic + every member store are
// byte-exact, but retail parks the m_ctx context ptr in eax (immediate stores for
// m_48/m_50, a late `mov eax,1` for the return) while cl parks it in edx and
// reuses one `mov eax,1` for both m_50 and the return. A pure eax<->edx coin-flip
// over the trailing constant; no source lever flips it (hoisting the ctx read,
// reordering the stores - all no-change at the same plateau). ~90%.
RVA(0x00164790, 0x41)
i32 CDDrawWorkerBase::Helper_164790(i32 a, i32 b) {
    m_5c = a;
    m_10 = 0;
    m_14 = 0;
    m_40 = 0;
    m_44 = 0;
    m_4c = 0;
    m_58 = 0;
    m_60 = b;
    m_48 = 0x32;
    m_50 = 1;
    m_3c = m_ctx->m_24;
    return 1;
}

// ===========================================================================
// look up a named object in the owner's map, then fetch element[idx]
// when in range; cache it at m_78 and return whether it is non-null.
// ===========================================================================
// @early-stop
// scheduling wall (topic:regalloc): body byte-exact (same no-edi regalloc, the
// tail-duplicated v!=0 epilogue, the bounds dispatch). Only residue is WHERE the
// Lookup out-param zero-init lands - retail emits it after both arg pushes, cl
// emits it between them (a 1-instruction reorder). Not source-steerable. ~95%.
RVA(0x00166040, 0x66)
i32 CDDrawWorkerB::Helper_166040(i32 key, i32 idx) {
    CObject* obj = 0;
    m_ctx->m_10->m_10.Lookup((const char*)key, obj);
    CDDrawWorkerObj* p = (CDDrawWorkerObj*)obj;
    i32 v;
    if (p != 0 && idx >= p->m_64 && idx <= p->m_68) {
        v = (i32)p->m_14[idx];
    } else {
        v = 0;
    }
    m_78 = v;
    return v != 0;
}

SIZE_UNKNOWN(CDDrawWorkerCtx);
SIZE_UNKNOWN(CDDrawWorkerCtxMap);
SIZE_UNKNOWN(CDDrawWorkerObj);
