#include <Enums.h>
#include <Gruntz/ProjActCache.h>
#include <Gruntz/UserLogic.h>
#include <Mfc.h>
#include <Bute/ButeTree.h>
#include <Bute/PTreeNode.h>
#include <Bute/ButeStore.h>
#include <Wap32/zBitVec.h>
#include <Gruntz/UserBaseLink.h>
#include <rva.h>
#include <AddrWord.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#undef isspace
#undef isdigit
#pragma function(memcpy)

#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/TypeKeyCollStr.h>
#include <Gruntz/XferArchive.h>
#include <Wap32/ZVec.h>
#include <Utils/BitArrayWord.h>

DATA(0x002bf428)
void* g_retAddrBreadcrumb;

DATA(0x002bf400)
i32 g_helperRefCount;

inline CTypeCollRuntime::CTypeCollRuntime()

    : _zdvec(sizeof(CString), 0x7d0, 0x7da, ZVecNoScratch()) {
    CString* item = Slots();
    i32 count = m_grown;
    if (item != NULL && count != 0) {
        do {
            item->CString::CString();
            ++item;
        } while (--count);
    }
}

CTypeCollRuntime::~CTypeCollRuntime() {
    CString* item = Elem(m_lo);
    i32 count = m_hi - m_lo + 1;
    if (item != NULL && count != 0) {
        do {
            item->CString::~CString();
            ++item;
        } while (--count);
    }
}

DATA(0x002bf650)
CTypeCollRuntime g_typeColl;

VTBL(zBitVec, 0x001f04c8);
VTBL(zErrHandling, 0x001f04cc);
VTBL(_zdvec, 0x001f04d0);
VTBL(_zvec, 0x001f04d4);
VTBL(CTypeCollRuntime, 0x001f04e4);

VTBL(zPtrColl, 0x001f04d8);
// Interior fields of one CActReg; do not define overlapping globals.

DATA(0x0021ad28)
i32 g_defaultProjActSize;

DATA(0x0021adf4)
const char s_out_of_memory[] = "out of memory";

DATA(0x002bf498)
TypeKeyRec g_recs23[32];
DATA(0x002bf618)
i32 g_recCount23;

DATA(0x002bf408)
CVariantSlot g_zBitSetErrorSlot("zBitSet: ");

DATA(0x002bf430)
CVariantSlot g_globalErrorSlot("Global Error: ");

DATA(0x002bf448)
char* g_errDataInvalid;
DATA(0x002bf44c)
char* g_errOverflow;
DATA(0x002bf450)
char* g_errOutOfRange;
DATA(0x002bf454)
char* g_errNullArg;
DATA(0x002bf458)
char* g_errExists;
DATA(0x002bf45c)
char* g_errBadArg;
DATA(0x002bf460)
char* g_errNoFile;
DATA(0x002bf464)
char* g_errOutOfMem;

DATA(0x002bf468)
CVariantSlot g_dynamicArrayErrorSlot("Dynamic Array: ");

DATA(0x002bf480)
CVariantSlot g_symTabErrorSlot("zSymTab: ");

// @early-stop
RVA(0x0016d190, 0x101)
void* zPTree::Find(const char* key) {
    if (key == NULL) {
        char* msg = g_errNullArg;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0x16);
        return 0;
    }
    CButeTreeNode* root = m_root;
    m_descentCursor = root;
    m_candidateLeaf = NULL;
    m_lookupPending = 1;
    i32 bitmax = static_cast<i32>(strlen(key)) * 8 + 7;
    m_keyBitLength = bitmax;
    if (root == NULL) {
        return 0;
    }
    i32 b = root->m_bit;
    while (b <= bitmax) {
        CButeTreeNode** slot = m_descentCursor->m_child;
        if (key[b >> 3] & (1 << (b & 7))) {
            ++slot;
        }
        CButeTreeNode* child = *slot;
        m_candidateLeaf = child;
        if (child == NULL) {
            return 0;
        }
        if (child->m_bit <= b) {
            if (strcmp(key, child->m_key) == 0) {
                m_lookupPending = 0;
                return m_candidateLeaf->m_value;
            }
            return 0;
        }
        m_descentCursor = child;
        b = child->m_bit;
    }
    m_candidateLeaf = m_descentCursor;
    return 0;
}

