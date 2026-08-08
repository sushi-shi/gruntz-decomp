#ifndef GRUNTZ_CMOVINGLOGICSERIAL_H
#define GRUNTZ_CMOVINGLOGICSERIAL_H

#include <rva.h>

#include <Gruntz/MotionState.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

class ostream;
class istream;

istream& ReadCurve(istream& accum, CMotionState& c);

#endif // GRUNTZ_CMOVINGLOGICSERIAL_H
