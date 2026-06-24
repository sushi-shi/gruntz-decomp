// SymTab.cpp - CSymTab, the Remus/ButeMgr hierarchical symbol table (the tree of
// named scopes the .bute parser builds). Recovered from the this/ecx trace group
// "ClassUnknown_12" (4 methods at 0x139ee0/0x13a230/0x13bae0/0x13be40), all on a
// single object shape with two embedded engine hash tables at +0x38/+0x40 and the
// owning parser at +0x18. See include/Bute/SymTab.h for the full layout.
#include <rva.h>

#include <Bute/SymTab.h>

// The tokenizer's "is this character part of a token" predicate, inlined at each
// scan site. When the parser supplies a delimiter set, a token char is one NOT in
// it (strchr == 0); otherwise the default identifier classes apply: printable
// punctuation/space .. '.', digits, upper, lower. The `== 0` coercion emits the
// neg/sbb/inc int->bool normalize that retail uses (docs/patterns/
// int-to-bool-normalize.md).
static i32 IsTokenChar(const char* delims, char ch) {
    if (delims) {
        return strchr(delims, ch) == 0;
    }
    if (ch >= ' ' && ch <= '.') {
        return 1;
    }
    if (ch >= '0' && ch <= '9') {
        return 1;
    }
    if (ch >= 'A' && ch <= 'Z') {
        return 1;
    }
    if (ch >= 'a' && ch <= 'z') {
        return 1;
    }
    return 0;
}

// ~CSymTab (0x139ee0): tear down the scope tree. Walk the leaf-symbol table
// (m_symbols, +0x40) clearing+freeing each record, then the child-scope table
// (m_subTabs, +0x38) recursing ~CSymTab on each, then free the owned buffers and
// null the fields. The two CHashTable members auto-destruct after the body, in
// reverse declaration order (m_symbols then m_subTabs) at descending trylevels --
// the /GX member-teardown frame (docs/patterns/eh-dtor-model-members-as-
// destructible.md).
RVA(0x00139ee0, 0x11e)
CSymTab::~CSymTab() {
    CHashEntry* cur;
    for (cur = m_symbols.First(); cur != 0;) {
        CHashEntry* next = m_symbols.Next(cur);
        m_symbols.Remove(cur);
        CSymRec* rec = (CSymRec*)cur->m_payload;
        if (rec) {
            rec->Clear();
            RezFree(rec);
        }
        cur = next;
    }
    for (cur = m_subTabs.First(); cur != 0;) {
        CHashEntry* next = m_subTabs.Next(cur);
        m_subTabs.Remove(cur);
        CSymTab* sub = (CSymTab*)cur->m_payload;
        if (sub) {
            sub->~CSymTab();
            RezFree(sub);
        }
        cur = next;
    }
    if (m_name) {
        RezFree(m_name);
    }
    if (m_buf48) {
        RezFree(m_buf48);
    }
    m_name = 0;
    m_rec = 0;
    m_08 = 0;
    m_04 = 0;
    m_10 = 0;
    m_0c = 0;
    m_buf48 = 0;
    m_owner = 0;
    m_1c = 0;
    m_34 = 0;
    // m_symbols, m_subTabs destruct here (reverse decl order, /GX trylevels).
}

// FindSub (0x13a230): look up `name` in the child-scope table (m_subTabs, +0x38),
// forwarding `m_owner->m_68 == 0` as the walk's flag.
RVA(0x0013a230, 0x29)
void* CSymTab::FindSub(const char* name) {
    if (!name) {
        return (void*)name;
    }
    return m_subTabs.Walk(name, m_owner->m_68 == 0);
}

// @early-stop
// recursive path tokenizer; the inlined IsTokenChar (3x) + the working-pointer arg
// reuse + the FindSub recursion schedule against a documented regalloc/scheduling
// wall. Logic complete; byte-match parked for the final sweep.
RVA(0x0013bae0, 0x1b9)
void* CSymTab::ResolvePath(const char* path) {
    char buf[0x30];
    const char* p = path;
    if ((i32)strlen(path) > 1) {
        if (!IsTokenChar(m_owner->m_delims, *p)) {
            ++p;
            path = p;
        }
    }
    i32 n = 0;
    while (IsTokenChar(m_owner->m_delims, *p)) {
        buf[n] = *p;
        ++n;
        ++p;
    }
    buf[n] = 0;
    void* sub = FindSub(buf);
    if (!sub) {
        return sub;
    }
    char c = path[n];
    if (c == 0) {
        return sub;
    }
    while (!IsTokenChar(m_owner->m_delims, c)) {
        c = path[n + 1];
        ++n;
        if (c == 0) {
            return sub;
        }
    }
    return ((CSymTab*)sub)->ResolvePath(path + n);
}

// @early-stop
// last-delimiter split + scope resolve; inlined IsTokenChar + the rep-movs token
// copy + the SymTab_InsertResolved tail. Logic complete; byte-match parked.
RVA(0x0013be40, 0x1ac)
i32 CSymTab::ResolveQualified(const char* name, void* arg) {
    char qual[0x100];
    char key[0x24];
    const char* p = name;
    i32 len = (i32)strlen(name);
    if (len > 1) {
        if (!IsTokenChar(m_owner->m_delims, *p)) {
            ++p;
            --len;
        }
    }
    i32 i = len - 1;
    while (!IsTokenChar(m_owner->m_delims, p[i])) {
        --i;
        if (i < 0) {
            break;
        }
    }
    if (i == len) {
        return 0;
    }
    const char* tail = p + i + 1;
    strncpy(qual, tail, strlen(tail) + 1);
    if (i <= 0) {
        return Insert(qual, arg);
    }
    strncpy(key, p, (u32)i);
    key[i] = 0;
    CSymTab* scope = (CSymTab*)ResolvePath(key);
    if (!scope) {
        return 0;
    }
    return scope->Insert(qual, arg);
}
