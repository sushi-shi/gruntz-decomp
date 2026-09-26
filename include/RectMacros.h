#ifndef GRUNTZ_RECTMACROS_H
#define GRUNTZ_RECTMACROS_H

#define SET_RECT_COMPONENTS(rect, l, t, r, b)                                                      \
    (rect).left = (l);                                                                             \
    (rect).top = (t);                                                                              \
    (rect).right = (r);                                                                            \
    (rect).bottom = (b)
#define SET_RECT_XY_EXTENTS(rect, l, r, t, b)                                                      \
    (rect).left = (l);                                                                             \
    (rect).right = (r);                                                                            \
    (rect).top = (t);                                                                              \
    (rect).bottom = (b)
#define SET_SIZE_COMPONENTS(size, w, h)                                                            \
    (size).cx = (w);                                                                               \
    (size).cy = (h)
#define OFFSET_RECT_COMPONENTS(rect, dx, dy)                                                       \
    (rect).left += (dx);                                                                           \
    (rect).top += (dy);                                                                            \
    (rect).right += (dx);                                                                          \
    (rect).bottom += (dy)
#define OFFSET_RECT_X_EDGES(rect, leftDelta, rightDelta)                                           \
    (rect).left += (leftDelta);                                                                    \
    (rect).right += (rightDelta)
#define OFFSET_RECT_Y_EDGES(rect, topDelta, bottomDelta)                                           \
    (rect).top += (topDelta);                                                                      \
    (rect).bottom += (bottomDelta)

#endif // GRUNTZ_RECTMACROS_H
