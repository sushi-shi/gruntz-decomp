#ifndef GRUNTZ_GRUNTZ_MOTIONINLINE_H
#define GRUNTZ_GRUNTZ_MOTIONINLINE_H

#include <Gruntz/MotionState.h>
#include <Lith/BDefs.h>

#include <math.h>

inline double
ArrivalVelocity(double velocity, double acceleration, double position, double target) {
    if (acceleration == g_motionZero) {
        return velocity;
    }
    double discriminant = SQR(velocity) - (target - position) * acceleration * g_motionNegTwo;
    if (g_motionZero > discriminant) {
        discriminant = g_motionZero;
    }
    double root = sqrt(discriminant);
    return (velocity > g_motionZero) ? root : -root;
}

inline void StepMotionAxis(
    double& velocity,
    double acceleration,
    double& position,
    double maxStep,
    double lowerBound,
    double upperBound,
    double positiveVelocityClamp,
    double& step,
    double dt
) {
    double accelerationStep = dt * acceleration;
    double proposedStep = (velocity - accelerationStep * g_motionNegHalf) * dt;
    step = proposedStep;
    do {
        double clampedStep;
        if (proposedStep > maxStep) {
            clampedStep = maxStep;
        } else if (proposedStep < -maxStep) {
            clampedStep = -maxStep;
        } else {
            break;
        }
        step = clampedStep;
        velocity = ArrivalVelocity(velocity, acceleration, position, clampedStep + position);
    } while (0);
    double oldPosition = position;
    double newPosition = step + position;
    position = newPosition;
    if (newPosition > upperBound) {
        velocity = ArrivalVelocity(velocity, acceleration, position, upperBound);
        step = upperBound - oldPosition;
        position = upperBound;
    } else if (newPosition < lowerBound) {
        velocity = ArrivalVelocity(velocity, acceleration, position, lowerBound);
        step = lowerBound - oldPosition;
        position = lowerBound;
    } else {
        velocity += accelerationStep;
    }
    if (velocity > positiveVelocityClamp) {
        velocity = positiveVelocityClamp;
    }
}

#endif // GRUNTZ_GRUNTZ_MOTIONINLINE_H
