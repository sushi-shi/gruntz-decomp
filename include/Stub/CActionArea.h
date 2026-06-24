#ifndef GRUNTZ_STUB_CACTIONAREA_H
#define GRUNTZ_STUB_CACTIONAREA_H
#include <rva.h>
#include <Stub/CUserLogic.h>
// CActionArea : CUserLogic (RTTI). sizeof 0x68.
class CActionArea : public CUserLogic {
public:
    CActionArea(i32);
    char m_size_pad[0x28]; // own region over CUserLogic (0x40)
};
SIZE(CActionArea, 0x68);
#endif