RVA(0x0016d2a0, 0x26)
zBitVec::~zBitVec() {
    if (static_cast<u32>(m_capacity) > 0x20) {
        free(m_words);
    }
}

// @early-stop
RVA_COMPGEN(0x0016d2d0, 0x1e, ??_GzBitVec@@UAEPAXI@Z)
RVA(0x0016d2f0, 0xac)
zBitVec& zBitVec::operator=(const zBitVec& that) {
    if (this != &that) {
        if (m_capacity != that.m_capacity) {
            if (static_cast<u32>(m_capacity) > 0x20) {
                ::operator delete(m_words);
            }
            if (static_cast<u32>(that.m_capacity) > 0x20) {
                m_words = static_cast<u32*>(
                    malloc((static_cast<u32>(that.m_capacity) >> BITARRAY_WORD_SHIFT) * 4)
                );
                if (!m_words) {
                    char* msg = g_errOutOfMem;
                    g_retAddrBreadcrumb = GetCallerRetAddr();
                    m_errSink->Set(this, msg, 0xc);
                    m_capacity = 0x20;
                    return *this;
                }
            }
            m_capacity = that.m_capacity;
        }
        const u32* src = (static_cast<u32>(that.m_capacity) > 0x20) ? that.m_words : &that.m_inline;
        u32* dst = (static_cast<u32>(m_capacity) > 0x20) ? m_words : &m_inline;
        memcpy(dst, src, static_cast<u32>(m_capacity) >> 3);
    }
    return *this;
}

// @early-stop
RVA(0x0016d3a0, 0x344)
zBitVec::zBitVec(const char* tokens, i32 minSize) : zErrHandling(&g_zBitSetErrorSlot) {
    i32 maxv = 0;
    const char* start;
    const char* q;
    if (tokens == NULL) {
        char* msg = g_errNullArg;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0x16);
        return;
    }
    if (minSize == 0) {
        minSize = g_defaultProjActSize;
    }

    const char* p = tokens;
    if (isspace(*p)) {
        do {
            ++p;
        } while (isspace(*p));
    }
    if (*p == 0) {
        if (!SetSize(minSize)) {
            goto oom;
        }
        return;
    }
    if (!isdigit(*p)) {
        goto badchar;
    }

    start = p;
    while (*p != 0) {
        i32 v = 0;
        i32 sawSep = 0;
        while (isdigit(*p)) {
            v = v * 10 + (*p - '0');
            ++p;
        }
        if (static_cast<u32>(v) > static_cast<u32>(maxv)) {
            maxv = v;
        }
        while (*p != 0) {
            if (isdigit(*p)) {
                break;
            }
            if (sawSep && *p != ' ') {
                goto badchar;
            }
            if (strchr(" ,-", *p) == NULL) {
                goto badchar;
            }
            if (*p != ' ') {
                sawSep = 1;
            }
            ++p;
        }
    }

    if (static_cast<u32>(minSize) > static_cast<u32>(maxv)) {
        maxv = minSize;
    }
    if (!SetSize(maxv)) {
        goto oom;
    }

    q = start;
    while (*q != 0) {
        i32 v = 0;
        while (isdigit(*q)) {
            v = v * 10 + (*q - '0');
            ++q;
        }
        {
            u32* band = (static_cast<u32>(m_capacity) > 0x20) ? m_words : &m_inline;
            band[static_cast<u32>(v) >> BITARRAY_WORD_SHIFT] |= 1u << (v & BITARRAY_BIT_MASK);
        }
        if (*q == 0) {
            break;
        }
        if (isspace(*q)) {
            while (isspace(q[1])) {
                ++q;
            }
        }
        char sep = *q;
        ++q;
        if (isspace(*q)) {
            while (isspace(q[1])) {
                ++q;
            }
        }
        if (sep == '-') {
            i32 v2 = 0;
            while (isdigit(*q)) {
                v2 = v2 * 10 + (*q - '0');
                ++q;
            }
            if (v > v2) {
                i32 t = v;
                v = v2;
                v2 = t;
            }
            for (i32 b = v + 1; b <= v2; ++b) {
                u32* band = (static_cast<u32>(m_capacity) > 0x20) ? m_words : &m_inline;
                band[static_cast<u32>(b) >> BITARRAY_WORD_SHIFT] |= 1u << (b & BITARRAY_BIT_MASK);
            }
            while (*q != 0 && !isdigit(*q)) {
                ++q;
            }
        }
    }
    return;

