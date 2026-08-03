#ifndef GRUNTZ_GRUNTZ_MOVIEID_H
#define GRUNTZ_GRUNTZ_MOVIEID_H

#include <Enums.h>

// Which cutscene CGruntzMgr::BuildMoviePath resolves, named by the file its own
// arm returns - the filenames are in the binary's string table beside the
// "%c:\MOVIEZ\%s" template.
//
// The id sequence is deliberately not contiguous: -1, then 0, 2, 4, 6, 8, 10, 12
// in steps of two, then 13 and 14 adjacent. Whatever produces it is not in the
// reconstructed tree yet (BuildMoviePath has no caller here), so the step is
// recorded rather than explained.
//
// The main menu's MOVIEZ page has its own entries - MENU_MOVIEZ_{INTRO, TITLE,
// FINAL, CREDITZ, LOGO} - but there are six of those against ten ids and nothing
// maps them, so they are NOT used as names here.
GZ_ENUM_BEGIN(MovieId)
    MOVIE_LOGO = -1,
    MOVIE_GRUNTZ0 = 0,
    MOVIE_GRUNTZ1 = 2,
    MOVIE_GRUNTZ2 = 4,
    MOVIE_GRUNTZ3 = 6,
    MOVIE_GRUNTZ4 = 8,
    MOVIE_GRUNTZ5 = 10,
    MOVIE_GRUNTZ6 = 12,
    MOVIE_GRUNTZ7 = 13,
    MOVIE_GRUNTZ8 = 14
GZ_ENUM_END(MovieId)

#endif // GRUNTZ_GRUNTZ_MOVIEID_H
