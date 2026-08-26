#include <rva.h>

#include <Mfc.h>

#include <AddrWord.h>
#include <Bute/ButeStore.h>
#include <Bute/ButeTree.h>
#include <Bute/PTreeNode.h>
#include <Enums.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/TypeKeyCollStr.h>
#include <Gruntz/UserLogic.h>
#include <Utils/BitArrayWord.h>
#include <Wap32/zBitVec.h>
#include <Wap32/ZVec.h>

#include <ctype.h>
#include <iostream.h>
#include <stdlib.h>
#include <string.h>

#undef isspace
#undef isdigit
#pragma function(memcpy)

DATA(0x002c0380)
void* g_retAddrBreadcrumb;

// Interior fields of one CActReg; do not define overlapping globals.

DATA(0x0021bc88)
i32 g_defaultProjActSize = 32;

DATA(0x0021bd54)
char s_out_of_memory[] = "out of memory";

RVA_DYNINIT(0x0016d9d0, 0x5, g_zBitSetErrorSlot)
RVA_DYNINIT(0x0016d9e0, 0x10, g_zBitSetErrorSlot)
DATA(0x002c0360)
CVariantSlot g_zBitSetErrorSlot("zBitSet: ");

DATA(0x002c0378)
void(__cdecl* g_tmErrorCallback)(char* buf, i32 v);

RVA_DYNINIT(0x0016dc80, 0x5, g_globalErrorSlot)
RVA_DYNINIT(0x0016dc90, 0x10, g_globalErrorSlot)
DATA(0x002c0388)
CVariantSlot g_globalErrorSlot("Global Error: ");

DATA(0x002c03a0)
char* g_errDataInvalid;
DATA(0x002c03a4)
char* g_errOverflow;
DATA(0x002c03a8)
char* g_errOutOfRange;
DATA(0x002c03ac)
char* g_errNullArg;
DATA(0x002c03b0)
char* g_errExists;
DATA(0x002c03b4)
char* g_errBadArg;
DATA(0x002c03b8)
char* g_errNoFile;
DATA(0x002c03bc)
char* g_errOutOfMem;

RVA_DYNINIT(0x0016e0f0, 0x5, g_dynamicArrayErrorSlot)
RVA_DYNINIT(0x0016e100, 0x10, g_dynamicArrayErrorSlot)
DATA(0x002c03c0)
CVariantSlot g_dynamicArrayErrorSlot("Dynamic Array: ");

RVA_DYNINIT(0x0016e2b0, 0x5, g_rezArchiveErrorSlot)
RVA_DYNINIT(0x0016e2c0, 0x10, g_rezArchiveErrorSlot)
DATA(0x002c03d8)
CVariantSlot g_rezArchiveErrorSlot("zSymTab: ");

RVA_DYNINIT(0x0016e460, 0x5, g_variantOverrides)
RVA_DYNINIT(0x0016e470, 0x1, g_variantOverrides)
DATA(0x002c03f0)
TypeKeyRec g_variantOverrides[32];
DATA(0x002c0570)
i32 g_variantOverrideCount;

RVA(0x0016d0b0, 0x22f)
ostream& WriteCurve(ostream& accum, const CMotionState& c) {
    accum << c.m_time;
    accum << c.m_deltaTime;
    accum << c.m_acceleration.x;
    accum << c.m_acceleration.y;
    accum << c.m_acceleration.z;
    accum << c.m_velocity.x;
    accum << c.m_velocity.y;
    accum << c.m_velocity.z;
    accum << c.m_position.x;
    accum << c.m_position.y;
    accum << c.m_position.z;
    accum << c.m_minBounds.x;
    accum << c.m_minBounds.y;
    accum << c.m_minBounds.z;
    accum << c.m_maxBounds.x;
    accum << c.m_maxBounds.y;
    accum << c.m_maxBounds.z;
    accum << c.m_step.x;
    accum << c.m_step.y;
    accum << c.m_step.z;
    accum << c.m_stepDisabled;
    accum << c.m_reservedc0.x;
    accum << c.m_reservedc0.y;
    accum << c.m_reservedc0.z;
    accum << c.m_maxStep.x;
    accum << c.m_maxStep.y;
    accum << c.m_maxStep.z;
    accum << c.m_maxVelocity.x;
    accum << c.m_maxVelocity.y;
    accum << c.m_maxVelocity.z;
    return accum;
}

