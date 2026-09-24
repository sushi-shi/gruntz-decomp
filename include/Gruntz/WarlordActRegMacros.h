#ifndef GRUNTZ_GRUNTZ_WARLORDACTREGMACROS_H
#define GRUNTZ_GRUNTZ_WARLORDACTREGMACROS_H

#define REGISTER_NAME(key)                                                                         \
    i32 id_ = ActFindId(key);                                                                      \
    if (id_ == 0) {                                                                                \
        ActInsertId(key, g_typeCounter);                                                           \
        id_ = g_typeCounter;                                                                       \
        g_typeColl[g_typeCounter] = key;                                                           \
        ++g_typeCounter;                                                                           \
    }

#define REGISTER_ACTION(key, handler)                                                              \
    do {                                                                                           \
        REGISTER_NAME(key)                                                                         \
        CActHandler& slot_ = CActRegPool<CWarlord>::s_table[id_];                                  \
        slot_ = static_cast<CActHandler>(handler);                                                 \
    } while (0)

#endif // GRUNTZ_GRUNTZ_WARLORDACTREGMACROS_H
