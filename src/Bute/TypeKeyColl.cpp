#include <rva.h>

#include <Mfc.h>

#include <Bute/ButeStore.h>
#include <Enums.h>
#include <Gruntz/GameObjectLogicTypes.h>
#include <Gruntz/LogicEventDispatch.h>
#include <Gruntz/MovingLogicSerial.h>
#include <Gruntz/TypeKeyColl.h>
#include <Gruntz/TypeKeyCollStr.h>
#include <Gruntz/UserLogic.h>
#include <Utils/BitArrayWord.h>
#include <ZTools/BitVec.h>
#include <ZTools/Error.h>
#include <ZTools/PTree.h>
#include <ZTools/ZVec.h>

#include <ctype.h>
#include <iostream.h>
#include <stdlib.h>
#include <string.h>

#undef isspace
#undef isdigit
#pragma function(memcpy)

DATA(0x002bf428)
unsigned long zMinErr::caller_ip;

// Interior fields of one CActReg; do not define overlapping globals.

DATA(0x0021ad28)
i32 g_defaultProjActSize = 32;

DATA(0x0021adf4)
char g_out_of_memory[] = "out of memory";

RVA_DYNINIT(0x0016d6f0, 0x5, zBitVec::ceh)
RVA_DYNINIT(0x0016d700, 0x10, zBitVec::ceh)
DATA(0x002bf408)
zErrHandler zBitVec::ceh("zBitSet: ");

DATA(0x002bf420)
erf_t zMinErr::ef;

RVA_DYNINIT(0x0016d9a0, 0x5, _)
RVA_DYNINIT(0x0016d9b0, 0x10, _)
DATA(0x002bf430)
static zErrHandler _("Global Error: ");

DATA(0x002bf448)
char* zErrHandling::_inval;
DATA(0x002bf44c)
char* zErrHandling::_overflow;
DATA(0x002bf450)
char* zErrHandling::_range;
DATA(0x002bf454)
char* zErrHandling::_nullparg;
DATA(0x002bf458)
char* zErrHandling::_exists;
DATA(0x002bf45c)
char* zErrHandling::_badarg;
DATA(0x002bf460)
char* zErrHandling::_nosuch;
DATA(0x002bf464)
char* zErrHandling::_nomem;

RVA_DYNINIT(0x0016de10, 0x5, _zvec::ceh)
RVA_DYNINIT(0x0016de20, 0x10, _zvec::ceh)
DATA(0x002bf468)
zErrHandler _zvec::ceh("Dynamic Array: ");

RVA_DYNINIT(0x0016dfd0, 0x5, zPTree::ceh)
RVA_DYNINIT(0x0016dfe0, 0x10, zPTree::ceh)
DATA(0x002bf480)
zErrHandler zPTree::ceh("zSymTab: ");

DATA(0x002bf498)
_dhandler zErrHandler::dl[32];
DATA(0x002bf618)
i32 zErrHandler::ndh;

RVA(0x0016cdd0, 0x22f)
ostream& WriteCurve(ostream& accum, const CMotionState& c) {
    accum << c.m_time;
    accum << c.m_deltaTime;
    accum << c.m_acceleration.m_x;
    accum << c.m_acceleration.m_y;
    accum << c.m_acceleration.m_z;
    accum << c.m_velocity.m_x;
    accum << c.m_velocity.m_y;
    accum << c.m_velocity.m_z;
    accum << c.m_position.m_x;
    accum << c.m_position.m_y;
    accum << c.m_position.m_z;
    accum << c.m_minBounds.m_x;
    accum << c.m_minBounds.m_y;
    accum << c.m_minBounds.m_z;
    accum << c.m_maxBounds.m_x;
    accum << c.m_maxBounds.m_y;
    accum << c.m_maxBounds.m_z;
    accum << c.m_step.m_x;
    accum << c.m_step.m_y;
    accum << c.m_step.m_z;
    accum << c.m_stepDisabled;
    accum << c.m_reservedc0.m_x;
    accum << c.m_reservedc0.m_y;
    accum << c.m_reservedc0.m_z;
    accum << c.m_maxStep.m_x;
    accum << c.m_maxStep.m_y;
    accum << c.m_maxStep.m_z;
    accum << c.m_maxVelocity.m_x;
    accum << c.m_maxVelocity.m_y;
    accum << c.m_maxVelocity.m_z;
    return accum;
}

