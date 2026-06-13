#ifndef MANAGERS_BUTEMGR_H
#define MANAGERS_BUTEMGR_H

/*
 * ButeMgr — attribute-file ("Bute") config parser.
 *
 * No leaked .cpp path; the name is mined from the parser's own error strings:
 *   "ButeMgr (%d):  A formatting error in the attribute file was encountered"
 *   "ButeMgr:  duplicate {symbol|tag} encountered - %s"
 *   "ButeMgr:  Invalid tag specified - [%s]    Symbol not found - [%s]:%s"
 *   "ButeMgr:  Type mismatch - [%s]:%s"
 * (STRINGS_ANALYSIS.md §6). NOT in RTTI — no @rtti tag. NO layout recovered.
 *
 * This is a Monolith class reused from the LithTech lineage. It parses INI-like
 * "attribute" text databases:
 *   - attributez.txt / Attributez.txt  (game attribute database)
 *   - dwrects.txt                       (dword-rect table)
 * Tagged-section format ([tag] with typed key/value symbols).
 */

#undef UNICODE
#undef _UNICODE
#include <afxwin.h>

class ButeMgr
{
public:
    //@size: unknown @todo
    //@todo: Parse(path); typed getters (GetInt/GetString/GetRect/GetDword...);
    //       symbol table keyed by [tag]:symbol. Layout unknown.
    //
    // Error codes it can report (from strings): formatting error, bad symbol,
    // invalid token, duplicate symbol/tag, invalid tag, symbol-not-found,
    // type-mismatch.
};

#endif /* MANAGERS_BUTEMGR_H */
