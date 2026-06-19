// CMapStringToOb.h - MFC CMapStringToOb (NAFXCW string->CObject* hash map).
// Reconstructed as deeply as the byte-matches need: a CObject vptr + hash
// table/size/count then an opaque tail to the real 0x1c size; methods are
// out-of-line NAFXCW calls (reloc-masked). Embedded by value (each parent +0x10).
#ifndef GRUNTZ_GRUNTZ_CMAPSTRINGTOOB_H
#define GRUNTZ_GRUNTZ_CMAPSTRINGTOOB_H
#include <Gruntz/CString.h>

class CObject;
typedef int POSITION;   // opaque MFC iteration handle (int form for the ternary guard)

class CMapStringToOb {
public:
    int Lookup(const char *key, CObject *&rValue) const;
    CObject *&operator[](const char *key);
    int RemoveKey(const char *key);
    void GetNextAssoc(POSITION &rNextPosition, CString &rKey, CObject *&rValue) const;
    void RemoveAll();

    void     *m_vptr;              // +0x00  CObject vptr
    unsigned  m_nHashTableSize;    // +0x04
    void    **m_pHashTable;        // +0x08
    int       m_nCount;            // +0x0c
    char      m_tail[0x1c - 0x10]; // +0x10..0x1b  free-list/blocks/blocksize (opaque)
};

#endif  // GRUNTZ_GRUNTZ_CMAPSTRINGTOOB_H
