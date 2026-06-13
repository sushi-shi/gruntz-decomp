#ifndef GAME_GRUNT_H
#define GAME_GRUNT_H

/*
 * CGrunt and the Grunt-related object family.
 *
 * CGrunt is the central gameplay actor. Its FIELD ORDER (not offsets/types) is
 * recovered from the leaked debug-dump format string (STRINGS_ANALYSIS.md §5):
 *
 *   [p=%d][g=%d][health=%d][x=%d][y=%d][dir=%d][stm=%d][ttl=%d][tool=%d][toy=%d]
 *   [da=%d][wp=%d][iic=%d][qat=%d][qax=%d][ia=%d][iad=%d][rnd=%d]
 *
 * Each token below is expanded to a guessed readable name with a guessed @todo
 * type, and the RAW token is kept in a comment. All %d here implies int-ish, but
 * the in-struct type/width/offset is UNKNOWN — every member is @todo. The dump
 * order is NOT guaranteed to be the struct order, only the print order.
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>
#include "../enums.h"

/*
 * CGrunt — a single Gruntz unit/actor.
 * .?AVCGrunt@@
 * Probable base: CObject (MFC) or a WAP object — @todo (not recovered).
 */
class CGrunt /* : public CObject @todo */
{
public:
    //@size: unknown @todo

    /* --- fields, in debug-dump print order (offsets/types all @todo) --- */

    //@todo type/offset   raw: [p]
    int m_player;            // p   — owning player index
    //@todo type/offset   raw: [g]
    int m_gruntId;           // g   — grunt id (within player)
    //@todo type/offset   raw: [health]
    int m_health;            // health
    //@todo type/offset   raw: [x]
    int m_x;                 // x   — tile/world X
    //@todo type/offset   raw: [y]
    int m_y;                 // y   — tile/world Y
    //@todo type/offset   raw: [dir]  (see enum Direction)
    int m_dir;               // dir — facing direction (0..7)
    //@todo type/offset   raw: [stm]
    int m_stamina;           // stm — stamina
    //@todo type/offset   raw: [ttl]
    int m_timeToLive;        // ttl — time-to-live
    //@todo type/offset   raw: [tool]  (see enum Tool)
    int m_tool;              // tool — held tool
    //@todo type/offset   raw: [toy]   (see enum Toy)
    int m_toy;               // toy  — held toy
    //@todo type/offset   raw: [da]    meaning unknown
    int m_da;                // da   — /* unknown */
    //@todo type/offset   raw: [wp]    waypoint?
    int m_wp;                // wp   — /* unknown, "waypoint"? */
    //@todo type/offset   raw: [iic]   meaning unknown
    int m_iic;               // iic  — /* unknown */
    //@todo type/offset   raw: [qat]   meaning unknown ("queued action"?)
    int m_qat;               // qat  — /* unknown */
    //@todo type/offset   raw: [qax]   meaning unknown
    int m_qax;               // qax  — /* unknown */
    //@todo type/offset   raw: [ia]    meaning unknown
    int m_ia;                // ia   — /* unknown */
    //@todo type/offset   raw: [iad]   meaning unknown
    int m_iad;               // iad  — /* unknown */
    //@todo type/offset   raw: [rnd]   random seed/state?
    int m_rnd;               // rnd  — /* unknown, RNG state? */
};

/*
 * CGruntVoice — per-grunt voice playback.
 * .?AVCGruntVoice@@
 */
class CGruntVoice { /* .?AVCGruntVoice@@ */ };  //@todo layout

/*
 * CGruntPuddle — the puddle left when a grunt dies/melts.
 * .?AVCGruntPuddle@@
 */
class CGruntPuddle { /* .?AVCGruntPuddle@@ */ };  //@todo layout

/*
 * Spawn / placement points.
 */
class CGruntCreationPoint { /* .?AVCGruntCreationPoint@@ */ };  //@todo layout
class CGruntStartingPoint { /* .?AVCGruntStartingPoint@@ */ };  //@todo layout

#endif /* GAME_GRUNT_H */