oom: {
    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, msg, 0xc);
    return;
}
badchar: {
    char* msg = g_errBadArg;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, msg, 0x16);
    return;
}
}

inline zBitVec::zBitVec() : zErrHandling(&g_zBitSetErrorSlot) {
    if (!SetSize(g_defaultProjActSize)) {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0xc);
    }
}

RVA(0x0016d710, 0x76)
CUserBaseLink::CUserBaseLink() {}

RVA(0x0016d790, 0xb1)
zBitVec::zBitVec(i32 idx, i32 sizehint) : zErrHandling(&g_zBitSetErrorSlot) {
    u32 n = static_cast<u32>(sizehint);
    if (n == 0) {
        n = static_cast<u32>(g_defaultProjActSize);
    }
    if (static_cast<u32>(idx) >= n) {
        n = static_cast<u32>(idx) + 1;
    }
    if (!SetSize(static_cast<i32>(n))) {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0xc);
    } else {
        u32* base = (static_cast<u32>(m_capacity) > 0x20) ? m_words : &m_inline;
        u32* slot = base + (static_cast<u32>(idx) >> BITARRAY_WORD_SHIFT);
        *slot |= 1u << (idx & BITARRAY_BIT_MASK);
    }
}

RVA(0x0016d850, 0x11e)

void CVariantSlot::Set(void* key, void* name, i32 value) {
    if (m_typeTag == 4) {
        m_valueWord = static_cast<u16>(value);
        return;
    }
    i32 idx;
    if (g_recCount23 != 0) {

        AddrWord<char> k;
        k.m_addr = static_cast<char*>(key);
        idx = this->Find(k.m_word);
    } else {
        idx = -1;
    }
    if (idx == -1) {
        if (m_typeTag == 2) {

            char buf[0xa0];
            strcpy(buf, m_label);

            strncat(buf, static_cast<const char*>(name), 0x4f);
            m_callback(buf, value);
        } else if (m_typeTag == 1) {
            m_valueWord = static_cast<u16>(value);
        }
    } else {
        if (m_typeTag == 2) {

            AddrWord<char> rec;
            rec.m_addr = static_cast<char*>(name);
            g_recs23[idx].m_callback(rec.m_word, value);
        } else if (m_typeTag == 1) {
            g_recs23[idx].m_value = static_cast<short>(value);
        }
    }
}

RVA(0x0016d990, 0x3)
__declspec(naked) void* GetRetAddr() {
    __asm {
        pop  eax
        push eax
        ret
    }
}

RVA(0x0016d9c0, 0x75)
RVA_COMPGEN(0x0016da40, 0x1e, ??_GzErrHandling@@UAEPAXI@Z)
zErrHandling::zErrHandling(CVariantSlot* errSink)

    : m_errSink(errSink ? errSink : &g_globalErrorSlot) {

    if (g_errOutOfMem == NULL) {
        g_errOutOfMem = "Out of memory";
        g_errDataInvalid = "Data structure is invalid";
        g_errOverflow = "Overflow";
        g_errNoFile = "No such file, handle or object";
        g_errOutOfRange = "Out of range";
        g_errExists = "Target alrready exisits";
        g_errNullArg = "Null pointer argument";
        g_errBadArg = "Bad argument value";
    }
}

