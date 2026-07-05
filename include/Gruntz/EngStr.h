// EngStr.h - the engine's small string class + the CUserLogic/CGrunt link
// sub-object that embeds it (C:\Proj\Gruntz shared incs).
//
// SHARED so BOTH the tile-logic game-object family (<Gruntz/UserLogic.h>) and the
// CGrunt world (<Gruntz/Grunt.h>) embed the SAME +0x18 link sub-object. Before
// this split each header carried its own link model (UserLogic.h: CUserBaseLink /
// EngStr; Grunt.h: a placeholder GruntLinkSub whose out-of-line Dtor() did NOT
// delink to ~EngStr 0x16d2a0), so CGrunt's link teardown scored fuzzy. Both worlds
// now call the real ~EngStr (0x16d2a0), matching retail (~CGrunt @0xf39d).
//
// EngStr layout {vptr@0, ?@4, len@8, buf@0xc}; size 0x10. Its three operations
// live in the engine string TU, modeled NO-body so the calls reloc-mask:
//   EngStr(char const*, int) = 0x16d3a0  (836B; construct from a C string)
//   operator=(EngStr const&) = 0x16d2f0  (172B; deep copy-assign)
//   ~EngStr()                = 0x16d2a0  (38B)
//
// IDENTITY (matcher-2, proven 2026-07-05): this "EngStr" IS the zBitVec /
// CContainerErr small-buffer container already modeled in <Gruntz/ProjActCache.h>
// (RTTI-less vtable 0x5f04c8 == ??_7zBitVec; base CContainerErr @0x16d9c0/0x16da60;
// SetSize @0x16e100 == zBitVec::SetSize). ~EngStr @0x16d2a0 stamps 0x5f04c8, frees
// buf when cap>0x20, chains ~CContainerErr; operator= @0x16d2f0 reallocs+memcpys
// cap/8 bytes; the 836B ctor @0x16d3a0 is a whitespace-delimited numeric-token
// PARSER (not a string copy). Reconstructing the four requires the polymorphic
// zBitVec/CContainerErr shape here, but this class is embedded in EVERY leaf ctor
// (CUserBaseLink::m_str), CContainerErr is defined THREE times (here as EngStr,
// <Wap32/EngStr.h>, <Gruntz/ProjActCache.h>), and the two consumers are the game's
// two widest headers -> a dedicated 3-way-dedup + butterfly pass, not a drive-by.
// Kept NO-body (correctly reloc-masking the leaf ctors) pending that pass.
#ifndef GRUNTZ_ENGSTR_H
#define GRUNTZ_ENGSTR_H

#include <rva.h>

SIZE_UNKNOWN(EngStr);
struct EngStr {
    EngStr(); // default (unused; lets the link's empty ctor stub compile)
    EngStr(const char* s, i32 n);
    ~EngStr();
    EngStr& operator=(const EngStr& o);
    void* m_0;
    i32 m_4;
    i32 m_8;
    char* m_c;
};

// The global empty C string the link's name field is seeded from (0x6293f4).
extern "C" char g_emptyString[];

// ---------------------------------------------------------------------------
// CUserBaseLink - the destructible sub-object embedded at CUserLogic+0x18 (and
// CGrunt+0x18). Its only field is an EngStr name. Its ctor (0x16d710) is the one
// out-of-line constructor the whole game-object family chains; it can throw,
// which is what makes MSVC emit the /GX EH frame in every leaf ctor/dtor.
// ---------------------------------------------------------------------------
SIZE_UNKNOWN(CUserBaseLink);
struct CUserBaseLink {
    CUserBaseLink();    // 0x16d710 (out-of-line; can throw)
    ~CUserBaseLink() {} // inline: folds to the embedded ~EngStr call in leaf dtors
    EngStr m_str;       // +0x00  (its name field; the 0x5f04c8 EngStr vptr)
};

#endif // GRUNTZ_ENGSTR_H