RVA(0x0016d000, 0x189)
istream& ReadCurve(istream& accum, CMotionState& c) {
    accum >> c.m_time;
    accum >> c.m_deltaTime;
    accum >> c.m_acceleration.m_x;
    accum >> c.m_acceleration.m_y;
    accum >> c.m_acceleration.m_z;
    accum >> c.m_velocity.m_x;
    accum >> c.m_velocity.m_y;
    accum >> c.m_velocity.m_z;
    accum >> c.m_position.m_x;
    accum >> c.m_position.m_y;
    accum >> c.m_position.m_z;
    accum >> c.m_minBounds.m_x;
    accum >> c.m_minBounds.m_y;
    accum >> c.m_minBounds.m_z;
    accum >> c.m_maxBounds.m_x;
    accum >> c.m_maxBounds.m_y;
    accum >> c.m_maxBounds.m_z;
    accum >> c.m_step.m_x;
    accum >> c.m_step.m_y;
    accum >> c.m_step.m_z;
    accum >> c.m_stepDisabled;
    accum >> c.m_reservedc0.m_x;
    accum >> c.m_reservedc0.m_y;
    accum >> c.m_reservedc0.m_z;
    accum >> c.m_maxStep.m_x;
    accum >> c.m_maxStep.m_y;
    accum >> c.m_maxStep.m_z;
    accum >> c.m_maxVelocity.m_x;
    accum >> c.m_maxVelocity.m_y;
    accum >> c.m_maxVelocity.m_z;
    return accum;
}

// @early-stop
RVA(0x0016d190, 0x101)
void* zPTree::lookup(const char* key) {
    if (key == NULL) {
        handle(_nullparg, 0x16);
        return NULL;
    }
    m_p = m_root;
    m_q = NULL;
    m_preview = true;
    m_sbits = static_cast<i32>(strlen(key)) * PTREE_BITS_PER_BYTE + PTREE_BYTE_BIT_MASK;
    if (m_p == NULL) {
        return NULL;
    }
    i32 branch = m_p->m_index;
    while (branch <= m_sbits) {
        m_q = m_p->ptr(bit(key, branch));
        if (m_q == NULL) {
            return NULL;
        }
        if (m_q->m_index <= branch) {
            if (strcmp(key, m_q->m_symbol) == 0) {
                m_preview = false;
                return m_q->m_body;
            }
            return NULL;
        }
        m_p = m_q;
        branch = m_p->m_index;
    }
    m_q = m_p;
    return NULL;
}

RVA(0x0016d2a0, 0x26)
zBitVec::~zBitVec() {
    if (static_cast<u32>(m_capacity) > 0x20) {
        free(m_words);
    }
}

RVA_COMPGEN(0x0016d2d0, 0x1e, ??_GzBitVec@@UAEPAXI@Z)
// @early-stop
RVA(0x0016d2f0, 0xac)
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
                    handle(_nomem, 0xc);
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

RVA(0x0016d3a0, 0x344)
zBitVec::zBitVec(const char* tokens, i32 minSize) : zErrHandling(&zBitVec::ceh) {
    const char* start;
    const char* q;
    if (tokens == NULL) {
        handle(_nullparg, 0x16);
        return;
    }
    if (minSize == 0) {
        minSize = g_defaultProjActSize;
    }

    i32 maxv = 0;
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
            u32* band = body();
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
            i32 rangeEnd = 0;
            if (*q == 0) {
                break;
            }
            while (isdigit(*q)) {
                rangeEnd = rangeEnd * 10 + (*q - '0');
                ++q;
            }
            if (static_cast<u32>(v) > static_cast<u32>(rangeEnd)) {
                i32 t = v;
                v = rangeEnd;
                rangeEnd = t;
            }
            for (++v; static_cast<u32>(v) <= static_cast<u32>(rangeEnd); ++v) {
                u32* band = body();
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
    handle(_nomem, 0xc);
    return;
}
badchar: {
    handle(_badarg, 0x16);
    return;
}
}

