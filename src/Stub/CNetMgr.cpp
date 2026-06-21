#include <rva.h>
// CNetMgr.cpp - engine-label stubs for CNetMgr (reloc-correlation).

class CNetMgr {
public:
    void AckDropPlayer(int);
    void ReportVersionMsg(char const*, int);
    void SendNetStat(int, unsigned int, int);
    void SendStat3(int, unsigned int, int);
    void SendStatFlag(int, int);
};
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x0016d1, 0x5)
void CNetMgr::AckDropPlayer(int) {}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x001af0, 0x5)
void CNetMgr::ReportVersionMsg(char const*, int) {}
// @confidence: high
// @source: reloc-correlation (2 callers)
// @stub
RVA(0x002955, 0x5)
void CNetMgr::SendNetStat(int, unsigned int, int) {}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x003d0a, 0x5)
void CNetMgr::SendStat3(int, unsigned int, int) {}
// @confidence: med
// @source: reloc-correlation (1 caller)
// @stub
RVA(0x002e82, 0x5)
void CNetMgr::SendStatFlag(int, int) {}
