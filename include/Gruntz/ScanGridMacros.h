#ifndef INCLUDE_GRUNTZ_SCANGRIDMACROS_H
#define INCLUDE_GRUNTZ_SCANGRIDMACROS_H

#include <MfcWin.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/FreeNodePool.h>

#define GRID_CLIP(grid, srcRect)                                                                   \
    {                                                                                              \
        const RECT* clipSrc = (srcRect);                                                           \
        CRect rb(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        RECT ra;                                                                                   \
        if (clipSrc != NULL) {                                                                     \
            ra = *clipSrc;                                                                         \
            ra.right = ra.right + 1;                                                               \
            ra.bottom = ra.bottom + 1;                                                             \
        } else {                                                                                   \
            ra = CRect(0, 0, (grid)->m_width, (grid)->m_height);                                   \
        }                                                                                          \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

#define GRID_CLIP_INL(grid, srcRect)                                                               \
    {                                                                                              \
        const RECT* clipSrc = (srcRect);                                                           \
        RECT rb;                                                                                   \
        rb.left = 0;                                                                               \
        rb.top = 0;                                                                                \
        rb.right = (grid)->m_width;                                                                \
        rb.bottom = (grid)->m_height;                                                              \
        RECT ra;                                                                                   \
        if (clipSrc != NULL) {                                                                     \
            ra = *clipSrc;                                                                         \
            ra.right = ra.right + 1;                                                               \
            ra.bottom = ra.bottom + 1;                                                             \
        } else {                                                                                   \
            ra = CRect(0, 0, (grid)->m_width, (grid)->m_height);                                   \
        }                                                                                          \
        RECT* clipBounds = &(grid)->m_bounds;                                                      \
        if (!IntersectRect(clipBounds, &ra, &rb)) {                                                \
            *clipBounds = ra;                                                                      \
        }                                                                                          \
        (grid)->m_gridW = clipBounds->right - clipBounds->left;                                    \
        (grid)->m_gridH = clipBounds->bottom - clipBounds->top;                                    \
    }

#define GRID_CLIP_INL_FIELDS(grid, srcRect)                                                        \
    {                                                                                              \
        const RECT* clipSrc = (srcRect);                                                           \
        RECT rb;                                                                                   \
        rb.left = 0;                                                                               \
        rb.top = 0;                                                                                \
        rb.right = (grid)->m_width;                                                                \
        rb.bottom = (grid)->m_height;                                                              \
        RECT ra;                                                                                   \
        if (clipSrc != NULL) {                                                                     \
            ra = *clipSrc;                                                                         \
            ra.right = ra.right + 1;                                                               \
            ra.bottom = ra.bottom + 1;                                                             \
        } else {                                                                                   \
            ra.left = 0;                                                                           \
            ra.top = 0;                                                                            \
            ra.right = (grid)->m_width;                                                            \
            ra.bottom = (grid)->m_height;                                                          \
        }                                                                                          \
        RECT* clipBounds = &(grid)->m_bounds;                                                      \
        if (!IntersectRect(clipBounds, &ra, &rb)) {                                                \
            *clipBounds = ra;                                                                      \
        }                                                                                          \
        (grid)->m_gridW = clipBounds->right - clipBounds->left;                                    \
        (grid)->m_gridH = clipBounds->bottom - clipBounds->top;                                    \
    }

#define GRID_CLIP_NULL(grid)                                                                       \
    {                                                                                              \
        CRect rb(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        RECT ra;                                                                                   \
        ra = CRect(0, 0, (grid)->m_width, (grid)->m_height);                                       \
        RECT* clipBounds = &(grid)->m_bounds;                                                      \
        if (!IntersectRect(clipBounds, &ra, &rb)) {                                                \
            *clipBounds = ra;                                                                      \
        }                                                                                          \
        (grid)->m_gridW = clipBounds->right - clipBounds->left;                                    \
        (grid)->m_gridH = clipBounds->bottom - clipBounds->top;                                    \
    }

#define GRID_RECT_INLINE(grid)                                                                     \
    {                                                                                              \
        RECT ra;                                                                                   \
        ra.left = 0;                                                                               \
        ra.top = 0;                                                                                \
        ra.right = (grid)->m_width;                                                                \
        ra.bottom = (grid)->m_height;                                                              \
        RECT rb;                                                                                   \
        rb.left = 0;                                                                               \
        rb.top = 0;                                                                                \
        rb.right = (grid)->m_width;                                                                \
        rb.bottom = (grid)->m_height;                                                              \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

#define GRID_RECT_INLINE_LOCAL(grid)                                                               \
    {                                                                                              \
        RECT ra;                                                                                   \
        ra.left = 0;                                                                               \
        ra.top = 0;                                                                                \
        ra.right = (grid)->m_width;                                                                \
        ra.bottom = (grid)->m_height;                                                              \
        RECT rb;                                                                                   \
        rb.left = 0;                                                                               \
        rb.top = 0;                                                                                \
        rb.right = (grid)->m_width;                                                                \
        rb.bottom = (grid)->m_height;                                                              \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = ra.right - ra.left;                                                      \
        (grid)->m_gridH = ra.bottom - ra.top;                                                      \
    }

#define PRIO(dst, r)                                                                               \
    switch (r) {                                                                                   \
        case PICKUP_BOMB:                                                                          \
            dst = 2;                                                                               \
            break;                                                                                 \
        case PICKUP_WELDER:                                                                        \
            dst = 3;                                                                               \
            break;                                                                                 \
        case PICKUP_SWORD:                                                                         \
            dst = 4;                                                                               \
            break;                                                                                 \
        case PICKUP_GUNHAT:                                                                        \
            dst = 5;                                                                               \
            break;                                                                                 \
        case PICKUP_CLUB:                                                                          \
            dst = 6;                                                                               \
            break;                                                                                 \
        case PICKUP_ROCK:                                                                          \
            dst = 7;                                                                               \
            break;                                                                                 \
        case PICKUP_SHOVEL:                                                                        \
            dst = 8;                                                                               \
            break;                                                                                 \
        case PICKUP_BOOMERANG:                                                                     \
            dst = 9;                                                                               \
            break;                                                                                 \
        case PICKUP_SPRING:                                                                        \
            dst = 10;                                                                              \
            break;                                                                                 \
        case PICKUP_GAUNTLETZ:                                                                     \
            dst = 11;                                                                              \
            break;                                                                                 \
        case PICKUP_WINGZ:                                                                         \
            dst = 12;                                                                              \
            break;                                                                                 \
        case PICKUP_SPY:                                                                           \
            dst = 13;                                                                              \
            break;                                                                                 \
        case PICKUP_BRICK:                                                                         \
            dst = 14;                                                                              \
            break;                                                                                 \
        case PICKUP_GRAVITYBOOTZ:                                                                  \
            dst = 15;                                                                              \
            break;                                                                                 \
        case PICKUP_SHIELD:                                                                        \
            dst = 16;                                                                              \
            break;                                                                                 \
        case PICKUP_GOOBER:                                                                        \
            dst = 17;                                                                              \
            break;                                                                                 \
        case PICKUP_TOOB:                                                                          \
            dst = 18;                                                                              \
            break;                                                                                 \
        case PICKUP_GLOVEZ:                                                                        \
            dst = 19;                                                                              \
            break;                                                                                 \
        case PICKUP_TIMEBOMB:                                                                      \
            dst = 20;                                                                              \
            break;                                                                                 \
        case PICKUP_NERFGUN:                                                                       \
            dst = 21;                                                                              \
            break;                                                                                 \
        case PICKUP_WAND:                                                                          \
            dst = 22;                                                                              \
            break;                                                                                 \
        default:                                                                                   \
            dst = 23;                                                                              \
            break;                                                                                 \
    }

#endif // INCLUDE_GRUNTZ_SCANGRIDMACROS_H
