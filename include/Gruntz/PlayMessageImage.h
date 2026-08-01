#ifndef GRUNTZ_PLAYMESSAGEIMAGE_H
#define GRUNTZ_PLAYMESSAGEIMAGE_H

#include <Mfc.h>
#include <Ints.h>
#include <rva.h>

class CDDrawSurfaceMgr;
class CImage;

i32 LayerBlitFrame(CDDrawSurfaceMgr*, CImage*, i32, i32, i32, i32);

#endif // GRUNTZ_PLAYMESSAGEIMAGE_H
