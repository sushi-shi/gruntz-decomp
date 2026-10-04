#ifndef GRUNTZ_PORTABLE_TEST
#include <StdAfx.h>
#endif
#include <Image/RasterData.h>
#include <algorithm>
#include <string.h>
namespace raster {
static unsigned int word16(const unsigned char* p) {
    return static_cast<unsigned int>(p[0]) | (static_cast<unsigned int>(p[1]) << 8);
}
static unsigned int word32(const unsigned char* p) {
    return word16(p) | (word16(p + 2) << 16);
}
void Image::swap(Image& other) {
    std::swap(width, other.width); std::swap(height, other.height);
    std::swap(channels, other.channels); std::swap(flags, other.flags);
    std::swap(encodedBytes, other.encodedBytes);
    pixels.swap(other.pixels); palette.swap(other.palette);
}
DecodeError ByteRunReader::next(unsigned char& value) {
    if (!m_repeat) {
        if (m_offset == m_length) return Truncated;
        m_value = m_bytes[m_offset++];
        if (m_compressed && (m_value & 0xc0) == 0xc0) {
            m_repeat = m_value & 0x3f;
            if (!m_repeat) return InvalidRun;
            if (m_offset == m_length) return Truncated;
            m_value = m_bytes[m_offset++];
        } else m_repeat = 1;
    }
    --m_repeat;
    value = m_value;
    return Decoded;
}
static bool fits(unsigned int width, unsigned int height, unsigned int channels, size_t maximum) {
    return width && height && channels && width <= maximum / channels
        && height <= maximum / (static_cast<size_t>(width) * channels);
}
bool convertSkipRunsTo16(const void* bytes, size_t length, unsigned int width, unsigned int height,
    const unsigned short* colors, std::vector<unsigned char>& result, size_t maxBytes) {
    if (!bytes || !colors || !fits(width, height, 2, maxBytes)) return false;
    const unsigned char* data = static_cast<const unsigned char*>(bytes);
    size_t offset = 0;
    std::vector<unsigned char> output;
    for (unsigned int y = 0; y < height; ++y) {
        unsigned int x = 0;
        while (x < width) {
            if (offset == length) return false;
            const unsigned char token = data[offset++];
            const unsigned int count = token & 0x7f;
            if (!count || count > width - x) return false;
            const size_t written = (token & 0x80) ? 1 : 1 + count * 2;
            if (output.size() > maxBytes || written > maxBytes - output.size()) return false;
            output.push_back(token);
            if (!(token & 0x80)) {
                if (count > length - offset) return false;
                for (unsigned int i = 0; i < count; ++i) {
                    const unsigned short color = colors[data[offset++]];
                    output.push_back(static_cast<unsigned char>(color));
                    output.push_back(static_cast<unsigned char>(color >> 8));
                }
            }
            x += count;
        }
    }
    result.swap(output);
    return true;
}
bool copyRows(const Image& image, void* destination, size_t capacity, size_t pitch,
    bool bgr, bool bottomUp) {
    if (!destination || (image.channels != 1 && image.channels != 3)
        || !fits(image.width, image.height, image.channels, image.pixels.size())) return false;
    const size_t rowBytes = static_cast<size_t>(image.width) * image.channels;
    if (pitch < rowBytes || rowBytes > capacity || image.height - 1 > (capacity - rowBytes) / pitch) return false;
    unsigned char* target = static_cast<unsigned char*>(destination);
    for (size_t y = 0; y < image.height; ++y) {
        const unsigned char* src = &image.pixels[y * rowBytes];
        unsigned char* dst = target + (bottomUp ? image.height - y - 1 : y) * pitch;
        if (bgr && image.channels == 3) {
            for (size_t x = 0; x < image.width; ++x) {
                dst[x * 3] = src[x * 3 + 2]; dst[x * 3 + 1] = src[x * 3 + 1]; dst[x * 3 + 2] = src[x * 3];
            }
        } else memcpy(dst, src, rowBytes);
    }
    return true;
}
DecodeError decodePcx(const void* bytes, size_t length, Image& result, size_t maxBytes) {
    if (!bytes || length < 128) return InvalidHeader;
    const unsigned char* data = static_cast<const unsigned char*>(bytes);
    if (data[0] != 10 || data[2] > 1 || data[3] != 8 || (data[65] != 1 && data[65] != 3)) return InvalidHeader;
    const unsigned int left = word16(data + 4), top = word16(data + 6);
    const unsigned int right = word16(data + 8), bottom = word16(data + 10);
    if (right < left || bottom < top) return InvalidDimensions;
    Image decoded;
    decoded.width = right - left + 1; decoded.height = bottom - top + 1;
    decoded.channels = data[65];
    const unsigned int stride = word16(data + 66);
    if (stride < decoded.width) return InvalidDimensions;
    if (!fits(stride, decoded.height, decoded.channels, maxBytes)) return LimitExceeded;
    size_t payloadEnd = length;
    if (decoded.channels == 1) {
        if (length < 128 + 769 || data[length - 769] != 12) return MissingPalette;
        payloadEnd -= 769;
        decoded.palette.assign(data + payloadEnd + 1, data + length);
    }
    decoded.pixels.resize(static_cast<size_t>(decoded.width) * decoded.height * decoded.channels);
    ByteRunReader reader(data + 128, payloadEnd - 128, data[2] != 0);
    for (unsigned int y = 0; y < decoded.height; ++y) {
        for (unsigned int plane = 0; plane < decoded.channels; ++plane) {
            for (unsigned int x = 0; x < stride; ++x) {
                unsigned char value;
                const DecodeError error = reader.next(value);
                if (error != Decoded) return error;
                if (x < decoded.width) decoded.pixels[(static_cast<size_t>(y) * decoded.width + x) * decoded.channels + plane] = value;
            }
        }
    }
    if (!reader.complete()) return InvalidRun;
    decoded.encodedBytes = reader.consumed();
    result.swap(decoded);
    return Decoded;
}
DecodeError decodePid(const void* bytes, size_t length, Image& result, size_t maxBytes) {
    if (!bytes || length < 32) return InvalidHeader;
    const unsigned char* data = static_cast<const unsigned char*>(bytes);
    if (word32(data) != 10 && word32(data) != 0) return InvalidHeader;
    Image decoded;
    decoded.flags = word32(data + 4); decoded.width = word32(data + 8); decoded.height = word32(data + 12);
    decoded.channels = 1;
    if (!decoded.width || !decoded.height || decoded.width > 0x7fffffffU || decoded.height > 0x7fffffffU) return InvalidDimensions;
    if (!fits(decoded.width, decoded.height, 1, maxBytes)) return LimitExceeded;
    size_t payloadEnd = length;
    if (decoded.flags & 0x80) {
        if (length < 32 + 768) return MissingPalette;
        payloadEnd -= 768;
        decoded.palette.assign(data + payloadEnd, data + length);
    }
    decoded.pixels.resize(static_cast<size_t>(decoded.width) * decoded.height);
    if (decoded.flags & 0x20) {
        const unsigned char fill = (decoded.flags & 0x100) ? static_cast<unsigned char>(word32(data + 24)) : 0;
        size_t offset = 32;
        for (unsigned int y = 0; y < decoded.height; ++y) {
            unsigned int x = 0;
            while (x < decoded.width) {
                if (offset == payloadEnd) return Truncated;
                const unsigned char token = data[offset++];
                const unsigned int count = token & 0x7f;
                if (!count || count > decoded.width - x) return InvalidRun;
                unsigned char* target = &decoded.pixels[static_cast<size_t>(y) * decoded.width + x];
                if (token & 0x80) memset(target, fill, count);
                else {
                    if (count > payloadEnd - offset) return Truncated;
                    memcpy(target, data + offset, count); offset += count;
                }
                x += count;
            }
        }
        decoded.encodedBytes = offset - 32;
    } else {
        ByteRunReader reader(data + 32, payloadEnd - 32, true);
        for (size_t i = 0; i < decoded.pixels.size(); ++i) {
            const DecodeError error = reader.next(decoded.pixels[i]);
            if (error != Decoded) return error;
        }
        if (!reader.complete()) return InvalidRun;
        decoded.encodedBytes = reader.consumed();
    }
    result.swap(decoded);
    return Decoded;
}
}
