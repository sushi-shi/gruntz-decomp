// CObList.h - MFC CObList (NAFXCW doubly-linked object list). Reconstructed only
// as deeply as the byte-matches need: 0x1c bytes total (vptr + node head/tail/
// count + free-list + block params, all opaque); the methods are out-of-line
// NAFXCW calls (reloc-masked). Embedded by value (e.g. CMultiStartDlg +0x74,
// CDDrawWorkerList +0x10), so the 0x1c size is load-bearing.
#ifndef GRUNTZ_GRUNTZ_COBLIST_H
#define GRUNTZ_GRUNTZ_COBLIST_H

struct __POSITION;
class CObject;

class CObList {
public:
    CObList(int nBlockSize);
    __POSITION *AddHead(CObject *newElement);
    __POSITION *AddTail(CObject *newElement);
    void RemoveAll();
    void RemoveAt(__POSITION *position);

    char m_body[0x1c];          // +0x00 (incl. the implicit vptr)
};

#endif  // GRUNTZ_GRUNTZ_COBLIST_H