RVA(0x0016da60, 0x12)
zErrHandling::~zErrHandling() {

    m_errSink->Add(this, 0);
}

// @early-stop
RVA(0x0016da80, 0x10b)
void* _zvec::GrowTo(i32 idx, i32 at) {
    void* p;
    if (idx < m_lo) {
        p = realloc(m_base, (m_hi - (idx - at) + 1) * m_stride);
        if (!p) {
            g_retAddrBreadcrumb = GetCallerRetAddr();
            m_errSink->Set(this, const_cast<char*>(s_out_of_memory), 0x22);
            return 0;
        }
        i32 oldbytes = (m_hi - m_lo + 1) * m_stride;
        i32 shift = m_lo - (idx - at);
        m_grown = shift;
        m_alloc = static_cast<char*>(p);
        memcpy(m_alloc + shift * m_stride, p, oldbytes);
        memset(m_alloc, 0, m_grown * m_stride);
        m_lo = idx - at;
        m_base = static_cast<char*>(p);
        return p;
    }
    i32 hinew = idx + at;
    p = realloc(m_base, (hinew - m_lo + 1) * m_stride);
    if (!p) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, const_cast<char*>(s_out_of_memory), 0x22);
        return 0;
    }
    i32 oldbytes = (m_hi - m_lo + 1) * m_stride;
    char* fill = static_cast<char*>(p) + oldbytes;
    m_grown = hinew - m_hi;
    m_alloc = fill;
    memset(fill, 0, m_grown * m_stride);
    m_hi = hinew;
    m_base = static_cast<char*>(p);
    return p;
}

// @early-stop
RVA(0x0016db90, 0x206)
void* zPTree::Insert(const char* key, void* value) {
    if (m_lookupPending == 0) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, const_cast<char*>("No prior lookup"), 0x16);
        return 0;
    }
    i32 newbit = m_keyBitLength - 7;
    m_lookupPending = 0;
    m_keyBitLength = newbit;
    if (key == NULL || value == NULL) {
        char* msg = g_errNullArg;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0x16);
        return 0;
    }

    i32 critbit;
    if (m_candidateLeaf != NULL) {
        critbit = FirstDiffBit(key, m_candidateLeaf->m_key);
    } else {
        critbit = newbit - 1;
    }

    CButeTreeNode* node = static_cast<CButeTreeNode*>(::operator new(0x14));
    if (node != NULL) {
        node->m_value = static_cast<char*>(value);
        node->m_bit = critbit;
        char* keybuf = static_cast<char*>(::operator new((m_keyBitLength >> 3) + 1));
        node->m_key = keybuf;
        if (keybuf != NULL) {
            strcpy(keybuf, key);

            i32 dir = key[critbit >> 3] & (1 << (critbit & 7));
            if (dir) {
                node->m_child[1] = node;
            } else {
                node->m_child[0] = node;
            }

            CButeTreeNode* cursor = m_descentCursor;
            i32 d2 = dir;
            if (cursor == NULL) {
                m_root = node;
            } else if (critbit < cursor->m_bit) {

                CButeTreeNode* p = m_root;
                m_descentCursor = NULL;
                m_candidateLeaf = p;
                if (p->m_bit <= critbit) {
                    CButeTreeNode* c;
                    do {
                        p = m_candidateLeaf;
                        m_descentCursor = p;
                        d2 = key[p->m_bit >> 3] & (1 << (p->m_bit & 7));
                        CButeTreeNode** s = p->m_child;
                        if (d2) {
                            ++s;
                        }
                        c = *s;
                        m_candidateLeaf = c;
                    } while (c->m_bit <= critbit);
                }
                CButeTreeNode* cur2 = m_descentCursor;
                if (cur2 == NULL) {
                    m_root = node;
                } else {
                    CButeTreeNode** s2 = cur2->m_child;
                    if (d2) {
                        ++s2;
                    }
                    *s2 = node;
                }
            } else {
                CButeTreeNode** s1 = cursor->m_child;
                if (key[cursor->m_bit >> 3] & (1 << (cursor->m_bit & 7))) {
                    ++s1;
                }
                *s1 = node;
            }

            CButeTreeNode** other = &node->m_child[1];
            if (dir) {
                other = &node->m_child[0];
            }
            *other = m_candidateLeaf;
            m_nodeCount++;
            return value;
        }
    }

    char* msg = g_errOutOfMem;
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, msg, 0xc);
    return 0;
}

