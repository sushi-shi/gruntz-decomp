// CString.h - the MFC CString (statically-linked NAFXCW). Reconstructed only as
// deeply as the byte-matches need: a single char* at +0x00; the method bodies
// live in NAFXCW so their `call rel32` displacements reloc-mask in objdiff - only
// the exact mangled symbol + arg shape are load-bearing. One header, #included
// wherever a TU uses CString, instead of re-declaring it inline in every TU.
#ifndef SRC_INCS_CSTRING_H
#define SRC_INCS_CSTRING_H

class CString {
public:
    CString(const char *s);
    CString(const CString &o);
    ~CString();
    char *m_pchData;             // +0x00
};

#endif  // SRC_INCS_CSTRING_H
