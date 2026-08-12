#ifndef INCLUDE_GRUNTZ_SCANGRIDMACROS_H
#define INCLUDE_GRUNTZ_SCANGRIDMACROS_H

#include <MfcWin.h>

#include <Gruntz/CoordNode.h>
#include <Gruntz/FreeNodePool.h>

// The grunt scan/step compilands share these force-inline devices; MSVC 5.0
// macros are the faithful model of the always-inlined expansion.

#define GRID_BOUNDS(grid)                                                                          \
    {                                                                                              \
        CRect ra(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        CRect rb(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        ra = rb;                                                                                   \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

#define RECYCLE_COORDS(head)                                                                       \
    {                                                                                              \
        CoordNode* n = (head);                                                                     \
        while (n != 0) {                                                                           \
            CoordNode* next = n->m_next;                                                           \
            void* pay = n->m_coord;                                                                \
            if (pay != 0) {                                                                        \
                CoordPoolNode* slot = g_coordPool.NodeOf(pay);                                     \
                slot->m_next = g_coordPool.m_freeHead;                                             \
                g_coordPool.m_freeHead = slot;                                                     \
            }                                                                                      \
            n = next;                                                                              \
        }                                                                                          \
    }

#define GRID_RECT_BOUNDS(grid)                                                                     \
    {                                                                                              \
        CRect ra(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        CRect rb(0, 0, (grid)->m_width, (grid)->m_height);                                         \
        ra = rb;                                                                                   \
        if (!IntersectRect(&(grid)->m_bounds, &ra, &rb)) {                                         \
            (grid)->m_bounds = ra;                                                                 \
        }                                                                                          \
        (grid)->m_gridW = (grid)->m_bounds.right - (grid)->m_bounds.left;                          \
        (grid)->m_gridH = (grid)->m_bounds.bottom - (grid)->m_bounds.top;                          \
    }

// CMapMgr::Clip(src) expanded in place -- the shape cl emits when it inlines the
// 0x2b340 body with a non-constant src: the (0,0,w,h) rect is built by the
// out-of-line CRect ctor, the src rect is copied and its right/bottom bumped,
// and the NULL arm re-runs the ctor into a temporary.
#define GRID_CLIP(grid, srcRect)                                                                   \
    {                                                                                              \
        const RECT* clipSrc = (srcRect);                                                           \
        RECT rb;                                                                                   \
        static_cast<RECT*>(new (&rb) CRect(0, 0, (grid)->m_width, (grid)->m_height));              \
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

// Same expansion as GRID_CLIP, but at the sites where cl builds the (0,0,w,h) rect
// with field stores instead of the out-of-line CRect ctor; only the NULL arm's
// temporary keeps its ctor call. The bounds rect is reached through one pointer:
// retail reads back m_bounds.right/left/bottom/top through the same register it
// handed IntersectRect, never re-deriving them from the grid.
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

// Same expansion with field stores in both the bounds rect and the NULL arm.
// StepBrickLayerBehavior's retail relocation stream contains no CRect constructor
// at this site; its NULL arm writes the four fields directly.
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

// CMapMgr::Clip(NULL) expanded in place: the constant-NULL source folds the
// `src != NULL` arm away, so both rects are built by the out-of-line CRect ctor
// -- rb directly, ra by assignment from a second, temporary CRect (a four-field
// copy off the ctor's return register).  m_bounds is reached through one pointer.
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

#define DRAIN_COORDS()                                                                             \
    if (CoordCount() != 0) {                                                                       \
        POSITION dpos = m_coordList.GetHeadPosition();                                             \
        while (dpos != 0) {                                                                        \
            Coord* cur = static_cast<Coord*>(m_coordList.GetNext(dpos));                           \
            if (cur != 0) {                                                                        \
                g_coordPool.Push(cur);                                                             \
            }                                                                                      \
        }                                                                                          \
        m_coordList.RemoveAll();                                                                   \
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
