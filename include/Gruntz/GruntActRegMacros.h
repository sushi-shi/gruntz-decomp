#ifndef GRUNTZ_GRUNTZ_GRUNTACTREGMACROS_H
#define GRUNTZ_GRUNTZ_GRUNTACTREGMACROS_H

#define BIND_GRUNT_ACT_RAW(id, handler)                                                            \
    {                                                                                              \
        GruntActPmf _p;                                                                            \
        _p.m_pmf = (handler);                                                                      \
        /* The stored generic CUserLogic PMF is reached through retail's raw */                    \
        /* _zvec accessor; the typed view exists only at this ABI seam. */                         \
        *CActReg::AsElem(CActRegPool<CGrunt>::s_table._zvec::IndexToPtr(id)) = _p.m_h;             \
    }

#define BIND_GRUNT_ACT_TYPED(id, handler)                                                          \
    {                                                                                              \
        GruntActPmf _p;                                                                            \
        _p.m_pmf = (handler);                                                                      \
        *CActRegPool<CGrunt>::s_table.Resolve(id) = _p.m_h;                                        \
    }

#define REGISTER_GRUNT_ACT_KEY_IMPL(key, handler, bind)                                            \
    {                                                                                              \
        i32 id = ActFindId(key);                                                                   \
        if (id == 0) {                                                                             \
            ActInsertId(key, g_typeCounter);                                                       \
            id = g_typeCounter;                                                                    \
            /* Keep the global counter as the name-slot lookup's direct argument; */               \
            /* using id changes MSVC's two-consumer CSE and register allocation. */                \
            /* See docs/patterns/act-registrar-counter-cse-and-freeloop.md. */                     \
            CString* slot = g_typeColl.ScratchResolve(g_typeCounter);                              \
            i32 n = g_typeColl.m_grown;                                                            \
            CString* list = ActNameSlots();                                                        \
            while (n-- != 0) {                                                                     \
                if (list != 0) {                                                                   \
                    list->CString::CString();                                                      \
                }                                                                                  \
                list++;                                                                            \
            }                                                                                      \
            *slot = (key);                                                                         \
            g_typeCounter++;                                                                       \
        }                                                                                          \
        bind(id, handler);                                                                         \
    }

#define REGISTER_GRUNT_ACT_KEY(key, handler)                                                       \
    REGISTER_GRUNT_ACT_KEY_IMPL(key, handler, BIND_GRUNT_ACT_RAW)

#define REGISTER_GRUNT_ACT_KEY_TYPED(key, handler)                                                 \
    REGISTER_GRUNT_ACT_KEY_IMPL(key, handler, BIND_GRUNT_ACT_TYPED)

#define REGISTER_GRUNT_ACT_KEY_DERIVED(key, handler)                                               \
    {                                                                                              \
        i32 id = ActFindId(key);                                                                   \
        if (id == 0) {                                                                             \
            ActInsertId(key, g_typeCounter);                                                       \
            id = g_typeCounter;                                                                    \
            *g_typeColl.SlotOf(id) = (key);                                                        \
            g_typeCounter++;                                                                       \
        }                                                                                          \
        BIND_GRUNT_ACT_TYPED(id, handler);                                                         \
    }

#endif // GRUNTZ_GRUNTZ_GRUNTACTREGMACROS_H
