#ifndef GRUNTZ_BRICKTILEID_H
#define GRUNTZ_BRICKTILEID_H

#include <Enums.h>

// The WWD tile-image index of a Brickz stack - what CTileActionEvent::m_actionCode
// holds, what CGruntzMapMgr::LoadAttributes re-rolls, and what CTileActionEvent
// writes back into CDDrawWorkerHost::m_tileGrid.
//
// A stack is one to three brickz high. At most ONE brick in it is coloured; the
// rest are plain brown. The id therefore encodes (colour, height, which layer is
// the coloured one), and the three facts that pin it are all in this file's own
// callers:
//
//  * COLOUR, from CTileActionEvent::Process. Breaking the top brick sets `effect`
//    to the colour's height-1 id exactly when the break removed the last brick of
//    that colour, and `effect` selects the break sprite by name:
//    0x132 -> "GAME_REDBRICKBREAK", 0x138 -> "GAME_BLUEBRICKBREAK",
//    0x13e -> "GAME_GOLDBRICKBREAK", 0x144 -> "GAME_BLACKBRICKBREAK", and the
//    default (the brown ladder) -> "GAME_BRICKBREAK". The same five colours, in
//    this order, are the [Brickz] Brown/Red/Blue/Gold/Black weights that
//    CGruntzMapMgr::LoadAttributes reads out of the bute.
//
//  * HEIGHT, from CTileActionEvent::SetActionCode. When the current player has
//    not been shown the stack's true colour (m_playerFlags[g_curPlayer] == 0) the
//    id collapses to a BROWN id - 0x12f, 0x130 or 0x131 - and which one it picks
//    is the whole grouping: the five height-1 ids collapse to 0x12f, the nine
//    height-2 ids to 0x130, the thirteen height-3 ids to 0x131. That is exactly
//    the "playerz can no longer see the color of the Brickz in the stack" rule
//    from the editor docs, and it partitions the 27 ids by height. The three
//    re-rollers in LoadAttributes partition them the same way.
//
//  * LAYER, from CTileActionEvent::Process's newCode chain (break the top brick)
//    read against CTileActionEvent::MorphByTool (add one on top). Adding a BROWN
//    brick (PICKUP_BROWNBRICK, 0x22) takes RED_1 -> RED_2_LOW -> RED_3_LOW and
//    RED_2_TOP -> RED_3_MID, so _LOW/_MID/_TOP name where the coloured brick sits
//    in the stack. Adding a COLOURED brick (0x23-0x26) always lands on top, so it
//    takes any height-1 id to <colour>_2_TOP and any height-2 id to
//    <colour>_3_TOP whatever was underneath - which is why the _TOP ids break
//    back to a plain brown remainder.
//
// Gold is the odd one out: Process guards its arms with `if (brick != 0) break;`,
// so a grunt cannot break a gold brick at all. The hit only plays
// "GRUNTZ_NORMALGRUNT_IMPACTMM3" and sets m_playerFlags for that player, i.e. it
// reveals the colour instead of removing a layer.
GZ_ENUM_BEGIN(BrickTileId)
// The tile a fully broken stack leaves behind: Process's newCode for every
// height-1 arm, and the value CTileActionEvent::Process returns 1 for.
    BRICKTILE_CLEARED = 0x12d,

    BRICKTILE_BROWN_1 = 0x12f,
    BRICKTILE_BROWN_2 = 0x130,
    BRICKTILE_BROWN_3 = 0x131,

    BRICKTILE_RED_1 = 0x132,
    BRICKTILE_RED_2_LOW = 0x133,
    BRICKTILE_RED_2_TOP = 0x134,
    BRICKTILE_RED_3_LOW = 0x135,
    BRICKTILE_RED_3_MID = 0x136,
    BRICKTILE_RED_3_TOP = 0x137,

    BRICKTILE_BLUE_1 = 0x138,
    BRICKTILE_BLUE_2_LOW = 0x139,
    BRICKTILE_BLUE_2_TOP = 0x13a,
    BRICKTILE_BLUE_3_LOW = 0x13b,
    BRICKTILE_BLUE_3_MID = 0x13c,
    BRICKTILE_BLUE_3_TOP = 0x13d,

    BRICKTILE_GOLD_1 = 0x13e,
    BRICKTILE_GOLD_2_LOW = 0x13f,
    BRICKTILE_GOLD_2_TOP = 0x140,
    BRICKTILE_GOLD_3_LOW = 0x141,
    BRICKTILE_GOLD_3_MID = 0x142,
    BRICKTILE_GOLD_3_TOP = 0x143,

    BRICKTILE_BLACK_1 = 0x144,
    BRICKTILE_BLACK_2_LOW = 0x145,
    BRICKTILE_BLACK_2_TOP = 0x146,
    BRICKTILE_BLACK_3_LOW = 0x147,
    BRICKTILE_BLACK_3_MID = 0x148,
    BRICKTILE_BLACK_3_TOP = 0x149
GZ_ENUM_END(BrickTileId)

#endif // GRUNTZ_BRICKTILEID_H
