// CString.h - the MFC CString (statically-linked NAFXCW). Reconstructed only as
// deeply as the byte-matches need: a single char* at +0x00; the method bodies
// live in NAFXCW so their `call rel32` displacements reloc-mask in objdiff - only
// the exact mangled symbol + arg shape are load-bearing. One header, #included
// wherever a TU uses CString, instead of re-declaring it inline in every TU.
// (Some TUs previously called this placeholder "AfxString"; same NAFXCW class.)
#ifndef GRUNTZ_GRUNTZ_CSTRING_H
#define GRUNTZ_GRUNTZ_CSTRING_H

class CString {
public:
    CString();
    CString(const char *s);
    CString(const CString &o);
    ~CString();
    const CString &operator=(const char *src);
    void Empty();
    void Format(const char *fmt, ...);
    // MFC's CString -> LPCTSTR is an inline accessor (a plain [this+0] load).
    operator const char *() const { return m_pchData; }

    char *m_pchData;             // +0x00
};

#endif  // GRUNTZ_GRUNTZ_CSTRING_H
