#ifndef FORMATS_WWD_OBJECT_H
#define FORMATS_WWD_OBJECT_H

/*
 * WWD object-record struct — the on-disk world-object descriptor.
 *
 * Gruntz levels are .WWD ("Wap World Data") files. Each placed object is one
 * record with the field schema below. The SAME struct is parsed by:
 *   - the game's WWD/CObject loader (the runtime object base), and
 *   - the standalone editor (GruntzEdit.exe / WapWorld.exe), whose
 *     Object-Properties dialogs expose every field as a labeled control.
 *
 * VERSION-INDEPENDENT: the field set / layout is shared between the game and the
 * editor (and across the WAP32-engine sibling editors WapWorld/GMEdit). Evidence
 * that the struct is real and shared editor<->game: the validator string
 *   "Plane %s: Bad map tile value (%i) at %i,%i"
 * is byte-identical in GRUNTZ.EXE and in the editor binaries.
 *
 * PROVENANCE: field NAMES were mined from the editor's Object-Properties /
 * Object-Rectangles / Object-Points / Object-Flags / Object-User-Values /
 * Object-Misc-Values / Object-Hit-Info dialog labels (build/editor-strings/
 * GruntzEdit.utf16.txt). The on-disk byte LAYOUT is now PINNED from the Gruntz
 * loader (WwdFile::ReadPlaneObjects @0x162af0 advances the source pointer by
 * 0x11C = 284 per record via `lea [esi+0x11c]` + `rep movs`) cross-validated
 * against OpenClaw WwdFile.cpp ReadPlaneObjects + libwap.h WwdObject and
 * libwap32 wwd_read.cpp read_objects. The fixed part is 284 bytes (0x11C),
 * followed by FOUR length-prefixed variable strings: name, logic, imageSet,
 * sound. (Field-by-field offset table is the WwdObjectRecord struct below.)
 *
 * KEY CORRECTION to the earlier editor-derived schema: imageSet is NOT an int —
 * on disk it is a length-prefixed STRING (its length is one of four DWORDs at
 * record offset +0x0C), and there is ALSO a trailing `sound` string (length at
 * +0x10) that the editor schema omitted. The libwap32 wwd_structs.h is missing
 * soundLength; OpenClaw libwap.h (soundLength) and libwap32 wwd_read.cpp (the
 * 5th read it calls "animation") are correct — trust those.
 *
 * Coordinates: WWD uses pixel-space X/Y (not tile indices); tiles are a separate
 * plane array. Rects are inclusive {left,top,right,bottom} (editor labels them
 * Left/Top/Right/Bottom).
 *
 * TWO views below:
 *   1. WwdObjectRecord — the PINNED on-disk fixed record (284 B = 0x11C) with
 *      every field's byte offset confirmed; this is the load-bearing layout.
 *   2. WwdObject       — the editor-grouped, name-rich view (CString-based),
 *      kept for readability; its member ORDER is editor-dialog grouping, NOT the
 *      on-disk order, so it is NOT a byte-layout source.
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>

/* Inclusive rectangle, {left,top,right,bottom}, as used by every object rect. */
struct WwdRect
{
    int left;    // "Left:"
    int top;     // "Top:"
    int right;   // "Right:"
    int bottom;  // "Bottom:"
};

/*
 * WwdObjectRecord — the PINNED on-disk fixed object record.
 * @size: 0x11C (284 bytes), confirmed by WwdFile::ReadPlaneObjects @0x162af0
 *        (`lea [esi+0x11c]` + `rep movs` per object). After this fixed block come
 *        FOUR length-prefixed (lengths at +0x04/+0x08/+0x0C/+0x10) variable
 *        strings: name, logic, imageSet, sound (NOT NUL-terminated on disk).
 *
 * All fields are little-endian 32-bit. Offsets in comments are byte offsets into
 * the record. This is the load-bearing layout; the editor-grouped WwdObject below
 * is a name-rich convenience view only.
 */
