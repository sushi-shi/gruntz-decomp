#include <rva.h>
#include <Ints.h>

#include <Gruntz/SbiDtorChain.h>
// SBI_WarlordHeadEh.cpp - the /GX EH-framed CSBI_WarlordHead destructor
// (C:\Proj\Gruntz). The split off the frameless base TU is matching-neutral (each
// function is RVA-keyed). Chain: CSBI_WarlordHead : CSBI_ImageSet : CSBI_Image :
// CSBI_RectOnly : CStatusBarItem (the four bases carried by SbiDtorChain.h).

struct CSBI_WarlordHead : CSBI_ImageSet {
    virtual ~CSBI_WarlordHead();
    void DtorReset(); // 0xe7400  most-derived member teardown (reloc-masked)
};

RVA(0x00104a00, 0x94)
CSBI_WarlordHead::~CSBI_WarlordHead() {
    DtorReset();
}
