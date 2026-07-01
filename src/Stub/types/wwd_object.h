#ifndef FORMATS_WWD_OBJECT_H
#define FORMATS_WWD_OBJECT_H

#include <Ints.h>

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

// CString in the editor-grouped WwdObject view is an MFC type (a single char*);
// modeled here as a 4-byte placeholder so the header parses without <afxwin.h>.
typedef void* WwdCString;

/* Inclusive rectangle, {left,top,right,bottom}, as used by every object rect. */
struct WwdRect {
    i32 left;   // "Left:"
    i32 top;    // "Top:"
    i32 right;  // "Right:"
    i32 bottom; // "Bottom:"
};

/*
 * WwdObjectRecord — the PINNED on-disk fixed object record (sizeof == 0x11C, 284
 * bytes), confirmed by WwdFile::ReadPlaneObjects @0x162af0 (`lea [esi+0x11c]` +
 * `rep movs` per object). After this fixed block come FOUR length-prefixed
 * (lengths at +0x04/+0x08/+0x0C/+0x10) variable strings: name, logic, imageSet,
 * sound (NOT NUL-terminated on disk).
 *
 * All fields are little-endian 32-bit. Offsets in comments are byte offsets into
 * the record. This is the load-bearing layout; the editor-grouped WwdObject below
 * is a name-rich convenience view only.
 */
struct WwdObjectRecord {
    i32 id;             // @0x000  "ID"
    u32 nameLength;     // @0x004  length of trailing name string
    u32 logicLength;    // @0x008  length of trailing logic string
    u32 imageSetLength; // @0x00C  length of trailing image-set string
    u32 soundLength;    // @0x010  length of trailing sound string ("animation" in libwap32)
    i32 x;              // @0x014  "X Coord:" (pixel)
    i32 y;              // @0x018  "Y Coord:"
    i32 z;              // @0x01C  "Z Coord:"
    i32 i;              // @0x020  frame index
    u32 addFlags;       // @0x024  Difficult/EyeCandy/HighDetail/...
    u32 dynamicFlags;   // @0x028  "Dynamic Flags" NoHit/AlwaysActive/Safe/AutoHitDamage
    u32 drawFlags;      // @0x02C  "Draw Flags" NoDraw/Mirror/Invert/Flash
    u32 userFlags;      // @0x030  "User Flags"
    i32 score;          // @0x034  "Score:"
    i32 points;         // @0x038  "Points:"
    i32 powerup;        // @0x03C  "Powerup:"
    i32 damage;         // @0x040  "Damage:"
    i32 smarts;         // @0x044  "Smarts:"
    i32 health;         // @0x048  "Health:"
    i32 moveRect[4];    // @0x04C  "Move Rect"   {l,t,r,b}
    i32 hitRect[4];     // @0x05C  "Hit Rect"
    i32 attackRect[4];  // @0x06C  "Attack Rect"
    i32 clipRect[4];    // @0x07C  "Clip Rect"
    i32 userRect1[4];   // @0x08C  "User1 Rect"
    i32 userRect2[4];   // @0x09C  "User2 Rect"
    i32 userValue1;     // @0x0AC  "User 1:"
    i32 userValue2;     // @0x0B0  "User 2:"
    i32 userValue3;     // @0x0B4  "User 3:"
    i32 userValue4;     // @0x0B8  "User 4:"
    i32 userValue5;     // @0x0BC  "User 5:"
    i32 userValue6;     // @0x0C0  "User 6:"
    i32 userValue7;     // @0x0C4  "User 7:"
    i32 userValue8;     // @0x0C8  "User 8:"
    i32 minX;           // @0x0CC  "xMin:"
    i32 minY;           // @0x0D0  "yMin:"
    i32 maxX;           // @0x0D4  "xMax:"
    i32 maxY;           // @0x0D8  "yMax:"
    i32 speedX;         // @0x0DC  "Speed X:"
    i32 speedY;         // @0x0E0  "Speed Y:"
    i32 tweakX;         // @0x0E4  "xTweak"
    i32 tweakY;         // @0x0E8  "yTweak"
    i32 counter;        // @0x0EC  "Counter:"
    i32 speed;          // @0x0F0  "Speed:"
    i32 width;          // @0x0F4  "Width:"
    i32 height;         // @0x0F8  "Height:"
    i32 direction;      // @0x0FC  "Direction:"
    i32 faceDir;        // @0x100  "Face Dir:"
    i32 timeDelay;      // @0x104  "Time Delay:"
    i32 frameDelay;     // @0x108  "Frame Delay:"
    u32 objectType;     // @0x10C  "Object Type" (single value)
    u32 hitTypeFlags;   // @0x110  "Hit Type Flags"
    i32 moveResX;       // @0x114  "xMoveRes:"
    i32 moveResY;       // @0x118  "yMoveRes:"
    // @0x11C (284): variable-length, length-prefixed (above), NOT NUL-terminated:
    //   char name[nameLength]; char logic[logicLength];
    //   char imageSet[imageSetLength]; char sound[soundLength];
}; // sizeof == 0x11C (284) — PINNED