struct WwdObjectRecord
{
    int          id;            // @0x000  "ID"
    unsigned int nameLength;    // @0x004  length of trailing name string
    unsigned int logicLength;   // @0x008  length of trailing logic string
    unsigned int imageSetLength;// @0x00C  length of trailing image-set string
    unsigned int soundLength;   // @0x010  length of trailing sound string ("animation" in libwap32)
    int          x;             // @0x014  "X Coord:" (pixel)
    int          y;             // @0x018  "Y Coord:"
    int          z;             // @0x01C  "Z Coord:"
    int          i;             // @0x020  frame index
    unsigned int addFlags;      // @0x024  Difficult/EyeCandy/HighDetail/...
    unsigned int dynamicFlags;  // @0x028  "Dynamic Flags" NoHit/AlwaysActive/Safe/AutoHitDamage
    unsigned int drawFlags;     // @0x02C  "Draw Flags" NoDraw/Mirror/Invert/Flash
    unsigned int userFlags;     // @0x030  "User Flags"
    int          score;         // @0x034  "Score:"
    int          points;        // @0x038  "Points:"
    int          powerup;       // @0x03C  "Powerup:"
    int          damage;        // @0x040  "Damage:"
    int          smarts;        // @0x044  "Smarts:"
    int          health;        // @0x048  "Health:"
    int          moveRect[4];   // @0x04C  "Move Rect"   {l,t,r,b}
    int          hitRect[4];    // @0x05C  "Hit Rect"
    int          attackRect[4]; // @0x06C  "Attack Rect"
    int          clipRect[4];   // @0x07C  "Clip Rect"
    int          userRect1[4];  // @0x08C  "User1 Rect"
    int          userRect2[4];  // @0x09C  "User2 Rect"
    int          userValue1;    // @0x0AC  "User 1:"
    int          userValue2;    // @0x0B0  "User 2:"
    int          userValue3;    // @0x0B4  "User 3:"
    int          userValue4;    // @0x0B8  "User 4:"
    int          userValue5;    // @0x0BC  "User 5:"
    int          userValue6;    // @0x0C0  "User 6:"
    int          userValue7;    // @0x0C4  "User 7:"
    int          userValue8;    // @0x0C8  "User 8:"
    int          minX;          // @0x0CC  "xMin:"
    int          minY;          // @0x0D0  "yMin:"
    int          maxX;          // @0x0D4  "xMax:"
    int          maxY;          // @0x0D8  "yMax:"
    int          speedX;        // @0x0DC  "Speed X:"
    int          speedY;        // @0x0E0  "Speed Y:"
    int          tweakX;        // @0x0E4  "xTweak"
    int          tweakY;        // @0x0E8  "yTweak"
    int          counter;       // @0x0EC  "Counter:"
    int          speed;         // @0x0F0  "Speed:"
    int          width;         // @0x0F4  "Width:"
    int          height;        // @0x0F8  "Height:"
    int          direction;     // @0x0FC  "Direction:"
    int          faceDir;       // @0x100  "Face Dir:"
    int          timeDelay;     // @0x104  "Time Delay:"
    int          frameDelay;    // @0x108  "Frame Delay:"
    unsigned int objectType;    // @0x10C  "Object Type" (single value)
    unsigned int hitTypeFlags;  // @0x110  "Hit Type Flags"
    int          moveResX;      // @0x114  "xMoveRes:"
    int          moveResY;      // @0x118  "yMoveRes:"
    // @0x11C (284): variable-length, length-prefixed (above), NOT NUL-terminated:
    //   char name[nameLength]; char logic[logicLength];
    //   char imageSet[imageSetLength]; char sound[soundLength];
};  // sizeof == 0x11C (284) — PINNED

/*
 * WwdObject — one world-object record (editor-grouped, name-rich VIEW).
 * @size: not a byte-layout source — see WwdObjectRecord above for the PINNED
 *        on-disk fixed 0x11C layout. Member ORDER here is editor-dialog grouping,
 *        not on-disk order.
 *
 * Field NAMES are the editor's labels, in dialog grouping order. The game's
 * CObject (and every gameplay subclass: CGrunt, hazards, triggers, …) is built
 * from this record.
 */
struct WwdObject
{
    /* --- Identification (Object-Properties dialog) --- */
    CString name;        // "Name:"        object instance name (length-prefixed on disk)
    CString logic;       // "Logic:"       logic-program name (-> CUserLogic dispatch)
    CString imageSet;    // "Image Set:"   primary image-set name (length-prefixed STRING on disk, NOT an int)
    CString sound;       // trailing sound/"animation" string (length-prefixed on disk; editor omitted it)

    /* --- Attributes --- */
    int score;           // "Score:"
    int points;          // "Points:"      (editor exposes both Score and Points)
    int powerup;         // "Powerup:"     (see enum Powerup)
    int damage;          // "Damage:"
    int smarts;          // "Smarts:"      AI level
    int health;          // "Health:"

    /* --- Motion / facing --- */
    int speedX;          // "Speed X:"
    int speedY;          // "Speed Y:"
    int speed;           // "Speed:"       (scalar speed, distinct from Speed X/Y)
    int faceDir;         // "Face Dir:"    (see enum Direction)
    int direction;       // "Direction:"   movement direction (vs facing)

    /* --- Misc values (Object-Misc-Values dialog) --- */
    int counter;         // "Counter:"
    int width;           // "Width:"
    int height;          // "Height:"
    int timeDelay;       // "Time Delay:"
    int frameDelay;      // "Frame Delay:"

    /* --- Position --- */
    int zCoord;          // "Z Coord:"     draw/sort depth
    int xCoord;          // "X Coord:"     pixel X
    int yCoord;          // "Y Coord:"     pixel Y

