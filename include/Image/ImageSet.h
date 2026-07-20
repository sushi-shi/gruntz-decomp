#ifndef SRC_IMAGE_IMAGESET_H
#define SRC_IMAGE_IMAGESET_H
#include <rva.h>

#include <Ints.h>

#include <Image/CImage.h> // the frame element IS the RTTI CImage (was the CImageFrame view)

#include <DDrawMgr/DDrawWorker.h> // the ONE real class (vtbl 0x1efbe8, CLoadable-derived)

typedef CDDrawWorker CImageSet;

#endif // SRC_IMAGE_IMAGESET_H
