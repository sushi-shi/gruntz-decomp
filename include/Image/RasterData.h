#ifndef GRUNTZ_IMAGE_RASTERDATA_H
#define GRUNTZ_IMAGE_RASTERDATA_H
#include <stddef.h>
#include <vector>
namespace raster {
enum DecodeError { Decoded, InvalidHeader, InvalidDimensions, MissingPalette,
    Truncated, InvalidRun, LimitExceeded };
struct Image {
    Image() : width(0), height(0), channels(0), flags(0), encodedBytes(0) {}
    unsigned int width, height, channels, flags;
    size_t encodedBytes;
    // Tight, top-down rows: palette indices or interleaved RGB.
    std::vector<unsigned char> pixels;
    // Empty or 256 RGB entries.
    std::vector<unsigned char> palette;
    void swap(Image& other);
};
// Byte-run state owns no storage; the input span is borrowed for the decode call.
class ByteRunReader {
public:
    ByteRunReader(const unsigned char* bytes, size_t length, bool compressed)
        : m_bytes(bytes), m_length(length), m_offset(0), m_repeat(0), m_value(0), m_compressed(compressed) {}
    DecodeError next(unsigned char& value);
    size_t consumed() const { return m_offset; }
    bool complete() const { return m_repeat == 0; }
private:
    const unsigned char* m_bytes;
    size_t m_length, m_offset;
    unsigned int m_repeat;
    unsigned char m_value;
    bool m_compressed;
};
// Failure leaves result unchanged. The limit includes padded decoded PCX rows.
bool convertSkipRunsTo16(const void* bytes, size_t length, unsigned int width, unsigned int height,
    const unsigned short* colors, std::vector<unsigned char>& result, size_t maxBytes = 128 * 1024 * 1024);
bool copyRows(const Image& image, void* destination, size_t capacity, size_t pitch,
    bool bgr, bool bottomUp);
DecodeError decodePcx(const void* bytes, size_t length, Image& result, size_t maxBytes = 64 * 1024 * 1024);
DecodeError decodePid(const void* bytes, size_t length, Image& result, size_t maxBytes = 64 * 1024 * 1024);
}
#endif
