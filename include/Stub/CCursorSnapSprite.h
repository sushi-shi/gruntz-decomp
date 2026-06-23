#ifndef GRUNTZ_STUB_CCURSORSNAPSPRITE_H
#define GRUNTZ_STUB_CCURSORSNAPSPRITE_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CCursorSnapSprite : CUserLogic (RTTI). sizeof 0x54.
class CCursorSnapSprite : public CUserLogic {
public:
    CCursorSnapSprite(int);
    char m_size_pad[0x14]; // own region over CUserLogic (0x40)
};
SIZE(CCursorSnapSprite, 0x54);
#endif