RVA(0x0016d2e0, 0x189)
istream& ReadCurve(istream& accum, CMotionState& c) {
    accum >> c.m_time;
    accum >> c.m_deltaTime;
    accum >> c.m_acceleration.x;
    accum >> c.m_acceleration.y;
    accum >> c.m_acceleration.z;
    accum >> c.m_velocity.x;
    accum >> c.m_velocity.y;
    accum >> c.m_velocity.z;
    accum >> c.m_position.x;
    accum >> c.m_position.y;
    accum >> c.m_position.z;
    accum >> c.m_minBounds.x;
    accum >> c.m_minBounds.y;
    accum >> c.m_minBounds.z;
    accum >> c.m_maxBounds.x;
    accum >> c.m_maxBounds.y;
    accum >> c.m_maxBounds.z;
    accum >> c.m_step.x;
    accum >> c.m_step.y;
    accum >> c.m_step.z;
    accum >> c.m_stepDisabled;
    accum >> c.m_reservedc0.x;
    accum >> c.m_reservedc0.y;
    accum >> c.m_reservedc0.z;
    accum >> c.m_maxStep.x;
    accum >> c.m_maxStep.y;
    accum >> c.m_maxStep.z;
    accum >> c.m_maxVelocity.x;
    accum >> c.m_maxVelocity.y;
    accum >> c.m_maxVelocity.z;
    return accum;
}

// @early-stop
RVA(0x0016d470, 0x101)
void* zPTree::Find(const char* key) {
    if (key == NULL) {
        char* msg = g_errNullArg;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0x16);
        return NULL;
    }
    CButeTreeNode* root = m_root;
    m_descentCursor = root;
    m_candidateLeaf = NULL;
    m_lookupPending = true;
    i32 bitmax = static_cast<i32>(strlen(key)) * PTREE_BITS_PER_BYTE + PTREE_BYTE_BIT_MASK;
    m_keyBitLength = bitmax;
    if (root == NULL) {
        return NULL;
    }
    i32 b = root->m_bit;
    while (b <= bitmax) {
        CButeTreeNode** slot = m_descentCursor->m_child;
        if (key[b >> PTREE_BYTE_BIT_SHIFT] & (1 << (b & PTREE_BYTE_BIT_MASK))) {
            ++slot;
        }
        CButeTreeNode* child = *slot;
        m_candidateLeaf = child;
        if (child == NULL) {
            return NULL;
        }
        if (child->m_bit <= b) {
            if (strcmp(key, child->m_key) == 0) {
                m_lookupPending = false;
                return m_candidateLeaf->m_value;
            }
            return NULL;
        }
        m_descentCursor = child;
        b = child->m_bit;
    }
    m_candidateLeaf = m_descentCursor;
    return NULL;
}

RVA(0x0016d580, 0x26)
zBitVec::~zBitVec() {
    if (static_cast<u32>(m_capacity) > 0x20) {
        free(m_words);
    }
}

