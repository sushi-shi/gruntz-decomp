#ifndef GRUNTZ_GRUNTZ_BATTLEZGRUNTMACROS_H
#define GRUNTZ_GRUNTZ_BATTLEZGRUNTMACROS_H

#define ARR_RECYCLE(g)                                                                             \
    if ((g)->CoordCount() != 0) {                                                                  \
        POSITION nd = (g)->CoordHead();                                                            \
        while (nd != 0) {                                                                          \
            POSITION cur = nd;                                                                     \
            (g)->m_coordList.GetNext(nd);                                                          \
            if (static_cast<Coord*>((g)->m_coordList.GetAt(cur)) != 0) {                           \
                g_coordPool.Push(static_cast<Coord*>((g)->m_coordList.GetAt(cur)));                \
            }                                                                                      \
        }                                                                                          \
        coordList->RemoveAll();                                                                    \
    }

#endif // GRUNTZ_GRUNTZ_BATTLEZGRUNTMACROS_H