/*
 * WwdObject — one world-object record (editor-grouped, name-rich VIEW).
 * NOT a byte-layout source — see WwdObjectRecord above for the PINNED on-disk
 * fixed 0x11C layout. Member ORDER here is editor-dialog grouping, not on-disk
 * order.
 *
 * Field NAMES are the editor's labels, in dialog grouping order. The game's
 * CObject (and every gameplay subclass: CGrunt, hazards, triggers, …) is built
 * from this record.
 */
struct WwdObject {
    /* --- Identification (Object-Properties dialog) --- */
    WwdCString name;  // "Name:"        object instance name (length-prefixed on disk)
    WwdCString logic; // "Logic:"       logic-program name (-> CUserLogic dispatch)
    WwdCString
        imageSet; // "Image Set:"   primary image-set name (length-prefixed STRING on disk, NOT an int)
    WwdCString
        sound; // trailing sound/"animation" string (length-prefixed on disk; editor omitted it)

    /* --- Attributes --- */
    i32 score;   // "Score:"
    i32 points;  // "Points:"      (editor exposes both Score and Points)
    i32 powerup; // "Powerup:"     (see enum Powerup)
    i32 damage;  // "Damage:"
    i32 smarts;  // "Smarts:"      AI level
    i32 health;  // "Health:"

    /* --- Motion / facing --- */
    i32 speedX;    // "Speed X:"
    i32 speedY;    // "Speed Y:"
    i32 speed;     // "Speed:"       (scalar speed, distinct from Speed X/Y)
    i32 faceDir;   // "Face Dir:"    (see enum Direction)
    i32 direction; // "Direction:"   movement direction (vs facing)

    /* --- Misc values (Object-Misc-Values dialog) --- */
    i32 counter;    // "Counter:"
    i32 width;      // "Width:"
    i32 height;     // "Height:"
    i32 timeDelay;  // "Time Delay:"
    i32 frameDelay; // "Frame Delay:"

    /* --- Position --- */
    i32 zCoord; // "Z Coord:"     draw/sort depth
    i32 xCoord; // "X Coord:"     pixel X
    i32 yCoord; // "Y Coord:"     pixel Y

    /* --- Movement bounds / tweaks (Object-Points dialog) --- */
    i32 xMin;     // "xMin:" / "X Min:"
    i32 xMax;     // "xMax:" / "X Max:"
    i32 yMin;     // "yMin:" / "Y Min:"
    i32 yMax;     // "yMax:" / "Y Max:"
    i32 xMoveRes; // "xMoveRes:"     movement resolution X
    i32 yMoveRes; // "yMoveRes:"     movement resolution Y
    i32 xTweak;   // "xTweak"
    i32 yTweak;   // "yTweak"