RVA(0x0016d710, 0x76)
zBitVec::zBitVec() : zErrHandling(&zBitVec::ceh) {
    if (!SetSize(g_defaultProjActSize)) {
        handle(_nomem, 0xc);
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0016d790, 0xb1)
zBitVec::zBitVec(i32 idx, i32 sizehint) : zErrHandling(&zBitVec::ceh) {
    u32 n = static_cast<u32>(sizehint);
    if (n == 0) {
        n = static_cast<u32>(g_defaultProjActSize);
    }
    if (static_cast<u32>(idx) >= n) {
        n = static_cast<u32>(idx) + 1;
    }
    if (!SetSize(static_cast<i32>(n))) {
        handle(_nomem, 0xc);
    } else {
        u32* base = (static_cast<u32>(m_capacity) > 0x20) ? m_words : &m_inline;
        u32* slot = base + (static_cast<u32>(idx) >> BITARRAY_WORD_SHIFT);
        *slot |= 1u << (idx & BITARRAY_BIT_MASK);
    }
}

RVA(0x0016d850, 0x11e)

void zErrHandler::handle(void* key, const char* name, i32 value) {
    i32 rv;
    char buf[0xa0];
    if (mode == QUICKEST) {
        evalue = static_cast<short>(value);
        return;
    }
    if (ndh != 0) {
        rv = srch(key);
    } else {
        rv = -1;
    }
    if (rv == -1) {
        if (mode == FCALL) {

            strcpy(buf, id);

            strncat(buf, name, 0x4f);
            default_ef(buf, value);
        } else if (mode == LOGGING) {
            evalue = static_cast<short>(value);
        }
    } else {
        if (mode == FCALL) {
            dl[rv].handler(name, value);
        } else if (mode == LOGGING) {
            dl[rv].lasterr = static_cast<short>(value);
        }
    }
}

// @dead-code
// Zero-ref: retail has no caller or address-taking reference.
RVA(0x0016d970, 0x17)
zMinErr::zMinErr() {
    if (zMinErr::ef == NULL) {
        ef = catcher;
    }
}

RVA(0x0016d990, 0x3)
__declspec(naked) unsigned long __ip() {
    __asm {
        pop  eax
        push eax
        ret
    }
}

RVA(0x0016d9c0, 0x75)
RVA_COMPGEN(0x0016da40, 0x1e, ??_GzErrHandling@@UAEPAXI@Z)
zErrHandling::zErrHandling(zErrHandler* errSink)

    : hp(errSink ? errSink : &_) {

    if (_nomem == NULL) {
        _nomem = "Out of memory";
        _inval = "Data structure is invalid";
        _overflow = "Overflow";
        _nosuch = "No such file, handle or object";
        _range = "Out of range";
        _exists = "Target alrready exisits";
        _nullparg = "Null pointer argument";
        _badarg = "Bad argument value";
    }
}

RVA(0x0016da60, 0x12)
zErrHandling::~zErrHandling() {

    hp->set_ef(this, NULL);
}

RVA(0x0016da80, 0x10b)
i32 _zdvec::realloc(i32 idx, i32 at) {
    char* p;
    if (idx < lo) {
        idx -= at;
        p = static_cast<char*>(::realloc(vec, (hi - idx + 1) * size));
        if (!p) {
            handle(g_out_of_memory, 0x22);
            return 0;
        }
        i32 oldbytes = (hi - lo + 1) * size;
        i32 shift = lo - idx;
        initcount = shift;
        init = p;
        memcpy(static_cast<char*>(init) + shift * size, p, oldbytes);
        memset(init, 0, initcount * size);
        lo = idx;
    } else {
        idx += at;
        p = static_cast<char*>(::realloc(vec, (idx - lo + 1) * size));
        if (!p) {
            handle(g_out_of_memory, 0x22);
            return 0;
        }
        init = p + (hi - lo + 1) * size;
        initcount = idx - hi;
        memset(init, 0, initcount * size);
        hi = idx;
    }
    vec = p;
    // PROVEN: the integer status preserves the allocated-address bits on success.
    return reinterpret_cast<i32>(p);
}

RVA(0x0016db90, 0x206)
void* zPTree::add(const char* key, void* value) {
    i32 newbranch;
    i32 dp;
    zPTreeNode* t;

    if (m_preview == false) {
        handle("No prior lookup", 0x16);
        return NULL;
    }
    m_preview = false;
    m_sbits -= PTREE_BYTE_BIT_MASK;
    if (key == NULL || value == NULL) {
        handle(_nullparg, 0x16);
        return NULL;
    }

    newbranch = m_q != NULL ? diffpos(key, m_q->m_symbol) : m_sbits - 1;
    t = new zPTreeNode;
    if (t == NULL) {
        handle(_nomem, 0xc);
        return NULL;
    }
    t->m_index = newbranch;
    t->m_body = value;
    t->m_symbol = new char[(m_sbits >> PTREE_BYTE_BIT_SHIFT) + 1];
    if (t->m_symbol == NULL) {
        handle(_nomem, 0xc);
        return NULL;
    }
    strcpy(t->m_symbol, key);

    dp = bit(key, newbranch);
    t->ptr(dp) = t;

    if (m_p != NULL) {
        if (newbranch >= m_p->m_index) {
            m_p->ptr(bit(key, m_p->m_index)) = t;
        } else {
            m_q = m_root;
            m_p = NULL;
            i32 b;
            while (m_q->m_index <= newbranch) {
                m_p = m_q;
                b = bit(key, m_q->m_index);
                m_q = m_q->ptr(b);
            }
            if (m_p == NULL) {
                m_root = t;
            } else {
                m_p->ptr(b) = t;
            }
        }
    } else {
        m_root = t;
    }

    t->ptr(!dp) = m_q;
    incc();
    return value;
}

RVA(0x0016dda0, 0x3c)
_zdvec::_zdvec(size_t s, i32 l, i32 h, void* overflow) : _zvec(s, l, h, overflow) {
    init = vec;
    initcount = hi - lo + 1;
}

RVA_COMPGEN(0x0016dde0, 0x1e, ??_G_zdvec@@UAEPAXI@Z)

RVA_COMPGEN(0x0016de00, 0x5, ??1_zdvec@@UAE@XZ)

RVA(0x0016de30, 0xe7)
_zvec::_zvec(size_t s, i32 l, i32 h, void* overflow)
    : zErrHandling(&_zvec::ceh), lo(l), hi(h), vec(NULL), ovf(overflow), size(s) {
    if (lo > hi) {
        handle("Inconsistent bounds", 0x16);
        return;
    }
    i32 total = (hi - lo + 1) * s;
    char* buf = static_cast<char*>(malloc(total));
    vec = buf;
    if (buf != NULL) {
        memset(buf, 0, total);
        if (ovf != NULL) {
            return;
        }
        ovf = malloc(size);
        if (ovf != NULL) {
            return;
        }
    }
    handle(g_out_of_memory, 0xc);
}

RVA_COMPGEN(0x0016df20, 0x1e, ??_G_zvec@@MAEPAXI@Z)

RVA(0x0016df40, 0x22)
_zvec::~_zvec() {
    char* p = vec;
    if (p) {
        free(p);
    }
}

RVA(0x0016df70, 0x22)
zPtrColl::zPtrColl(cleanup_behaviour cleanup, dtorf_t destructor)

    : m_dtor(destructor), m_flags(static_cast<i16>(cleanup)), m_count(0) {}

RVA_COMPGEN(0x0016dfa0, 0x1e, ??_GzPtrColl@@UAEPAXI@Z)
RVA(0x0016dfc0, 0x7)
zPtrColl::~zPtrColl() {}

RVA(0x0016dff0, 0x73)
zPTree::zPTree(dtorf_t destructor, cleanup_behaviour cleanup)

    : zErrHandling(&zPTree::ceh), zPtrColl(cleanup, destructor), m_root(NULL), m_preview(false) {}

RVA(0x0016e070, 0x7b)
void zPTree::cleanup(zPTreeNode* node) {
    zPTreeNode* n = node;
    if (n == NULL) {
        n = m_root;
        if (n == NULL) {
            return;
        }
    }
    if (n->m_left != NULL && n->m_left->m_index > n->m_index) {
        cleanup(n->m_left);
    }
    if (n->m_right != NULL && n->m_right->m_index > n->m_index) {
        cleanup(n->m_right);
    }
    delete[] n->m_symbol;
    if (purge()) {
        destroy(n->m_body);
        delete static_cast<char*>(n->m_body);
    }
    delete n;
}

RVA(0x0016e0f0, 0x4)
__declspec(naked) unsigned long __caller_ip() {
    __asm {
        mov eax, [ebp + 4]
        ret
    }
}

RVA(0x0016e100, 0x7f)
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

RVA(0x0016e1a0, 0x23)
zErrHandler::zErrHandler(const char* label) {
    mode = FCALL;
    prevmode = FCALL;

    default_ef = zMinErr::catcher;
    evalue = 0;
    id = label;
}

RVA(0x0016e1d0, 0x4b)
i32 zErrHandler::srch(void* o) {
    i32 lo = 0;
    i32 hi = ndh - 1;
    long cmp;
    if (hi >= 0) {
        do {
            slot = (lo + hi) / 2;
            // PROVEN: the table search compares signed 32-bit object-address differences.
            cmp = reinterpret_cast<long>(dl[slot].object) - reinterpret_cast<long>(o);
            if (cmp < 0) {
                lo = slot + 1;
            } else if (cmp <= 0) {
                return slot;
            } else {
                hi = slot - 1;
            }
        } while (lo <= hi);
    }
    slot = hi + 1;
    return -1;
}

RVA(0x0016e220, 0x139)
void zMinErr::catcher(const char* prefix, i32 errNum) {
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

    u32 v = 0xffff & caller_ip;
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
    caller_ip = v;
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

RVA(0x0016e360, 0x11a)
erf_t zErrHandler::set_ef(void* o, erf_t f) {
    i32 rv;
    if (f != NULL && ndh >= MAX_DEDICATED) {
        return NULL;
    }
    rv = ndh ? srch(o) : -1;
    if (rv == -1) {
        if (f == NULL) {
            return NULL;
        }
        if (ndh != 0) {
            memcpy(&dl[slot + 1], &dl[slot], (ndh - slot) * sizeof(_dhandler));
        }
        dl[slot].handler = f;
        dl[slot].object = o;
        dl[slot].lasterr = 0;
        ndh = ndh + 1;
        return NULL;
    } else {
        erf_t t = dl[rv].handler;
        if (f != NULL) {
            dl[rv].handler = f;
        } else {
            memcpy(&dl[slot], &dl[slot + 1], (ndh - slot - 1) * sizeof(_dhandler));
            ndh = ndh - 1;
        }
        return t;
    }
}

RVA(0x0016e480, 0x3e)
i32 zPTree::diffpos(const char* a, const char* b) {
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

RVA(0x0016e4c0, 0xf)
i32 DispatchLogicHit(CGameObject* obj) {
    return obj->m_logicRecord->m_userLogic->AdvanceAnimation();
}

RVA(0x0016e4d0, 0xf)
i32 DispatchLogicAttack(CGameObject* obj) {
    return obj->m_logicRecord->m_userLogic->StepAttackFire();
}

RVA(0x0016e4e0, 0xf)
i32 DispatchLogicBump(CGameObject* obj) {
    return obj->m_logicRecord->m_userLogic->RecordFrameTick();
}

RVA(0x0016e4f0, 0x19b)
i32 DispatchLogicEvent(CUserLogic* ar) {
    CString* entry = &g_typeColl[ar->m_logicRecord->EventCode()];
    ar->StepBehavior(entry->GetBuffer(0));
    ar->FireActivation(ar->m_logicRecord->EventCode());

    entry = &g_typeColl[ar->m_logicRecord->EventCode()];
    ar->FinalizeStep(entry->GetBuffer(0));
    return 1;
}
