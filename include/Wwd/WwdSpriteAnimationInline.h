#ifndef WWD_WWDSPRITEANIMATIONINLINE_H
#define WWD_WWDSPRITEANIMATIONINLINE_H

#define SET_ANIMATION_AND_MAYBE_ADVANCE(object, animation, advanceImmediately)                     \
    (object)->m_animationCursor.SetAnimation(animation);                                           \
    if (advanceImmediately) {                                                                      \
        (object)->m_animationCursor.Advance(g_engineFrameDelta);                                   \
    }

#endif // WWD_WWDSPRITEANIMATIONINLINE_H
