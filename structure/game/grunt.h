#ifndef GAME_GRUNT_H
#define GAME_GRUNT_H

/*
 * CGrunt and the Grunt-related object family.
 *
 * CGrunt is the central gameplay actor. Its FIELD ORDER (not offsets/types) is
 * hinted by the leaked debug-dump format string (STRINGS_ANALYSIS.md §5):
 *
 *   [p=%d][g=%d][health=%d][x=%d][y=%d][dir=%d][stm=%d][ttl=%d][tool=%d][toy=%d]
 *   [da=%d][wp=%d][iic=%d][qat=%d][qax=%d][ia=%d][iad=%d][rnd=%d]
 *
 *   p=player  g=gruntId  health  x  y  dir(enum Direction)  stm=stamina
 *   ttl=timeToLive  tool(enum Tool)  toy(enum Toy)  da/wp/iic/qat/qax/ia/iad=unknown
 *   rnd=RNG state?
 *
 * The in-struct types/widths/OFFSETS are UNKNOWN, and the dump PRINT order is not
 * guaranteed to be the struct order, so NO layout is encoded here (would be
 * invented). CGrunt is left as a name-only stub; the roster above is the only
 * recovered fact.
 *
 * Probable base: CObject (MFC) or a WAP object — not recovered.
 */

// CGrunt — a single Gruntz unit/actor. Layout unknown (see field roster above).
class CGrunt { /* .?AVCGrunt@@ */ };

// CGruntVoice — per-grunt voice playback.
class CGruntVoice { /* .?AVCGruntVoice@@ */ };

// CGruntPuddle — the puddle left when a grunt dies/melts.
class CGruntPuddle { /* .?AVCGruntPuddle@@ */ };

// Spawn / placement points.
class CGruntCreationPoint { /* .?AVCGruntCreationPoint@@ */ };
class CGruntStartingPoint { /* .?AVCGruntStartingPoint@@ */ };

#endif /* GAME_GRUNT_H */
