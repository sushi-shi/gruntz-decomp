#ifndef GRUNTZ_GRUNTZ_ERRORSTRINGID_H
#define GRUNTZ_GRUNTZ_ERRORSTRINGID_H

#include <Enums.h>

// The STRING-TABLE resource id CGameApp::ReportError takes as its first
// argument. CGruntzApp::ShowError is what says so: it stores the argument in
// m_errorCode and then feeds it straight to
// `LoadStringA(m_hInstance, id, g_errorText, 0xfa)`, falling back to
// IDS_DEFAULT_ERROR when that fails.
//
// KNOWN CONFLATION, recorded here rather than papered over. Roughly 100
// ReportError call sites currently spell this argument with GruntzCommandId
// names - CMD_NEW_GAME, CMD_TOGGLE_MUSIC, TRIGERR_* - because the two id spaces
// overlap in 0x80xx. They are NOT the same domain, and the collision below
// proves it: IDS_DEFAULT_ERROR is 0x8009, which GruntzCommandId.h also calls
// CMD_TOGGLE_SOUND. "Toggle sound" is not the default error message; the two
// spaces simply happen to share numbers. Unwinding those call sites needs the
// binary's string table read out, so it is one job, not a hundred - until then
// this header holds only what retail itself named.
//
// The SECOND argument of ReportError is a different thing again and needs no
// domain at all: ShowError prints it verbatim with `sprintf(detail, "(%i)",
// detailVal)` and appends it to the message. It is a per-call-site tag whose
// only job is to be unique, which is why its ~113 values (0x141 .. 0x1232) mean
// nothing individually and can never be named.
GZ_ENUM_BEGIN(ErrorStringId)
    IDS_DEFAULT_ERROR = 0x8009
GZ_ENUM_END(ErrorStringId)

#endif // GRUNTZ_GRUNTZ_ERRORSTRINGID_H
