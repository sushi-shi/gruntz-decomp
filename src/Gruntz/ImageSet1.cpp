#include <Gruntz/ImageSets.h>
#include <rva.h>

RVA(0x00161370, 0x7)
CImageSet1::~CImageSet1() {}

// CImageSet1::Parse (0x166d40, ??_7CImageSet1 slot +0x14). Copies three dwords
// from the WWD record at +0x08.. into m_04/m_08/m_0c via an advancing source
// pointer (retail's `add eax,8; mov (eax); add eax,4` cursor walk) and returns TRUE.
// @early-stop
// tail-peephole wall (same as CImageSet2/3): retail keeps the 2nd store's `add eax,4`
// then reads the 3rd via [eax]; cl folds that advance into the 3rd's +4 displacement.
// The advancing-cursor prologue + first read are byte-exact; the final fold is the
// documented entropy-tail wall (docs/patterns/header-fields-through-cursor-not-index.md).
RVA(0x00166d40, 0x24)
i32 CImageSet1::Parse(void* record) {
    i32* p = reinterpret_cast<i32*>((reinterpret_cast<char*>(record) + 8));
    m_04 = *p++;
    m_08 = *p++;
    m_0c = *p++;
    return 1;
}
