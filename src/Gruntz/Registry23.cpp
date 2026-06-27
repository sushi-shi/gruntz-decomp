// Registry23.cpp - global fixed-capacity (32) keyed record table insert/update/
// remove (RVA 0x16e360), trace-attributed to ClassUnknown_23. The records live in
// a global 12-byte-stride array (key @0x6bf498, val @0x6bf49c, flag @0x6bf4a0) with
// a global count @0x6bf618; the object only supplies the working slot index m_4.
#include <rva.h>
#include <string.h> // memmove

struct Rec23 {
    void* m_0; // key
    void* m_4; // value
    short m_8; // flag
    short m_a;
};
DATA(0x002bf498) extern Rec23 g_recs23[];
DATA(0x002bf618) extern int g_recCount23;

class Reg23 {
public:
    char m_pad[4];
    int m_4;
    int Find(void* key); // 0x16e1d0
    void* Add(void* key, void* val);
};

// @early-stop
// Control flow + 12-byte-stride index arithmetic + memmove shifts + global stores are
// all faithful, but MSVC pins val->ebx / key->edi where retail uses key->ebx / val->edi
// (callee-saved register-preference wall, docs/patterns/pin-local-for-callee-saved-reg.md);
// the ebx<->edi swap is not source-steerable and cascades through every store. Deferred.
RVA(0x0016e360, 0x11a)
void* Reg23::Add(void* key, void* val) {
    int count = g_recCount23;
    if (val != 0 && count >= 0x20)
        return 0;
    int idx;
    if (count != 0)
        idx = Find(key);
    else
        idx = -1;
    if (idx != -1) {
        void* old = g_recs23[idx].m_4;
        if (val != 0) {
            g_recs23[idx].m_4 = val;
            return old;
        }
        memmove(&g_recs23[m_4], &g_recs23[m_4 + 1], (g_recCount23 - m_4 - 1) * sizeof(Rec23));
        g_recCount23 = g_recCount23 - 1;
        return old;
    }
    if (val == 0)
        return 0;
    if (g_recCount23 != 0)
        memmove(&g_recs23[m_4 + 1], &g_recs23[m_4], (g_recCount23 - m_4) * sizeof(Rec23));
    g_recs23[m_4].m_4 = val;
    g_recs23[m_4].m_0 = key;
    g_recCount23 = g_recCount23 + 1;
    g_recs23[m_4].m_8 = 0;
    return 0;
}
