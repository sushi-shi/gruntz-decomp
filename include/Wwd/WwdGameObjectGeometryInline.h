#ifndef WWD_WWDGAMEOBJECTGEOMETRYINLINE_H
#define WWD_WWDGAMEOBJECTGEOMETRYINLINE_H

#define APPLY_GEOMETRY_DIRECT(object, sprite, applyDefault)                                        \
    (object)->m_animCursor.Setup(sprite);                                                          \
    if (applyDefault) {                                                                            \
        (object)->m_animCursor.Advance(g_engineFrameDelta);                                        \
    }

#endif // WWD_WWDGAMEOBJECTGEOMETRYINLINE_H