RVA_COMPGEN(0x0016d5b0, 0x1e, ??_GzBitVec@@UAEPAXI@Z)
// @early-stop
RVA(0x0016d5d0, 0xac)
zBitVec& zBitVec::operator=(const zBitVec& that) {
    if (this != &that) {
        if (m_capacity != that.m_capacity) {
            if (static_cast<u32>(m_capacity) > 0x20) {
                delete[] m_words;
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
RVA(0x0016d680, 0x344)
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
        if (SetSize(minSize)) {
            return;
        }
        goto oom;
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
        if (*p == 0) {
            break;
        }
        while (!isdigit(*p)) {
            if (*p == 0) {
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
            do {
                ++q;
            } while (isspace(*q));
        }
        char sep = *q;
        ++q;
        if (isspace(*q)) {
            do {
                ++q;
            } while (isspace(*q));
        }
        if (sep == '-') {
            i32 v2 = 0;
            if (*q == 0) {
                break;
            }
            while (isdigit(*q)) {
                v2 = v2 * 10 + (*q - '0');
                ++q;
            }
            if (static_cast<u32>(v) > static_cast<u32>(v2)) {
                i32 t = v;
                v = v2;
                v2 = t;
            }
            for (++v; static_cast<u32>(v) <= static_cast<u32>(v2); ++v) {
                u32* band = (static_cast<u32>(m_capacity) > 0x20) ? m_words : &m_inline;
                band[static_cast<u32>(v) >> BITARRAY_WORD_SHIFT] |= 1u << (v & BITARRAY_BIT_MASK);
            }
            if (*q == 0) {
                break;
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

RVA(0x0016d9f0, 0x76)
zBitVec::zBitVec() : zErrHandling(&g_zBitSetErrorSlot) {
    if (!SetSize(g_defaultProjActSize)) {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0xc);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0016da70, 0xb1)
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

RVA(0x0016db30, 0x11e)

void CVariantSlot::Set(zErrHandling* key, char* name, i32 value) {
    if (m_typeTag == VARIANT_SLOT_DIRECT_VALUE) {
        m_valueWord = static_cast<u16>(value);
        return;
    }
    i32 idx;
    if (g_variantOverrideCount != 0) {

        AddrWord<zErrHandling> k;
        k.m_addr = key;
        idx = this->Find(k.m_word);
    } else {
        idx = -1;
    }
    if (idx == -1) {
        if (m_typeTag == VARIANT_SLOT_CALLBACK) {

            char buf[0xa0];
            strcpy(buf, m_label);

            strncat(buf, name, 0x4f);
            m_callback(buf, value);
        } else if (m_typeTag == VARIANT_SLOT_RECORD_VALUE) {
            m_valueWord = static_cast<u16>(value);
        }
    } else {
        if (m_typeTag == VARIANT_SLOT_CALLBACK) {
            g_variantOverrides[idx].m_callback(name, value);
        } else if (m_typeTag == VARIANT_SLOT_RECORD_VALUE) {
            g_variantOverrides[idx].m_value = static_cast<short>(value);
        }
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0016dc50, 0x17)
CVariantSlot* CVariantSlot::EnsureTmErrorCallback() {
    if (g_tmErrorCallback == NULL) {
        g_tmErrorCallback = TmErrorHandler;
    }
    return this;
}

RVA(0x0016dc70, 0x3)
__declspec(naked) void* GetRetAddr() {
    __asm {
        pop  eax
        push eax
        ret
    }
}

RVA(0x0016dca0, 0x75)
RVA_COMPGEN(0x0016dd20, 0x1e, ??_GzErrHandling@@UAEPAXI@Z)
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

RVA(0x0016dd40, 0x12)
zErrHandling::~zErrHandling() {

    m_errSink->Add(this, NULL);
}

// @early-stop
RVA(0x0016dd60, 0x10b)
void* _zvec::GrowTo(i32 idx, i32 at) {
    char* p;
    if (idx < m_lo) {
        idx -= at;
        p = static_cast<char*>(realloc(m_base, (m_hi - idx + 1) * m_stride));
        if (!p) {
            g_retAddrBreadcrumb = GetCallerRetAddr();
            m_errSink->Set(this, const_cast<char*>(s_out_of_memory), 0x22);
            return NULL;
        }
        i32 oldbytes = (m_hi - m_lo + 1) * m_stride;
        i32 shift = m_lo - idx;
        m_grown = shift;
        m_alloc = p;
        memcpy(m_alloc + shift * m_stride, p, oldbytes);
        memset(m_alloc, 0, m_grown * m_stride);
        m_lo = idx;
        m_base = p;
        return p;
    }
    idx += at;
    p = static_cast<char*>(realloc(m_base, (idx - m_lo + 1) * m_stride));
    if (!p) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, const_cast<char*>(s_out_of_memory), 0x22);
        return NULL;
    }
    i32 oldbytes = (m_hi - m_lo + 1) * m_stride;
    char* fill = p + oldbytes;
    m_grown = idx - m_hi;
    m_alloc = fill;
    memset(fill, 0, m_grown * m_stride);
    m_hi = idx;
    m_base = p;
    return p;
}

// @early-stop
RVA(0x0016de70, 0x206)
void* zPTree::Insert(const char* key, void* value) {
    if (m_lookupPending == false) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, const_cast<char*>("No prior lookup"), 0x16);
        return NULL;
    }
    m_lookupPending = false;
    m_keyBitLength -= PTREE_BYTE_BIT_MASK;
    if (key == NULL || value == NULL) {
        char* msg = g_errNullArg;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0x16);
        return NULL;
    }

    i32 critbit;
    if (m_candidateLeaf != NULL) {
        critbit = FirstDiffBit(key, m_candidateLeaf->m_key);
    } else {
        critbit = m_keyBitLength - 1;
    }

    CButeTreeNode* node = new CButeTreeNode;
    if (node == NULL) {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0xc);
        return NULL;
    }
    node->m_value = static_cast<char*>(value);
    node->m_bit = critbit;
    char* keybuf = new char[(m_keyBitLength >> PTREE_BYTE_BIT_SHIFT) + 1];
    node->m_key = keybuf;
    if (keybuf == NULL) {
        char* msg = g_errOutOfMem;
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, msg, 0xc);
        return NULL;
    }
    strcpy(keybuf, key);

    i32 dir = key[critbit >> PTREE_BYTE_BIT_SHIFT] & (1 << (critbit & PTREE_BYTE_BIT_MASK));
    CButeTreeNode** child = node->m_child;
    if (dir) {
        ++child;
    }
    *child = node;

    CButeTreeNode* cursor = m_descentCursor;
    i32 d2 = dir;
    if (cursor != NULL) {
        if (critbit >= cursor->m_bit) {
            CButeTreeNode** s1 = cursor->m_child;
            if (key[cursor->m_bit >> PTREE_BYTE_BIT_SHIFT]
                & (1 << (cursor->m_bit & PTREE_BYTE_BIT_MASK))) {
                ++s1;
            }
            *s1 = node;
        } else {
            CButeTreeNode* p = m_root;
            m_descentCursor = NULL;
            m_candidateLeaf = p;
            if (p->m_bit <= critbit) {
                CButeTreeNode* c;
                do {
                    p = m_candidateLeaf;
                    m_descentCursor = p;
                    d2 = key[p->m_bit >> PTREE_BYTE_BIT_SHIFT]
                         & (1 << (p->m_bit & PTREE_BYTE_BIT_MASK));
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
        }
    } else {
        m_root = node;
    }

    CButeTreeNode** other = &node->m_child[1];
    if (dir) {
        other = &node->m_child[0];
    }
    *other = m_candidateLeaf;
    m_nodeCount++;
    return value;
}

RVA(0x0016e080, 0x3c)
_zdvec::_zdvec(i32 stride, i32 lo, i32 hi, void* scratch) : _zvec(stride, lo, hi, scratch) {
    m_alloc = m_base;
    m_grown = m_hi - m_lo + 1;
}

RVA_COMPGEN(0x0016e0c0, 0x1e, ??_G_zdvec@@UAEPAXI@Z)

RVA_COMPGEN(0x0016e0e0, 0x5, ??1_zdvec@@UAE@XZ)

RVA(0x0016e110, 0xe7)
_zvec::_zvec(i32 stride, i32 lo, i32 hi, void* scratch)
    : zErrHandling(&g_dynamicArrayErrorSlot),
      m_lo(lo),
      m_hi(hi),
      m_base(NULL),
      m_spare(static_cast<char*>(scratch)),
      m_stride(stride) {
    if (lo > hi) {
        g_retAddrBreadcrumb = GetCallerRetAddr();
        m_errSink->Set(this, const_cast<char*>("Inconsistent bounds"), 0x16);
        return;
    }
    i32 total = (hi - lo + 1) * stride;
    char* buf = static_cast<char*>(malloc(total));
    m_base = buf;
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
    m_errSink->Set(this, const_cast<char*>(s_out_of_memory), 0xc);
}

RVA_COMPGEN(0x0016e200, 0x1e, ??_G_zvec@@UAEPAXI@Z)

RVA(0x0016e220, 0x22)
_zvec::~_zvec() {
    char* p = m_base;
    if (p) {
        free(p);
    }
}

RVA(0x0016e250, 0x22)
zPtrColl::zPtrColl(i32 n, void(__cdecl* teardown)(void*))

    : m_teardown(teardown), m_kind(static_cast<i16>(n)), m_nodeCount(0) {}

RVA_COMPGEN(0x0016e280, 0x1e, ??_GzPtrColl@@UAEPAXI@Z)
RVA(0x0016e2a0, 0x7)
zPtrColl::~zPtrColl() {}

RVA(0x0016e2d0, 0x73)
zPTree::zPTree(void(__cdecl* teardown)(void*), i32 n)

    : zErrHandling(&g_rezArchiveErrorSlot),
      zPtrColl(n, teardown),
      m_root(NULL),
      m_lookupPending(false) {}

RVA(0x0016e350, 0x7b)
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
    delete[] n->m_key;
    if (m_kind & 2) {
        m_teardown(n->m_value);
        delete n->m_value;
    }
    delete n;
}

RVA(0x0016e3d0, 0x4)
__declspec(naked) void* GetCallerRetAddr() {
    __asm {
        mov eax, [ebp + 4]
        ret
    }
}

RVA(0x0016e3e0, 0x7f)
i32 zBitVec::SetSize(i32 nbits) {
    u32 n = static_cast<u32>(nbits);
    if (n > 0x20) {
        i32 nwords = static_cast<i32>((n >> BITARRAY_WORD_SHIFT) + ((n & BITARRAY_BIT_MASK) != 0));
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

RVA(0x0016e480, 0x23)
CVariantSlot::CVariantSlot(char* label) {
    m_typeTag = VARIANT_SLOT_CALLBACK;
    m_reserved10 = 2;

    m_callback = TmErrorHandler;
    m_valueWord = 0;
    m_label = label;
}

RVA(0x0016e4b0, 0x4b)
i32 CVariantSlot::Find(i32 key) {

    i32 lo = 0;
    i32 hi = g_variantOverrideCount - 1;
    if (hi >= 0) {
        do {
            i32 mid = (hi + lo) / 2;
            m_searchIndex = mid;
            i32 d = g_variantOverrides[mid].m_key - key;
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

RVA(0x0016e500, 0x139)
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
    MessageBoxA(NULL, msg, "C++ Tools error handler", MB_TASKMODAL | MB_ICONHAND);
    FatalAppExitA(0, "The error handler terminated the application");
    exit(1);
}

// @early-stop
RVA(0x0016e640, 0x11a)
VariantCallback CVariantSlot::Add(zErrHandling* key, VariantCallback val) {
    int count = g_variantOverrideCount;
    if (val != NULL && count >= 0x20) {
        return NULL;
    }
    int idx;
    if (count != 0) {

        AddrWord<zErrHandling> k;
        k.m_addr = key;
        idx = Find(k.m_word);
    } else {
        idx = -1;
    }
    if (idx == -1) {
        if (val == NULL) {
            return NULL;
        }
        if (g_variantOverrideCount != 0) {
            memcpy(
                &g_variantOverrides[m_searchIndex + 1],
                &g_variantOverrides[m_searchIndex],
                (g_variantOverrideCount - m_searchIndex) * sizeof(TypeKeyRec)
            );
        }
        g_variantOverrides[m_searchIndex].m_callback = val;
        AddrWord<zErrHandling> nk;
        nk.m_addr = key;
        g_variantOverrides[m_searchIndex].m_key = nk.m_word;
        g_variantOverrideCount = g_variantOverrideCount + 1;
        g_variantOverrides[m_searchIndex].m_value = 0;
        return NULL;
    }
    VariantCallback old = g_variantOverrides[idx].m_callback;
    if (val != NULL) {
        g_variantOverrides[idx].m_callback = val;
        return old;
    }
    memcpy(
        &g_variantOverrides[m_searchIndex],
        &g_variantOverrides[m_searchIndex + 1],
        (g_variantOverrideCount - m_searchIndex - 1) * sizeof(TypeKeyRec)
    );
    g_variantOverrideCount = g_variantOverrideCount - 1;
    return old;
}

RVA(0x0016e760, 0x3e)
i32 FirstDiffBit(const char* a, const char* b) {
    i32 n = 0;
    while (*a == *b) {
        n += 8;
        ++a;
        ++b;
    }
    i32 x = *a ^ *b;
    i32 c = 0;
    while (!(x & 1)) {
        x >>= 1;
        ++c;
    }
    return c + n;
}

RVA(0x0016e7a0, 0xf)
i32 DispatchLogicHit(CGameObject* obj) {
    return obj->m_logicRecord->m_userLogic->AdvanceAnimation();
}

RVA(0x0016e7b0, 0xf)
i32 DispatchLogicAttack(CGameObject* obj) {
    return obj->m_logicRecord->m_userLogic->StepAttackFire();
}

RVA(0x0016e7c0, 0xf)
i32 DispatchLogicBump(CGameObject* obj) {
    return obj->m_logicRecord->m_userLogic->RecordFrameTick();
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
            nodes->CString::CString();
        }
        ++nodes;
    }
}

RVA(0x0016e7d0, 0x19b)
i32 DispatchLogicEvent(CUserLogic* ar) {
    CString* entry = TypeResolve(ar->m_logicRecord->EventCode());
    FreeNodes();
    ar->StepBehavior(entry->GetBuffer(0));
    ar->FireActivation(ar->m_logicRecord->EventCode());

    entry = TypeResolve(ar->m_logicRecord->EventCode());
    FreeNodes();
    ar->FinalizeStep(entry->GetBuffer(0));
    return 1;
}
