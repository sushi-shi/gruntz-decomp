#ifndef GRUNTZ_DOUBLEVECTOR_H
#define GRUNTZ_DOUBLEVECTOR_H

#include <rva.h>

struct DoubleVector2 {
    void Init(double a = 0.0, double b = 0.0) {
        x = a;
        y = b;
    }

    double x;
    double y;
};

struct DoubleVector3 {
    void Init(double a = 0.0, double b = 0.0, double c = 0.0) {
        x = a;
        y = b;
        z = c;
    }

    double x;
    double y;
    double z;
};

#endif // GRUNTZ_DOUBLEVECTOR_H
