#ifndef GRUNTZ_GRUNTZ_SPELLZEFFECT_H
#define GRUNTZ_GRUNTZ_SPELLZEFFECT_H

#include <Enums.h>

// Scroll/Wand spell subtype, held in the WWD `Face Dir` field (offset not yet
// pinned) and dispatched in CGruntCombat. Cast by a Scroll (Toy 30) or a Magic
// Wand (Tool 19); 0 picks one of 1-6 at random.
//
// The 1-6 names are the live ones from the dispatch switch; SPELLZ_RANDOM comes
// from docs/domain/powerupz.md (editor/AppendixB/Spellz/0-RandomSpell.html).
GZ_ENUM_BEGIN(SpellzEffect)
    SPELLZ_RANDOM = 0,
    SPELLZ_FREEZE = 1,
    SPELLZ_HEALTH = 2,
    SPELLZ_RESURRECTION = 3,
    SPELLZ_TOYZ = 4,
    SPELLZ_TELEPORT = 5,
    SPELLZ_ROLLINGBALL = 6
GZ_ENUM_END(SpellzEffect)

#endif // GRUNTZ_GRUNTZ_SPELLZEFFECT_H
