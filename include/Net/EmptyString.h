// EmptyString.h - the shared empty C-string literal owned by NetMgrReportError.cpp
// (?g_emptyString / _g_emptyString, DATA()-bound at 0x2293f4 there). A NARROW, owner-only
// decl header: included solely by NetMgrReportError.cpp so the definition can drop the
// `extern "C"` keyword while keeping the exact C-linkage symbol + DATA() binding. Used as
// a pointer -> byte-neutral. (Two consumers - AttractState/DDrawSubMgr - reference it via
// a divergent C++-linkage `extern char g_emptyString[]`; those are left untouched.)
#ifndef NET_EMPTYSTRING_H
#define NET_EMPTYSTRING_H

extern "C" char g_emptyString[]; // 0x6293f4  ""

#endif // NET_EMPTYSTRING_H
