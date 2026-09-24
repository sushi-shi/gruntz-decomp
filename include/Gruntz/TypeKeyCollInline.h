#ifndef GRUNTZ_GRUNTZ_TYPEKEYCOLLINLINE_H
#define GRUNTZ_GRUNTZ_TYPEKEYCOLLINLINE_H

#include <Gruntz/TypeKeyColl.h>
#include <Wap32/zBitVec.h>

static inline CString* TypeResolve(i32 key) {
    g_typeColl.m_grown = 0;
    if (key >= g_typeColl.m_lo && key <= g_typeColl.m_hi) {
        return g_typeColl.Elem(key);
    }
    if ((static_cast<_zvec*>(&g_typeColl))->GrowTo(key, 0) != NULL) {
        return g_typeColl.Elem(key);
    }
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetRetAddr();
    g_typeColl.m_errSink->Set(&g_typeColl, msg, 0xc);
    return g_typeColl.Scratch();
}

static inline void FreeNodes() {
    CString* nodes = g_typeColl.Slots();
    i32 cnt = g_typeColl.m_grown;
    while (cnt-- != 0) {
        if (nodes != NULL) {
            nodes->CString::CString();
        }
        ++nodes;
    }
}

#endif // GRUNTZ_GRUNTZ_TYPEKEYCOLLINLINE_H
