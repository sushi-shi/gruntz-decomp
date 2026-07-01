// ShadeDescrTable.cpp - the global ShadeDescr* selector pair (RVAs 0x14dcf0 /
// 0x14dd90), trace-attributed to ClassUnknown_2 / ClassUnknown_11. 0x14dcf0 is a
// __stdcall setter that stashes a descriptor pointer into one of the seven global
// ShadeDescr* slots (0x6bf208..0x6bf220, with g_blendDescr the named one at
// 0x6bf218) keyed by a mode id; 0x14dd90 is the __thiscall selector that copies a
// caller-supplied descriptor (or, when null, the mode's global default) into the
// object's +0x1c field.
#include <rva.h>

struct ShadeDescr;

// Seven global ShadeDescr* slots; only +0x6bf218 carries a real RTTI name.
DATA(0x002bf208)
extern ShadeDescr* g_shadeDescr208;
DATA(0x002bf20c)
extern ShadeDescr* g_shadeDescr20c;
DATA(0x002bf210)
extern ShadeDescr* g_shadeDescr210;
DATA(0x002bf214)
extern ShadeDescr* g_shadeDescr214;
DATA(0x002bf21c)
extern ShadeDescr* g_shadeDescr21c;
DATA(0x002bf220)
extern ShadeDescr* g_shadeDescr220;
extern ShadeDescr* g_blendDescr; // 0x6bf218

// @early-stop
// Code bytes byte-exact (all 8 global stores + reloc-named globals pair); residual is
// the switch-jumptable-separate-comdat wall — MSVC emits the jump table as a separate
// $L symbol, the delinker inlines it at fn+0x6c, so only the jmpl table reloc differs.
RVA(0x0014dcf0, 0x69)
void SetShadeDescr(ShadeDescr* v, int mode) {
    switch (mode) {
        case 2:
            g_shadeDescr208 = v;
            break;
        case 3:
            g_shadeDescr20c = v;
            break;
        case 4:
            g_shadeDescr210 = v;
            break;
        case 6:
            g_shadeDescr214 = v;
            break;
        case 7:
            g_shadeDescr21c = v;
            break;
        case 10:
            g_shadeDescr220 = v;
            break;
        case 11:
            g_shadeDescr220 = v;
            break;
        case 9:
            g_blendDescr = v;
            break;
    }
}

class ShadeSelector {
public:
    char m_pad[0x14];
    int m_14;
    int m_18;
    ShadeDescr* m_1c;
    void Select(int mode, ShadeDescr* descr);
};

// @early-stop
// Code bytes byte-exact (verified llvm-objdump base vs target: every byte pairs except
// the single jmpl table displacement); residual is the switch-jumptable-separate-comdat
// wall (MSVC $L table symbol vs delinker inline-at-fn+0x74).
RVA(0x0014dd90, 0x74)
void ShadeSelector::Select(int mode, ShadeDescr* descr) {
    m_14 = mode;
    if (descr == 0) {
        switch (mode) {
            case 2:
                m_1c = g_shadeDescr208;
                break;
            case 3:
                m_1c = g_shadeDescr20c;
                break;
            case 4:
                m_1c = g_shadeDescr210;
                break;
            case 6:
                m_1c = g_shadeDescr214;
                break;
            case 7:
                m_1c = g_shadeDescr21c;
                break;
            case 10:
                m_1c = g_shadeDescr220;
                break;
            case 11:
                m_1c = g_shadeDescr220;
                break;
        }
    } else {
        m_1c = descr;
    }
}
SIZE_UNKNOWN(ShadeSelector);
