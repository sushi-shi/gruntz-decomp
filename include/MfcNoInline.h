#ifndef GRUNTZ_MFCNOINLINE_H
#define GRUNTZ_MFCNOINLINE_H

// <Mfc.h> with MFC's inline accessor bodies suppressed for EVERY compiler, not
// just clang - a per-TU codegen device, not an LSP workaround. A TU that
// includes this parses the rest of its headers against the out-of-line MFC
// accessors, which is what retail's objects show for those units.
//
// <Mfc.h> MUST come first and is not optional, for the same reason as in
// <MfcWin.h>: _AFX_ENABLE_INLINES is DEFINED by <afx.h>, so undefining it before
// afx.h has been parsed is a silent no-op. Relying on the include ORDER to have
// pulled <Mfc.h> already would leave this header's contract ambient - the
// canonical order does sort <Mfc.h> first, but a device that only works because
// of where it is sorted is not a device, it is a coincidence.

#include <Mfc.h>

#undef _AFX_ENABLE_INLINES

#endif // GRUNTZ_MFCNOINLINE_H
