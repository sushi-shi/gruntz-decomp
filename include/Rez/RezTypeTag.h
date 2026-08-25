#ifndef GRUNTZ_REZ_REZTYPETAG_H
#define GRUNTZ_REZ_REZTYPETAG_H

#include <Enums.h>

// REZ archive entry-type tags: the three-character type packed into a dword.
// ONE domain - CRezArchiveEntry::GetTypeTag() returns it and callers compare it
// against both families below. The IMGTAG_* names were a second enum
// (include/Image/ImageFormatTag.h) over the same field; merged here.
// Retail's switch key is UNSIGNED: CImage::Resolve/Reload and
// CDDrawSurfacePair::LoadImageA all dispatch the tag with `ja`, not `jg`
// (`sema disasm 0x00152f20 --branches --diff`). So the domain is returned as u32.
GZ_ENUM_BEGIN_SPLIT(RezTypeTag, u32)
// 0 = no type filter: CRezArchiveDir::FindEntry/FindEntryByFilename accept it to mean
// "any entry", the same wildcard role TRIGID_ANY plays.
    REZ_TAG_NONE = 0,
    REZ_TAG_WWD = 0x575744,
    REZ_TAG_WAV = 0x574156,
    REZ_TAG_ANI = 0x414e49,
    REZ_TAG_TXT = 0x545854,
    REZ_TAG_PAL = 0x50414c,
    // 'XMI' - the MIDIZ resource format, handed to the same CRezArchiveDir::FindEntry
    // type-tag parameter as REZ_TAG_WWD (CPlay::BuildMusicCategoryTable).
    REZ_TAG_XMI = 0x584d49,

    // The image formats, same field, retail's own spellings.
    IMGTAG_PMB = 0x424d50,
    IMGTAG_XCP = 0x504358,
    IMGTAG_DIR = 0x524944,
    IMGTAG_DIP = 0x504944
GZ_ENUM_END_SPLIT(RezTypeTag, u32)

#endif // GRUNTZ_REZ_REZTYPETAG_H
