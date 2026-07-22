#include <rva.h>
#include <DDrawMgr/DDrawShadeBlit.h> // CDDrawShadeBlit - the REAL owner of Select @0x14dd90
#include <DDrawMgr/ShadeDescrTable.h> // own exported globals (ex Globals.h)

struct ShadeDescr;

DATA(0x002bf208)
ShadeDescr* g_shadeDescr208 = 0;
DATA(0x002bf20c)
ShadeDescr* g_shadeDescr20c = 0;
DATA(0x002bf210)
ShadeDescr* g_shadeDescr210 = 0;
DATA(0x002bf214)
ShadeDescr* g_shadeDescr214 = 0;
DATA(0x002bf21c)
ShadeDescr* g_shadeDescr21c = 0;
DATA(0x002bf220)
ShadeDescr* g_shadeDescr220 = 0;

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

// @early-stop
// Code bytes byte-exact (verified llvm-objdump base vs target: every byte pairs except
// the single jmpl table displacement); residual is the switch-jumptable-separate-comdat
// wall (MSVC $L table symbol vs delinker inline-at-fn+0x74).
RVA(0x0014dd90, 0x74)
void CDDrawShadeBlit::Select(i32 mode, ShadeDescr* descr) {
    m_drawType = mode;
    if (descr == 0) {
        switch (mode) {
            case 2:
                m_palDescr = g_shadeDescr208;
                break;
            case 3:
                m_palDescr = g_shadeDescr20c;
                break;
            case 4:
                m_palDescr = g_shadeDescr210;
                break;
            case 6:
                m_palDescr = g_shadeDescr214;
                break;
            case 7:
                m_palDescr = g_shadeDescr21c;
                break;
            case 10:
                m_palDescr = g_shadeDescr220;
                break;
            case 11:
                m_palDescr = g_shadeDescr220;
                break;
        }
    } else {
        m_palDescr = descr;
    }
}
