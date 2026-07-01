// CoordNode.h - the intrusive coord-list node used by the grunt occupied-coord
// lists and the RezSync coord-pool freelist. One 0x0c-byte node: an intrusive
// ->next link at +0x00 and a {x,y} Coord payload pointer at +0x08 (the +0x04
// word is unused padding). The same node type the CGrunt path names
// GruntCoordNode (see Grunt.h).
#ifndef GRUNTZ_GRUNTZ_COORDNODE_H
#define GRUNTZ_GRUNTZ_COORDNODE_H

#include <Ints.h>

struct Coord; // {i32 x; i32 y;} payload; completed by the TU that reads it

struct CoordNode {
    CoordNode* m_next;  // +0x00  intrusive list/free link
    char m_pad04[0x04]; // +0x04  unused
    Coord* m_coord;     // +0x08  {x,y} payload
};

#endif // GRUNTZ_GRUNTZ_COORDNODE_H
