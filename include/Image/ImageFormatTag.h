// ImageFormatTag.h - the 4-character image/page format codes the resource layer
// dispatches on. Shared: CImage's two loader switches, CDDrawSurfacePair's entry-tag
// gate and every CSymTab::ResolveQualified screen-page lookup use the SAME words.
#ifndef IMAGE_IMAGEFORMATTAG_H
#define IMAGE_IMAGEFORMATTAG_H

enum ImageFormatTag {
    IMGTAG_PMB = 0x424d50, // 'BMP' reversed - the BMP loader (index 1)
    IMGTAG_XCP = 0x504358, // 'PCX' reversed - the PCX loader (index 2)
    IMGTAG_DIR = 0x524944, // 'RID' - loader index 3
    IMGTAG_DIP = 0x504944, // 'PID' reversed - loader index 4
};

#endif // IMAGE_IMAGEFORMATTAG_H