RVA(0x0016dda0, 0x3c)
_zdvec::_zdvec(i32 stride, i32 lo, i32 hi, void* scratch) : _zvec(stride, lo, hi, scratch) {
    m_alloc = m_base;
    m_grown = m_hi - m_lo + 1;
}

RVA_COMPGEN(0x0016dde0, 0x1e, ??_G_zdvec@@UAEPAXI@Z)

RVA_COMPGEN(0x0016de00, 0x5, ??1_zdvec@@UAE@XZ)

// @early-stop
RVA(0x0016de30, 0xe7)
_zvec::_zvec(i32 stride, i32 lo, i32 hi, void* scratch) : zErrHandling(&g_dynamicArrayErrorSlot) {
    m_spare = static_cast<char*>(scratch);
    m_lo = lo;
    m_hi = hi;
    m_base = NULL;
    m_stride = stride;
    if (lo > hi) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, const_cast<char*>("Inconsistent bounds"), 0x16);
        return;
    }
    i32 total = (hi - lo + 1) * stride;
    void* buf = malloc(total);
    m_base = static_cast<char*>(buf);
    if (buf != NULL) {
        memset(buf, 0, total);
        if (m_spare != NULL) {
            return;
        }
        m_spare = static_cast<char*>(malloc(m_stride));
        if (m_spare != NULL) {
            return;
        }
    }
    g_retAddrBreadcrumb = GetCallerRetAddr();
    m_errSink->Set(this, const_cast<char*>("out of memory"), 0xc);
}

RVA_COMPGEN(0x0016df20, 0x1e, ??_G_zvec@@UAEPAXI@Z)

RVA(0x0016df40, 0x22)
_zvec::~_zvec() {
    char* p = m_base;
    if (p) {
        free(p);
    }
}

RVA(0x0016df70, 0x22)
zPtrColl::zPtrColl(i32 n, void(__cdecl* teardown)(void*))

    : m_teardown(teardown), m_kind(static_cast<i16>(n)), m_nodeCount(0) {}

RVA_COMPGEN(0x0016dfa0, 0x1e, ??_GzPtrColl@@UAEPAXI@Z)
RVA(0x0016dfc0, 0x7)
zPtrColl::~zPtrColl() {}

RVA(0x0016dff0, 0x73)
zPTree::zPTree(void(__cdecl* teardown)(void*), i32 n)

    : zErrHandling(&g_symTabErrorSlot), zPtrColl(n, teardown), m_root(0), m_lookupPending(0) {}

RVA(0x0016e070, 0x7b)
void zPTree::ClearRecursive(CButeTreeNode* node) {
    CButeTreeNode* n = node;
    if (n == NULL) {
        n = m_root;
        if (n == NULL) {
            return;
        }
    }
    if (n->m_child[0] != NULL && n->m_child[0]->m_bit > n->m_bit) {
        ClearRecursive(n->m_child[0]);
    }
    if (n->m_child[1] != NULL && n->m_child[1]->m_bit > n->m_bit) {
        ClearRecursive(n->m_child[1]);
    }
    ::operator delete(n->m_key);
    if (m_kind & 2) {
        m_teardown(n->m_value);
        ::operator delete(n->m_value);
    }
    ::operator delete(n);
}

