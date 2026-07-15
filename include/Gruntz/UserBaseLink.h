// UserBaseLink.h - CUserBaseLink, the destructible link sub-object embedded at
// CUserLogic+0x18 and CGrunt+0x18. Its only field is a zBitVec "name" (the Ghidra
// "EngStr", now <Wap32/zBitVec.h>). Its out-of-line ctor (0x16d710) is the one
// constructor the whole game-object family chains; it can throw, which is what makes
// MSVC emit the /GX EH frame in every leaf ctor/dtor. Shared so both the tile-logic
// family (<Gruntz/UserLogic.h>) and the CGrunt world (<Gruntz/Grunt.h>) embed the SAME
// +0x18 sub-object and tear it down via the identical ~zBitVec (0x16d2a0).
//
// Its own module (NOT folded into <Wap32/zBitVec.h>): CUserBaseLink embeds a zBitVec,
// and pulling that into src/Wap32/EngStr.cpp header-fattens/reschedules
// zBitVec::SetSize (measured 98.7% -> 85.3%). EngStr.cpp includes only zBitVec.h.
#ifndef GRUNTZ_USERBASELINK_H
#define GRUNTZ_USERBASELINK_H

#include <Wap32/zBitVec.h>
#include <rva.h>

// The global empty C string the link's name field is seeded from (0x6293f4).
#include <EmptyString.h> // g_emptyString (the shared "" constant)

SIZE_UNKNOWN(CUserBaseLink);
struct CUserBaseLink {
    CUserBaseLink();    // 0x16d710 (out-of-line; can throw)
    ~CUserBaseLink() {} // inline: folds to the embedded ~zBitVec call in leaf dtors
    zBitVec m_str;      // +0x00  (its name field; the 0x5f04c8 zBitVec vptr)
};

#endif // GRUNTZ_USERBASELINK_H
