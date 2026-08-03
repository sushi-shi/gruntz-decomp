#ifndef GRUNTZ_REZ_REZTYPETAG_H
#define GRUNTZ_REZ_REZTYPETAG_H

#include <Enums.h>

// REZ archive entry-type tags: the three-character type packed into a dword.
// ONE domain - CParseSource::GetEntryTag() returns it and callers compare it
// against both families below. The IMGTAG_* names were a second enum
// (include/Image/ImageFormatTag.h) over the same field; merged here.
GZ_ENUM_BEGIN(RezTypeTag)
// 0 = no type filter: CSymTab::Insert/Find accept it to mean
// "any entry", the same wildcard role TRIGID_ANY plays.
    REZ_TAG_NONE = 0,
    REZ_TAG_WWD = 0x575744,
    REZ_TAG_WAV = 0x574156,
    REZ_TAG_ANI = 0x414e49,
    REZ_TAG_TXT = 0x545854,
    // 'XMI' - the MIDIZ bank format, handed to the same CSymTab::Insert
    // type-tag parameter as REZ_TAG_WWD (CPlay::BuildMusicCategoryTable).
    REZ_TAG_XMI = 0x584d49,

    // The image formats, same field, retail's own spellings.
    IMGTAG_PMB = 0x424d50,
    IMGTAG_XCP = 0x504358,
    IMGTAG_DIR = 0x524944,
    IMGTAG_DIP = 0x504944,

    // A third spelling that lived in ParseSource.h over the same field.
    PARSETAG_VAW = REZ_TAG_WAV,
    PARSETAG_INA = REZ_TAG_ANI
GZ_ENUM_END(RezTypeTag)

#endif // GRUNTZ_REZ_REZTYPETAG_H
