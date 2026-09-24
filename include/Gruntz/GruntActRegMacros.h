#ifndef GRUNTZ_GRUNTZ_GRUNTACTREGMACROS_H
#define GRUNTZ_GRUNTZ_GRUNTACTREGMACROS_H

// Registered methods have zero adjustment to the primary CUserLogic base.
#define ToActHandler(handler) static_cast<CActHandler>(handler)

#define STORE_GRUNT_ACT(registry, id, handler)                                                     \
    {                                                                                              \
        CActHandler& slot = (registry)[id];                                                        \
        slot = (handler);                                                                          \
    }

#define BIND_GRUNT_ACT(registry, id, handler)                                                      \
    {                                                                                              \
        CActHandler converted = ToActHandler(handler);                                             \
        STORE_GRUNT_ACT(registry, id, converted);                                                  \
    }

#define REGISTER_GRUNT_ACT_KEY(registry, key, handler)                                             \
    {                                                                                              \
        i32 id = ActFindId(key);                                                                   \
        if (id == 0) {                                                                             \
            ActInsertId(key, g_typeCounter);                                                       \
            id = g_typeCounter;                                                                    \
            g_typeColl[g_typeCounter] = (key);                                                     \
            g_typeCounter++;                                                                       \
        }                                                                                          \
        BIND_GRUNT_ACT(registry, id, handler);                                                     \
    }

#endif // GRUNTZ_GRUNTZ_GRUNTACTREGMACROS_H
