#ifndef GRUNTZ_CMOVINGLOGICSERIAL_H
#define GRUNTZ_CMOVINGLOGICSERIAL_H

#include <Ints.h>
#include <Gruntz/MotionState.h>
#include <rva.h>
#include <Gruntz/SerialArchive.h>

class ostream;
class istream;

istream& ReadCurve(istream& accum, CMotionState& c);

extern i32 g_logicTypesRegistered;

#endif // GRUNTZ_CMOVINGLOGICSERIAL_H
