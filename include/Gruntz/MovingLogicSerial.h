#ifndef GRUNTZ_CMOVINGLOGICSERIAL_H
#define GRUNTZ_CMOVINGLOGICSERIAL_H

#include <Ints.h>

#include <Gruntz/MotionState.h>
#include <Gruntz/SerialArchive.h>
#include <Ints.h>

class ostream;
class istream;

ostream& WriteCurve(ostream& accum, const CMotionState& c);
istream& ReadCurve(istream& accum, CMotionState& c);

#endif
