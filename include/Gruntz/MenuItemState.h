#ifndef GRUNTZ_GRUNTZ_MENUITEMSTATE_H
#define GRUNTZ_GRUNTZ_MENUITEMSTATE_H

#include <Enums.h>

// The state of a main-menu item (CMenuItem), named by retail's own members:
// CAnimatedMenuItem::GetStateAnimation switches on it and returns
// m_normalAnimation, m_selectedAnimation and m_disabledAnimation respectively.
//
// Corroborated by the initialiser, which picks DISABLED or NORMAL off the item's
// own flag bit (`m_state = (m_flags & 1) ? 3 : 1`), and by CMenuPage's focus
// walk, which accepts an item when its state is NORMAL or SELECTED - i.e.
// anything but disabled.
//
// Like SbiMenuItemState, the value doubles as the sprite's frame index: the draw
// path feeds it straight to page->m_items.GetAt(). It is a DIFFERENT domain from
// that one though - different class, and a different set (1..3 against 0..4).
GZ_ENUM_BEGIN(MenuItemState)
    MENUSTATE_NORMAL = 1,
    MENUSTATE_SELECTED = 2,
    MENUSTATE_DISABLED = 3
GZ_ENUM_END(MenuItemState)

#endif // GRUNTZ_GRUNTZ_MENUITEMSTATE_H