RVA(0x0016e0f0, 0x4)
__declspec(naked) void* GetCallerRetAddr() {
    __asm {
        mov eax, [ebp + 4]
        ret
    }
}

RVA(0x0016e100, 0x7f)
i32 zBitVec::SetSize(i32 nbits) {
    u32 n = static_cast<u32>(nbits);
    if (n > 0x20) {
        i32 nwords =
            static_cast<i32>((n >> BITARRAY_WORD_SHIFT) + ((n & BITARRAY_BIT_MASK) != 0 ? 1u : 0u));
        m_capacity = nwords;
        u32* band = static_cast<u32*>(malloc(nwords * 4));
        m_words = band;
        if (!band) {
            return 0;
        }
        memset(band, 0, m_capacity << 2);
        m_capacity = m_capacity << 5;
        return 1;
    }
    m_words = NULL;
    m_capacity = 0x20;
    return 1;
}

RVA(0x0016e1a0, 0x23)
CVariantSlot::CVariantSlot(char* label) {
    m_typeTag = 2;
    m_reserved10 = 2;

    m_callback = TmErrorHandler;
    m_valueWord = 0;
    m_label = label;
}

RVA(0x0016e1d0, 0x4b)
i32 CVariantSlot::Find(i32 key) {

    i32 lo = 0;
    i32 hi = g_recCount23 - 1;
    if (hi >= 0) {
        do {
            i32 mid = (hi + lo) / 2;
            m_searchIndex = mid;
            i32 d = g_recs23[mid].m_key - key;
            if (d < 0) {
                lo = mid + 1;
            } else if (d <= 0) {
                return mid;
            } else {
                hi = mid - 1;
            }
        } while (lo <= hi);
    }
    m_searchIndex = hi + 1;
    return -1;
}

RVA(0x0016e220, 0x139)
void TmErrorHandler(char* prefix, i32 errNum) {
    char tmp[10];
    char* np = &tmp[9];
    *np = 0;
    if (errNum != 0) {
        do {
            *--np = static_cast<char>((errNum % 10)) + '0';
            errNum = errNum / 10;
        } while (errNum != 0);
    }

    char msg[0x54];
    char* q = msg;
    while (0 != *prefix) {
        if (q >= &msg[0x40]) {
            break;
        }
        *q++ = *prefix++;
    }
    const char* s;
    s = " - error #";
    while (*s != 0) {
        *q++ = *s++;
    }
    while (*np != 0) {
        *q++ = *np++;
    }
    s = " Caller IP = ";
    while (*s != 0) {
        *q++ = *s++;
    }

    AddrWord<char> bc;
    bc.m_addr = static_cast<char*>(g_retAddrBreadcrumb);
    u32 v = 0xffff & bc.m_uword;
    char* hp = &tmp[9];
    *hp = 0;
    i32 i;
    i = 7;
    do {

        --hp;
        i32 d = v & 0xf;
        *hp = static_cast<char>((d > 9 ? d + 0x37 : d + 0x30));
        v >>= 4;
        if (4 == i) {
            break;
        }
    } while (i-- != 0);
    AddrWord<char> back;
    back.m_uword = v;
    g_retAddrBreadcrumb = back.m_addr;
    while (*hp != 0) {
        *q++ = *hp++;
    }
    *q++ = '\n';
    *q = 0;

    MessageBeep(0);
    MessageBoxA(0, msg, "C++ Tools error handler", MB_TASKMODAL | MB_ICONHAND);
    FatalAppExitA(0, "The error handler terminated the application");
    exit(1);
}

