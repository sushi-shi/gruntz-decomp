// DirectionClassify.cpp - 0x4a780, a Ghidra-missed 8-way direction classifier
// (__thiscall(other, exact) -> GruntDirectionCell*). @identity-TODO: the owning entity class is
// unrecovered (position-bearing, world coords as doubles at +0x78/+0x80, snapped cell
// ints at +0x140/+0x144; leaf between LoadAnimNameTable 0x49c60 and the CopyRect helper
// 0x4a9f0 in the CGrunt anim region). It returns the direction descriptor for the
// heading from `this` to `other`, chosen by the sign of the delta and the slope ratio
// against +-0.5 / +-2.0 (the 22.5/67.5-degree octant thresholds), with an exact-on-cell
// short-circuit. Field names are placeholders; only offsets + code bytes are load-bearing.
#include <Ints.h>
#include <rva.h>
#include <Gruntz/DirectionClassify.h>
#include <Gruntz/Grunt.h>

DATA(0x001e9750)
const double g_slopeNegHalf = -0.5;

DATA(0x001e9758)
const double g_slopePosHalf = 0.5;
DATA(0x001e9760)
const double g_slopePosTwo = 2.0;
DATA(0x001e9768)
const double g_slopeNegTwo = -2.0;

DATA(0x00244938)
GruntDirectionCell g_gruntMoveDirCenter(1, 1, 0);

// MotionEntity (the position-bearing entity Classify runs on) is defined in
// <Gruntz/DirectionClassify.h>; identity @identity-TODO.

// @early-stop
// ~96% - structure + logic + all FP thresholds/relocs exact (the 9-way octant tree,
// the __ftol conversions reading st0, the fcom/fcomp `<=` cascades, the on-cell
// short-circuit). Residual is confined to the initial dx/dy delta materialization: cl
// computes dx via `fld this.x; fsub other.x` and lands dy=ebx/dx=edi, retail computes
// dx via `fld other.x; fld this.x; fxch; fsubp` and lands dy=edi/dx=ebx; that register
// swap ripples through the sign tests. x87-stack-schedule + regalloc wall; permuter
// found no closing spelling.
RVA_COMPGEN(0x000479e0, 0x1a, _$E293344)
RVA(0x0004a780, 0x1ec)
GruntDirectionCell* MotionEntity::Classify(MotionEntity* other, char exact) {
    if (other == 0) {
        return &g_gruntMoveDirCenter;
    }
    i32 dy = static_cast<i32>((other->m_78 - m_78));
    i32 dx = static_cast<i32>((m_80 - other->m_80));
    if (dy == 0) {
        if (dx > 0) {
            return &g_gruntMoveDirNorth;
        }
        if (dx < 0) {
            return &g_gruntMoveDirSouth;
        }
        return &g_gruntMoveDirCenter;
    }

    char onCell = exact;
    if (onCell) {
        onCell = (static_cast<i32>(m_78) == m_140 && static_cast<i32>(m_80) == m_144) ? 1 : 0;
    }
    double ratio = static_cast<double>(dx) / static_cast<double>(dy);

    if (dx >= 0 && dy > 0) {
        if (onCell) {
            return &g_gruntMoveDirNorthEast;
        }
        if (ratio <= g_slopePosHalf) {
            return &g_gruntMoveDirEast;
        }
        if (ratio <= g_slopePosTwo) {
            return &g_gruntMoveDirNorthEast;
        }
        return &g_gruntMoveDirNorth;
    }
    if (dx >= 0) { // dy < 0
        if (onCell) {
            return &g_gruntMoveDirNorthWest;
        }
        if (ratio <= g_slopeNegTwo) {
            return &g_gruntMoveDirNorth;
        }
        if (ratio <= g_slopeNegHalf) {
            return &g_gruntMoveDirNorthWest;
        }
        return &g_gruntMoveDirWest;
    }
    if (dy > 0) { // dx < 0
        if (onCell) {
            return &g_gruntMoveDirSouthEast;
        }
        if (ratio <= g_slopeNegTwo) {
            return &g_gruntMoveDirSouth;
        }
        if (ratio <= g_slopeNegHalf) {
            return &g_gruntMoveDirSouthEast;
        }
        return &g_gruntMoveDirEast;
    }
    // dx < 0, dy < 0
    if (onCell) {
        return &g_gruntMoveDirSouthWest;
    }
    if (ratio <= g_slopePosHalf) {
        return &g_gruntMoveDirWest;
    }
    if (ratio <= g_slopePosTwo) {
        return &g_gruntMoveDirSouthWest;
    }
    return &g_gruntMoveDirSouth;
}
