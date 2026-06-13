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
 * PROVENANCE: field NAMES are mined VERBATIM from the editor's Object-Properties /
 * Object-Rectangles / Object-Points / Object-Flags / Object-User-Values /
 * Object-Misc-Values / Object-Hit-Info dialog labels (build/editor-strings/
 * GruntzEdit.utf16.txt). FIELD ORDER and the exact on-disk byte layout/types are
 * NOT yet confirmed against the binary loader — every @offset is @todo. The names
 * and the field SET are high confidence; ordering/widths are not.
 *
 * Coordinates: WWD uses pixel-space X/Y (not tile indices); tiles are a separate
 * plane array. Rects are inclusive {left,top,right,bottom} (editor labels them
 * Left/Top/Right/Bottom).
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
 * WwdObject — one world-object record.
 * @size: unknown @todo   (offsets/widths not confirmed against the loader)
 *
 * Field NAMES are the editor's labels, in dialog grouping order. The game's
 * CObject (and every gameplay subclass: CGrunt, hazards, triggers, …) is built
 * from this record.
 */
struct WwdObject
{
    /* --- Identification (Object-Properties dialog) --- */
    CString name;        // "Name:"        object instance name
    CString logic;       // "Logic:"       logic-program name (-> CUserLogic dispatch)
    int     imageSet;    // "Image Set:"   primary image-set index/key

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

    /* --- Image sets / prefixes (Add-Image-Sets dialog) --- */
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
