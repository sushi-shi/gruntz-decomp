#ifndef GRUNTZ_GRUNTZ_WARLORDACTREGMACROS_H
#define GRUNTZ_GRUNTZ_WARLORDACTREGMACROS_H

#include <Gruntz/ActNameRegistry.h>

#define REGISTER_ACTION(key, handler)                                                              \
    do {                                                                                           \
        ACT_NAME_ID(id_, key)                                                                      \
        CActHandler& slot_ = CActRegPool<CWarlord>::s_table[id_];                                  \
        slot_ = static_cast<CActHandler>(handler);                                                 \
    } while (0)

#endif // GRUNTZ_GRUNTZ_WARLORDACTREGMACROS_H
