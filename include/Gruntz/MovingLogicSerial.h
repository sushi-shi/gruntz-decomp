#ifndef GRUNTZ_CMOVINGLOGICSERIAL_H
#define GRUNTZ_CMOVINGLOGICSERIAL_H

#include <Ints.h>
#include <Gruntz/MovingLogic.h> // the canonical CMovingLogic + CMotionState (+0x38 curve)
#include <rva.h>

// The serialize accumulators are REAL CRT strstream temps (<strstrea.h>, statically
// linked): the ex-CButeWriteTemp/CButeReadTemp "Ctor(buf,len,..,1)" calls were
// ??0ostrstream@@QAE@PADHH@Z (0x1698c0) / ??0istrstream@@QAE@PADH@Z (0x169700) with
// the compiler's hidden vbase flag, the "teardown helpers" the compiler-emitted
// ~ostrstream/~istrstream (0x1699c0/0x1697c0) + ~ios vbase (0x169d70) pair, Length()
// the inlined pcount() vbase probe, GetBuffer() str() -> ?str@strstreambuf (0x1692b0).
// The defining TU includes the real <strstrea.h>; these fwd decls serve the rest.
class ostream;
class istream;

#include <Gruntz/SerialArchive.h>

#include <Rez/RezAlloc.h> // RezAlloc/RezFree (the global allocator pair)

void ReadCurve(istream& accum, CMotionState& c); // (external curve parser; reloc-masked)

void WriteName(void* accum, void* pstr); // 0x193080
void ReadName(void* accum, void* pstr);  // 0x193140

extern i32 g_logicTypesRegistered; // 0x6bf674 (?g_logicTypesRegistered@@3HA)

#endif // GRUNTZ_CMOVINGLOGICSERIAL_H
