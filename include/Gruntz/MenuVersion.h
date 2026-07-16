// MenuVersion.h - the app version-banner fields owned by MenuState.cpp (CMenuState::
// BuildVersionString reads them; each DATA()-bound in MenuState.cpp). A NARROW, owner-
// only decl header: included solely by MenuState.cpp so the definitions can drop the
// `extern "C"` keyword while keeping the exact C-linkage symbol + DATA() binding, WITHOUT
// putting these file-private globals into the widely-shared <Gruntz/GameMode.h> (which
// would ripple regalloc in unrelated GameMode.h includers via the decl-count butterfly).
// Mutable .bss ints -> byte-neutral.
#ifndef GRUNTZ_MENUVERSION_H
#define GRUNTZ_MENUVERSION_H

#include <Ints.h>

extern "C" i32 g_versionMajor; // 0x651608
extern "C" i32 g_versionMid;   // 0x65160c  (selects 2- vs 3-number format)
extern "C" i32 g_versionMinor; // 0x651610

#endif // GRUNTZ_MENUVERSION_H