    /* --- User scalars (Object-User-Values dialog) --- */
    i32 userValue; // "User Value:"
    i32 user[8];   // "User 1:" .. "User 8:"  (1-based in the editor)

    /* --- Rectangles (Object-Rectangles dialog) --- */
    WwdRect hitRect;    // "Hit Rect"
    WwdRect attackRect; // "Attack Rect"
    WwdRect clipRect;   // "Clip Rect"
    WwdRect moveRect;   // "Move Rect"
    WwdRect user1Rect;  // "User1 Rect"
    WwdRect user2Rect;  // "User2 Rect"

    /* --- Flag words (Object-Flags / Object-Hit-Info dialogs) --- */
    u32 drawFlags;     // "Draw Flags"     (see WwdDrawFlags)
    u32 dynamicFlags;  // "Dynamic Flags"
    u32 userFlags;     // "User Flags"
    u32 hitTypeFlags;  // "Hit Type Flags" (see WwdHitTypeFlags)
    u32 objectFlags;   // "Object Flags"   (see WwdObjectFlags)
    i32 autoHitDamage; // "Auto Hit Damage"

    /* --- Image sets / prefixes (Add-Image-Sets dialog) ---
     * NOTE: these imageSetN/prefixN belong to the WWD LEVEL HEADER (see
     * WwdHeader.imageSet1..4 / prefix1..4 in wwd.h), NOT to the per-object
     * on-disk record (WwdObjectRecord) — they are reproduced here only because the
     * editor surfaces them via an object-adjacent dialog. */
    WwdCString imageSet1, imageSet2, imageSet3, imageSet4; // "Image Set 1:".."4:"
    WwdCString prefix1, prefix2, prefix3, prefix4;         // "Prefix 1:".."4:"
};

/*
 * Object Flags — the 1-bit toggles in the editor's "Object Flags" dialog, in label
 * order. These are the WAP32-engine object flag bits (shared with WapWorld/GMEdit).
 * Bit VALUES are unverified (label order != bit order until confirmed); names verbatim.
 */
enum WwdObjectFlags {
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
    // (bit positions/values unverified against the binary; label order only.)
};

/*
 * Hit Type Flags / Object Type — the object hit-class bits in the editor's
 * "Object Hit Info" dialog (the "Object Type" / "Hit Type Flags" list). Names
 * verbatim; the editor lists them twice (combo + checklist).
 */
enum WwdHitTypeFlags {
    WWD_HIT_GENERIC, // "Generic"
    WWD_HIT_PLAYER,  // "Player"
    WWD_HIT_ENEMY,   // "Enemy"
    WWD_HIT_POWERUP, // "Powerup"
    WWD_HIT_SHOT,    // "Shot"
    WWD_HIT_PSHOT,   // "P-Shot"   (player shot)
    WWD_HIT_ESHOT,   // "E-Shot"   (enemy shot)
    WWD_HIT_SPECIAL, // "Special"
    WWD_HIT_USER1,   // "User 1"
    WWD_HIT_USER2,   // "User 2"
    WWD_HIT_USER3,   // "User 3"
    WWD_HIT_USER4    // "User 4"
    // (bit positions/values unverified against the binary; label order only.)
};

/*
 * Dynamic / Draw / User flag words also expose a generic "Flag 1".."Flag 12"
 * checklist in the editor ("Add Flags"). Those 12 generic bits map to whichever
 * flag word is being edited; not modeled as a distinct enum. (Which word(s) the
 * 12 generic flags apply to, and their bit meanings, are unverified.)
 */

#include <rva.h>
// Class metadata (SIZE sweep) - comprehension-only header (not in the
// matching build); annotation is text-scanned tree-wide, emits no code.
SIZE_UNKNOWN(WwdObject);
SIZE_UNKNOWN(WwdObjectRecord);

#endif /* FORMATS_WWD_OBJECT_H */
