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
 * (STRINGS_ANALYSIS.md §6). NOT in RTTI. NO layout recovered — name-only stub.
 *
 * This is a Monolith class reused from the LithTech lineage. It parses INI-like
 * "attribute" text databases (attributez.txt, dwrects.txt): tagged-section format
 * ([tag] with typed key/value symbols). Reported error codes (from strings):
 * formatting error, bad symbol, invalid token, duplicate symbol/tag, invalid tag,
 * symbol-not-found, type-mismatch.
 */

class ButeMgr { /* attribute-file parser; layout unknown */ };

#endif /* MANAGERS_BUTEMGR_H */
