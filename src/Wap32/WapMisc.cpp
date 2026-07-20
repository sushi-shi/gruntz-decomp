#include <rva.h>
#include <Ints.h>

#include <Font/Font.h> // real Font (the g_largeFont global)
#include <Wap32/ZVec.h>
#include <Wap32/NoTrackObjectStamp.h> // canonical CNoTrackObjectStamp (the vptr-stamp leaf)

RVA(0x0011d100, 0x7)
CNoTrackObjectStamp::~CNoTrackObjectStamp() {}

RVA(0x001155b0, 0xa)
void Unmatched_1155b0() {
    g_largeFont.Font::Font();
}
