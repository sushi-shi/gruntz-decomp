// CoordNode.h - the intrusive coord-list node used by the grunt occupied-coord
// lists and the RezSync coord-pool freelist. One 0x0c-byte node: an intrusive
// ->next link at +0x00 and a {x,y} Coord payload pointer at +0x08 (the +0x04
// word is unused padding). The same node type the CGrunt path names
// GruntCoordNode (see Grunt.h).
#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <rva.h>

// The {x,y} payload itself. Was re-declared as five identical .cpp-local views
// (Coord / Candidate / CCoordXY / ProbePair in BattlezMapConfig.cpp); this is the
// single shape.
SIZE(Coord, 0x8);
struct Coord {
    i32 m_x; // +0x00
    i32 m_y; // +0x04
};

SIZE(CoordNode, 0xc);
struct CoordNode {
    CoordNode* m_next;  // +0x00  intrusive list/free link
    char m_pad04[0x04]; // +0x04  unused
    Coord* m_coord;     // +0x08  {x,y} payload
};

#endif // GRUNTZ_GRUNTZ_COORDNODE_H
