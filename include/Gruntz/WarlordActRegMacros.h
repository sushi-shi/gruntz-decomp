#ifndef GRUNTZ_GRUNTZ_WARLORDACTREGMACROS_H
#define GRUNTZ_GRUNTZ_WARLORDACTREGMACROS_H

#define REGISTER_NAME(key)                                                                         \
    i32 id_ = ActFindId(key);                                                                      \
    if (id_ == 0) {                                                                                \
        ActInsertId(key, g_typeCounter);                                                           \
        id_ = g_typeCounter;                                                                       \
        CString* slot_ = g_typeColl.ScratchResolve(g_typeCounter);                                 \
        CString* p_ = g_typeColl.Slots();                                                          \
        for (i32 n_ = g_typeColl.m_grown; n_--; p_++) {                                            \
            ::new (static_cast<void*>(p_)) CString;                                                \
        }                                                                                          \
        *slot_ = key;                                                                              \
        ++g_typeCounter;                                                                           \
    }

#define REGISTER_ACTION(key, handler)                                                              \
    do {                                                                                           \
        REGISTER_NAME(key)                                                                         \
        /* Language-forced member-function representation seam; the byte accessor */               \
        /* returns to CActHandler only here. */                                                    \
        *reinterpret_cast<CActHandler*>(CActRegPool<CWarlord>::s_table._zvec::IndexToPtr(id_)) =   \
            static_cast<CActHandler>(handler);                                                     \
    } while (0)

#define REGISTER_ACTION_TYPED(key, handler)                                                        \
    do {                                                                                           \
        REGISTER_NAME(key)                                                                         \
        *CActRegPool<CWarlord>::s_table.Resolve(id_) = static_cast<CActHandler>(handler);          \
    } while (0)

#endif // GRUNTZ_GRUNTZ_WARLORDACTREGMACROS_H
