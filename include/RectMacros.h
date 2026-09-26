#ifndef GRUNTZ_RECTMACROS_H
#define GRUNTZ_RECTMACROS_H

#define SET_RECT_COMPONENTS(rect, l, t, r, b)                                                      \
    (rect).left = (l);                                                                             \
    (rect).top = (t);                                                                              \
    (rect).right = (r);                                                                            \
    (rect).bottom = (b)
#define RECT_WIDTH(rect) ((rect).right - (rect).left)
#define RECT_HEIGHT(rect) ((rect).bottom - (rect).top)
#define NORMALIZE_RECT_COMPONENTS(rect)                                                            \
    if ((rect).right < (rect).left) {                                                              \
        i32 t = (rect).left;                                                                       \
        (rect).left = (rect).right;                                                                \
        (rect).right = t;                                                                          \
    }                                                                                              \
    if ((rect).bottom < (rect).top) {                                                              \
        i32 t = (rect).top;                                                                        \
        (rect).top = (rect).bottom;                                                                \
        (rect).bottom = t;                                                                         \
    }
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
#define SET_POINT_COMPONENTS(point, xValue, yValue)                                                \
    (point).x = (xValue);                                                                          \
    (point).y = (yValue)
#define RECT_CENTER_X(rect) ((rect).left + ((rect).right - (rect).left) / 2)
#define RECT_CENTER_Y(rect) ((rect).top + ((rect).bottom - (rect).top) / 2)
#define DEFLATE_RECT_X(rect, amount)                                                               \
    (rect).left += (amount);                                                                       \
    (rect).right -= (amount)
#define DEFLATE_RECT_Y(rect, amount)                                                               \
    (rect).top += (amount);                                                                        \
    (rect).bottom -= (amount)

#define EXTEND_RECT_MAX(rect, dx, dy)                                                              \
    (rect).right += (dx);                                                                          \
    (rect).bottom += (dy)
#define SIZE_EQUALS_COMPONENTS(size, width, height) ((size).cx == (width) && (size).cy == (height))
#endif // GRUNTZ_RECTMACROS_H
