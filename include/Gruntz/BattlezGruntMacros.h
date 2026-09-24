#ifndef GRUNTZ_GRUNTZ_BATTLEZGRUNTMACROS_H
#define GRUNTZ_GRUNTZ_BATTLEZGRUNTMACROS_H

#define BATTLEZ_ACT_DIFFERS_FROM_IGLPJCR(unit, result)                                             \
    (!(result = HasAnimationActName(unit, "I")) && !(result = HasAnimationActName(unit, "G"))      \
     && !(result = HasAnimationActName(unit, "L")) && !(result = HasAnimationActName(unit, "P"))   \
     && !(result = HasAnimationActName(unit, "J")) && !(result = HasAnimationActName(unit, "C"))   \
     && !(result = HasAnimationActName(unit, "R")))

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
