// EmptyString.h - the engine's shared empty-string constant.
//
// g_emptyString (?_g_emptyString@ / _g_emptyString @0x2293f4) is a single `char[1] = ""`
// datum handed out tree-wide as the "" default for CString/char* fields (dialog text,
// bute tag names, net/save scratch, sound labels, ...). Its ONE definition + DATA pin
// live in src/Net/NetMgrReportError.cpp; it is C linkage (extern "C") so its name is
// reloc-masked in objdiff. Declared once here so consumers stop re-`extern`-ing it per-TU
// under scattered header homes.
#ifndef INCLUDE_EMPTYSTRING_H
#define INCLUDE_EMPTYSTRING_H

extern "C" char g_emptyString[]; // 0x2293f4  "" (defined in NetMgrReportError.cpp)

#endif // INCLUDE_EMPTYSTRING_H
