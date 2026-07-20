// StatusBarTabWidgets.h - the widget views the status-bar HOST's own builder TU
// (SBI_RectOnly.cpp: BuildStatusBarTabs / the tab builders) instantiates.
//
// SCOPE: included by that ONE TU. These are per-TU reconstruction views of the real
// SBI chain leaves, kept here (rather than in <Gruntz/StatusBarMgr.h>) precisely so
// the host class can be included by TUs that carry the REAL chain classes
// (<Gruntz/SBI_Image.h> / <Gruntz/SBI_MenuItem.h>) without a redefinition clash.
//
// @identity-TODO (pre-existing debt, NOT introduced by the split): CSbiRectSub IS the
// real CSBI_RectOnly and the CSBI_MenuItem below IS the real CSBI_MenuItem. The
// CSBI_RectOnly NAME is now free (the 2026-07-12 host split released it), so the only
// thing still blocking the fold onto the real chain is that this TU also carries
// <Gruntz/SbiTabzDialogViews.h>, which defines its OWN `class CSBI_Image` - so pulling
// in the real <Gruntz/SBI_Image.h> here would be a redefinition. Folding THAT view is
// the next step; it is a separate, measured change (SbiTabzDialogViews.h documents the
// base-ctor inline/out-of-line cost), not something to smuggle into the split.
#ifndef GRUNTZ_STATUSBARTABWIDGETS_H
#define GRUNTZ_STATUSBARTABWIDGETS_H

#include <Ints.h>
#include <rva.h>
#include <Gruntz/SbiConfig.h>
#include <Gruntz/SBI_MenuItem.h> // the CANONICAL CSBI_MenuItem : CSBI_Image : CSBI_RectOnly

#include <Gruntz/SBI_Image.h> // the canonical CSBI_RectOnly the tab builders `new`

#endif // GRUNTZ_STATUSBARTABWIDGETS_H
