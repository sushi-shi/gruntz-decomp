#include <StdAfx.h>
#include <Image/Image.h>
#include <Image/ImagePaletteNode.h>
#include <Image/RasterData.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
int main() {
    raster::Image image;
    image.width = 3; image.height = 2; image.channels = 3;
    for (unsigned char i = 0; i < 18; ++i) image.pixels.push_back(i);
    {
        CDib dib;
        assert(dib.InitRaster(image, NULL, 0));
        assert(dib.GetPitch() == 12 && dib.GetBufferSize() == 24 && dib.GetStride() == 3);
        for (size_t y = 0; y < 2; ++y) {
            const unsigned char* row = dib.GetAddress(y);
            for (size_t x = 0; x < 3; ++x) for (size_t c = 0; c < 3; ++c)
                assert(row[x * 3 + c] == image.pixels[(y * 3 + x) * 3 + 2 - c]);
        }
        const unsigned char before = dib.GetAddress(0)[0];
        unsigned char truncated[4] = {10, 5, 1, 8};
        assert(!dib.InitPcx(truncated, sizeof(truncated), NULL));
        assert(dib.GetPitch() == 12 && dib.GetAddress(0)[0] == before);
    }
    {
        CDib dib;
        assert(!dib.Init(NULL, 1, (-2147483647 - 1), BPP_RGB_24));
        assert(!dib.Init(NULL, 0x7fffffff, 2, BPP_RGB_24));
        assert(dib.Init(NULL, 3, 2, BPP_RGB_16));
        assert(dib.GetPitch() == 8 && dib.GetBufferSize() == 16 && dib.GetStride() == 2);
        assert(dib.GetAddress(0) - dib.GetAddress(1) == 8);
    }
    {
        CDib dib;
        assert(dib.Init(NULL, 3, 2, BPP_RGB_32));
        assert(dib.GetPitch() == 12 && dib.GetBufferSize() == 24);
    }
    {
        CDib dib;
        const unsigned char pixels[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18};
        unsigned char owned[18]; memcpy(owned, pixels, sizeof(owned));
        assert(dib.Init(owned, NULL, 3, 2, BPP_RGB_24));
        assert(memcmp(dib.GetAddress(0), pixels, 9) == 0 && memcmp(dib.GetAddress(1), pixels + 9, 9) == 0);
    }
    {
        CDib source;
        CDib converted;
        CDibPal palette;
        memset(palette.GetPes(), 0, 256 * sizeof(PALETTEENTRY));
        palette.GetPes()[1].peRed = 255;
        palette.GetPes()[2].peBlue = 255;
        HDC dc = CreateCompatibleDC(NULL);
        assert(dc);
        assert(source.Init(dc, 3, -2, BPP_PALETTED_8));
        DeleteDC(dc);
        memset(source.GetAddress(0), 1, 3); memset(source.GetAddress(1), 2, 3);
        assert(converted.Init(NULL, &source, &palette));
        assert(*reinterpret_cast<u16*>(converted.GetAddress(0)) == 0x7c00);
        assert(*reinterpret_cast<u16*>(converted.GetAddress(1)) == 0x001f);
    }
    {
        const unsigned char pixels[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24};
        unsigned char owned[24]; memcpy(owned, pixels, sizeof(owned));
        for (int width = 3; width <= 4; ++width) {
            CDib dib;
            assert(dib.Init(owned, NULL, width, 2, BPP_RGB_24, 0, RASTER_ROWS_BOTTOM_UP));
            assert(memcmp(dib.GetAddress(0), pixels + width * 3, width * 3) == 0);
            assert(memcmp(dib.GetAddress(1), pixels, width * 3) == 0);
        }
    }
    puts("DIB production raster upload and byte-pitch tests passed.");
    return 0;
}
