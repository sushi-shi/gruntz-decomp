// PlayMessageImage.h - the PlayMessageImage TU's external declarations.
#ifndef GRUNTZ_PLAYMESSAGEIMAGE_H
#define GRUNTZ_PLAYMESSAGEIMAGE_H

#include <Mfc.h> // afx.h FIRST (umbrella for any Win32 types below)
#include <Ints.h>
#include <rva.h>

class CDDrawSurfaceMgr;
class CImage;

i32 LayerBlitFrame(CDDrawSurfaceMgr*, CImage*, i32, i32, i32, i32); // 0x115300

#endif // GRUNTZ_PLAYMESSAGEIMAGE_H