    /* --- Movement bounds / tweaks (Object-Points dialog) --- */
    int xMin;            // "xMin:" / "X Min:"
    int xMax;            // "xMax:" / "X Max:"
    int yMin;            // "yMin:" / "Y Min:"
    int yMax;            // "yMax:" / "Y Max:"
    int xMoveRes;        // "xMoveRes:"     movement resolution X
    int yMoveRes;        // "yMoveRes:"     movement resolution Y
    int xTweak;          // "xTweak"
    int yTweak;          // "yTweak"

    /* --- User scalars (Object-User-Values dialog) --- */
    int userValue;       // "User Value:"
    int user[8];         // "User 1:" .. "User 8:"  (1-based in the editor)

    /* --- Rectangles (Object-Rectangles dialog) --- */
    WwdRect hitRect;     // "Hit Rect"
    WwdRect attackRect;  // "Attack Rect"
    WwdRect clipRect;    // "Clip Rect"
    WwdRect moveRect;    // "Move Rect"
    WwdRect user1Rect;   // "User1 Rect"
    WwdRect user2Rect;   // "User2 Rect"

    /* --- Flag words (Object-Flags / Object-Hit-Info dialogs) --- */
    unsigned int drawFlags;     // "Draw Flags"     (see WwdDrawFlags)
    unsigned int dynamicFlags;  // "Dynamic Flags"
    unsigned int userFlags;     // "User Flags"
    unsigned int hitTypeFlags;  // "Hit Type Flags" (see WwdHitTypeFlags)
    unsigned int objectFlags;   // "Object Flags"   (see WwdObjectFlags)
    int autoHitDamage;          // "Auto Hit Damage"

    /* --- Image sets / prefixes (Add-Image-Sets dialog) ---
     * NOTE: these imageSetN/prefixN belong to the WWD LEVEL HEADER (see
     * WwdHeader.imageSet1..4 / prefix1..4 in wwd.h), NOT to the per-object
     * on-disk record (WwdObjectRecord) — they are reproduced here only because the
     * editor surfaces them via an object-adjacent dialog. */
    CString imageSet1, imageSet2, imageSet3, imageSet4;  // "Image Set 1:".."4:"
    CString prefix1, prefix2, prefix3, prefix4;          // "Prefix 1:".."4:"
};

/*
 * Object Flags — the 1-bit toggles in the editor's "Object Flags" dialog, in label
 * order. These are the WAP32-engine object flag bits (shared with WapWorld/GMEdit).
 * Bit VALUES are @todo (label order != bit order until confirmed); names verbatim.
 */
enum WwdObjectFlags
{
    WWD_OBJ_NO_HIT,          // "No Hit"
    WWD_OBJ_ALWAYS_ACTIVE,   // "Always Active"
    WWD_OBJ_SAFE,            // "Safe"
    WWD_OBJ_AUTO_HIT_DAMAGE, // "Auto Hit Damage"
    WWD_OBJ_DIFFICULT,       // "Difficult"
    WWD_OBJ_EYE_CANDY,       // "Eye Candy"
    WWD_OBJ_HIGH_DETAIL,     // "High Detail"
    WWD_OBJ_MULTIPLAYER,     // "Multiplayer"
    WWD_OBJ_EXTRA_MEMORY,    // "Extra Memory"
    WWD_OBJ_FAST_CPU,        // "Fast CPU"
    WWD_OBJ_NO_DRAW,         // "No Draw"
    WWD_OBJ_MIRROR,          // "Mirror"
    WWD_OBJ_INVERT,          // "Invert"
    WWD_OBJ_FLASH            // "Flash"
    //@todo: bit positions/values unverified against the binary
};

/*
 * Hit Type Flags / Object Type — the object hit-class bits in the editor's
 * "Object Hit Info" dialog (the "Object Type" / "Hit Type Flags" list). Names
 * verbatim; the editor lists them twice (combo + checklist).
 */
enum WwdHitTypeFlags
{
    WWD_HIT_GENERIC,   // "Generic"
    WWD_HIT_PLAYER,    // "Player"
    WWD_HIT_ENEMY,     // "Enemy"
    WWD_HIT_POWERUP,   // "Powerup"
    WWD_HIT_SHOT,      // "Shot"
    WWD_HIT_PSHOT,     // "P-Shot"   (player shot)
    WWD_HIT_ESHOT,     // "E-Shot"   (enemy shot)
    WWD_HIT_SPECIAL,   // "Special"
    WWD_HIT_USER1,     // "User 1"
    WWD_HIT_USER2,     // "User 2"
    WWD_HIT_USER3,     // "User 3"
    WWD_HIT_USER4      // "User 4"
    //@todo: bit positions/values unverified against the binary
};

/*
 * Dynamic / Draw / User flag words also expose a generic "Flag 1".."Flag 12"
 * checklist in the editor ("Add Flags"). Those 12 generic bits map to whichever
 * flag word is being edited; not modeled as a distinct enum.
 *   @todo: which word(s) the 12 generic flags apply to, and their bit meanings.
 */

#endif /* FORMATS_WWD_OBJECT_H */
