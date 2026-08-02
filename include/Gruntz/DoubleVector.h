#ifndef GRUNTZ_DOUBLEVECTOR_H
#define GRUNTZ_DOUBLEVECTOR_H

#include <rva.h>

struct DoubleVector2 {
    double x;
    double y;
};
SIZE(0x10);

struct DoubleVector3 {
    double x;
    double y;
    double z;
};
SIZE(0x18);

#endif // GRUNTZ_DOUBLEVECTOR_H
