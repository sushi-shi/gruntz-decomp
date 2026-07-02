// GruntCmdPercent.cpp - percentage transfer curve (RVA 0x135110), trace-mis-homed
// onto CGruntzSingleCommand but actually a free __cdecl helper. Returns 100 for a
// zero input, else r = c_volScale - (c_volNum - pow(c_acosNorm, -(|v|/100)/c_powExp))
// * c_volScale (== 100*2^(-(|v|/100)/10)), floored via __ftol and sign-flipped for
// non-negative inputs. Shares the volume-curve constant pool with SoundDevice.cpp.
#include <rva.h>
#include <math.h> // pow -> __CIpow ; (int)double -> __ftol
#include <Globals.h>

RVA(0x00135110, 0x8e)
i32 ComputeCmdPercent(i32 v) {
    if (v == 0) {
        return 100;
    }
    double d;
    if (v < 0) {
        d = (double)(-v / 100);
    } else {
        d = (double)(v / 100);
    }
    double r = c_volScale - (c_volNum - pow(c_acosNorm, -d / c_powExp)) * c_volScale;
    if (v < 0) {
        return (i32)r;
    }
    return (i32)(-r);
}
