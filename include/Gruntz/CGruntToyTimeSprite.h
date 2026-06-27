// CGruntToyTimeSprite.h - the grunt toy-timer HUD sprite (C:\Proj\Gruntz), a
// CUserLogic leaf (vftables 0x5e705c / 0x5e70b4). Only the /GX leaf dtor is
// reconstructed here; offsets + code bytes are load-bearing.
#ifndef GRUNTZ_CGRUNTTOYTIMESPRITE_H
#define GRUNTZ_CGRUNTTOYTIMESPRITE_H

#include <rva.h>
#include <Gruntz/UserLogic.h>

class CGruntToyTimeSprite : public CUserLogic {
public:
    ~CGruntToyTimeSprite(); // 0x012130 (folds the CUserLogic teardown)
    char m_pad40[0x54 - 0x40];
};
SIZE(CGruntToyTimeSprite, 0x54);

#endif // GRUNTZ_CGRUNTTOYTIMESPRITE_H