// @early-stop
RVA(0x0016e360, 0x11a)
void* CVariantSlot::Add(void* key, void* val) {
    union CallbackWord {
        void* generic;
        VariantCallback callback;
    } callbackWord;
    int count = g_recCount23;
    if (val != NULL && count >= 0x20) {
        return 0;
    }
    int idx;
    if (count != 0) {

        AddrWord<char> k;
        k.m_addr = static_cast<char*>(key);
        idx = Find(k.m_word);
    } else {
        idx = -1;
    }
    if (idx == -1) {
        if (val == NULL) {
            return 0;
        }
        if (g_recCount23 != 0) {
            memcpy(
                &g_recs23[m_searchIndex + 1],
                &g_recs23[m_searchIndex],
                (g_recCount23 - m_searchIndex) * sizeof(TypeKeyRec)
            );
        }
        callbackWord.generic = val;
        g_recs23[m_searchIndex].m_callback = callbackWord.callback;
        AddrWord<char> nk;
        nk.m_addr = static_cast<char*>(key);
        g_recs23[m_searchIndex].m_key = nk.m_word;
        g_recCount23 = g_recCount23 + 1;
        g_recs23[m_searchIndex].m_value = 0;
        return 0;
    }
    callbackWord.callback = g_recs23[idx].m_callback;
    void* old = callbackWord.generic;
    if (val != NULL) {
        callbackWord.generic = val;
        g_recs23[idx].m_callback = callbackWord.callback;
        return old;
    }
    memcpy(
        &g_recs23[m_searchIndex],
        &g_recs23[m_searchIndex + 1],
        (g_recCount23 - m_searchIndex - 1) * sizeof(TypeKeyRec)
    );
    g_recCount23 = g_recCount23 - 1;
    return old;
}

RVA(0x0016e480, 0x3e)
i32 FirstDiffBit(const char* a, const char* b) {
    i32 n = 0;
    while (*a == *b) {
        n += 8;
        ++a;
        ++b;
    }
    i32 x = *a;
    x ^= *b;
    i32 c = 0;
    while (!(x & 1)) {
        x >>= 1;
        ++c;
    }
    return c + n;
}

RVA(0x0016e4c0, 0xf)
i32 LogicHitFactory(CGameObject* obj) {
    return obj->m_animWorker->m_logic->AdvanceAnimation();
}

RVA(0x0016e4d0, 0xf)
i32 LogicAttackFactory(CGameObject* obj) {
    return obj->m_animWorker->m_logic->StepAttackFire();
}

RVA(0x0016e4e0, 0xf)
i32 LogicBumpFactory(CGameObject* obj) {
    return obj->m_animWorker->m_logic->RecordFrameTick();
}

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
            nodes->~CString();
        }
        ++nodes;
    }
}

RVA(0x0016e4f0, 0x19b)
i32 ProjTypeXfer(CUserLogic* ar) {
    CString* entry = TypeResolve(ar->m_objAux->ActKey());
    FreeNodes();
    ar->XferName(entry->GetBuffer(0));
    ar->FireActivation(ar->m_objAux->ActKey());

    entry = TypeResolve(ar->m_objAux->ActKey());
    FreeNodes();
    ar->FinalizeStep(entry->GetBuffer(0));
    return 1;
}

__inline CButeTree::CButeTree(void(__cdecl* teardown)(void*), i32 n) : zPTree(teardown, n) {}

DATA(0x002bf620)
CButeTree g_buteTree = CButeTree(&ButeTreeNopFree, 0);

RVA_COMPGEN(0x0016e7a0, 0x48, ??__Fg_typeColl@@YAXXZ)

RVA_COMPGEN(0x0016e9c0, 0x45, ??_GCButeTree@@UAEPAXI@Z)

RVA(0x0016ea10, 0x1)
void ButeTreeNopFree(void*) {}

RVA_COMPGEN(0x0016ea20, 0x51, ??_GCTypeCollRuntime@@UAEPAXI@Z)
