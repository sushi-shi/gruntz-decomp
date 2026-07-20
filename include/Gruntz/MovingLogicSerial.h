#ifndef GRUNTZ_CMOVINGLOGICSERIAL_H
#define GRUNTZ_CMOVINGLOGICSERIAL_H

#include <Ints.h>
#include <Gruntz/MovingLogic.h> // the canonical CMovingLogic + CMotionState (+0x38 curve)
#include <rva.h>

class ostream {
public:
    ostream& operator<<(i32 v);    // ??6ostream@@QAEAAV0@H@Z  0x191d20
    ostream& operator<<(double v); // ??6ostream@@QAEAAV0@N@Z  0x191df0
};
class istream;

#include <Gruntz/SerialArchive.h>

#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

void ReadCurve(istream& accum, CMotionState& c); // 0x16d000

class CButeVbaseTeardown {
public:
    void DtorReadA();  // 0x1697c0  (read-temp derived teardown)
    void DtorWriteB(); // 0x1699c0 (write-temp derived teardown)
    void FuncB();      // 0x169d70 (shared vbase teardown)
};

class CButeReadTemp {
public:
    void Ctor(void* buf, i32 len, i32 flag); // 0x169700
    char _00[0x0c];
    CButeVbaseTeardown m_vbase; // +0x0c
    char _10[0x130];
};
class CButeWriteTemp {
public:
    void Ctor(void* buf, i32 cap, i32 a, i32 b); // 0x1698c0
    i32 Length();                                // inlined buffer-length probe
    char* GetBuffer();                           // 0x1692b0
    char _00[0x0c];
    CButeVbaseTeardown m_vbase; // +0x0c
    char _10[0x130];
};

void WriteName(void* accum, void* pstr); // 0x193080
void ReadName(void* accum, void* pstr);  // 0x193140

extern i32 g_logicTypesRegistered; // 0x6bf674 (?g_logicTypesRegistered@@3HA)

#endif // GRUNTZ_CMOVINGLOGICSERIAL_H
