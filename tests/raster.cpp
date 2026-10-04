#include <Image/RasterData.h>
#include <Rez/ArchiveData.h>
#include <Io/File.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>
static void word(std::vector<unsigned char>& bytes, size_t offset, unsigned int value, size_t count) {
    for (size_t i = 0; i < count; ++i) bytes[offset + i] = static_cast<unsigned char>(value >> (8 * i));
}
static std::vector<unsigned char> pcx(unsigned int planes) {
    std::vector<unsigned char> bytes(128, 0);
    bytes[0] = 10; bytes[1] = 5; bytes[2] = 1; bytes[3] = 8; bytes[65] = planes;
    word(bytes, 8, 2, 2); word(bytes, 10, 1, 2); word(bytes, 66, 4, 2);
    for (unsigned int y = 0; y < 2; ++y) {
        for (unsigned int plane = 0; plane < planes; ++plane) {
            for (unsigned int x = 0; x < 4; ++x) bytes.push_back(static_cast<unsigned char>(y * 40 + plane * 10 + x));
        }
    }
    if (planes == 1) {
        bytes.push_back(12);
        for (size_t i = 0; i < 768; ++i) bytes.push_back(static_cast<unsigned char>(i));
    }
    return bytes;
}
static std::vector<unsigned char> pid(unsigned int flags) {
    std::vector<unsigned char> bytes(32, 0);
    word(bytes, 0, 10, 4); word(bytes, 4, flags, 4); word(bytes, 8, 3, 4); word(bytes, 12, 2, 4);
    word(bytes, 24, 9, 4);
    if (flags & 0x20) {
        bytes.push_back(0x82); bytes.push_back(1); bytes.push_back(7);
        bytes.push_back(3); bytes.push_back(1); bytes.push_back(2); bytes.push_back(3);
    } else {
        bytes.push_back(0xc4); bytes.push_back(5); bytes.push_back(6); bytes.push_back(7);
    }
    if (flags & 0x80) bytes.resize(bytes.size() + 768, 33);
    return bytes;
}
static raster::DecodeError decode(const std::vector<unsigned char>& bytes, bool isPcx, raster::Image& result) {
    const void* data = bytes.empty() ? NULL : &bytes[0];
    return isPcx ? raster::decodePcx(data, bytes.size(), result) : raster::decodePid(data, bytes.size(), result);
}
static void invalid(const std::vector<unsigned char>& bytes, bool isPcx) {
    raster::Image result; result.width = 123; result.pixels.push_back(42);
    assert(decode(bytes, isPcx, result) != raster::Decoded);
    assert(result.width == 123 && result.pixels.size() == 1 && result.pixels[0] == 42);
}
int main(int argc, char** argv) {
    raster::Image result;
    for (unsigned int planes = 1; planes <= 3; planes += 2) {
        const std::vector<unsigned char> valid = pcx(planes);
        assert(decode(valid, true, result) == raster::Decoded);
        assert(result.width == 3 && result.height == 2 && result.channels == planes);
        for (size_t y = 0; y < 2; ++y) for (size_t x = 0; x < 3; ++x)
            for (size_t p = 0; p < planes; ++p) assert(result.pixels[(y * 3 + x) * planes + p] == y * 40 + p * 10 + x);
        for (size_t size = 0; size < valid.size(); ++size) invalid(std::vector<unsigned char>(valid.begin(), valid.begin() + size), true);
        std::vector<unsigned char> bad = valid; bad[128] = 0xc0; invalid(bad, true);
        bad = valid; word(bad, 66, 2, 2); invalid(bad, true);
        bad = valid; word(bad, 4, 4, 2); invalid(bad, true);
        bad = valid; bad[65] = 4; invalid(bad, true);
        assert(raster::decodePcx(&valid[0], valid.size(), result, 1) == raster::LimitExceeded);
    }
    {
        std::vector<unsigned char> raw = pcx(3); raw[2] = 0; raw[128] = 0xc0;
        assert(decode(raw, true, result) == raster::Decoded && result.pixels[0] == 0xc0);
        std::vector<unsigned char> run = pcx(3); run.resize(130); run[128] = 0xd8; run[129] = 77;
        assert(decode(run, true, result) == raster::Decoded);
        for (size_t i = 0; i < result.pixels.size(); ++i) assert(result.pixels[i] == 77);
        run[128] = 0xd9; invalid(run, true);
        run.resize(129); invalid(run, true);
    }
    {
        const std::vector<unsigned char> bytes = pcx(3);
        assert(decode(bytes, true, result) == raster::Decoded);
        std::vector<unsigned char> target(28, 0xee);
        assert(raster::copyRows(result, &target[2], 24, 12, true, true));
        assert(target[0] == 0xee && target[1] == 0xee && target[26] == 0xee && target[27] == 0xee);
        for (size_t y = 0; y < 2; ++y) {
            for (size_t x = 0; x < 3; ++x) for (size_t c = 0; c < 3; ++c)
                assert(target[2 + (1 - y) * 12 + x * 3 + c] == result.pixels[(y * 3 + x) * 3 + (2 - c)]);
            for (size_t p = 9; p < 12; ++p) assert(target[2 + y * 12 + p] == 0xee);
        }
        const std::vector<unsigned char> before = target;
        assert(!raster::copyRows(result, &target[2], 20, 12, true, false));
        assert(!raster::copyRows(result, &target[2], 24, 8, true, false));
        assert(!raster::copyRows(result, NULL, 24, 12, true, false));
        assert(target == before);
        assert(raster::copyRows(result, &target[2], 24, 12, false, false));
        assert(memcmp(&target[2], &result.pixels[0], 9) == 0);
        assert(memcmp(&target[14], &result.pixels[9], 9) == 0);
        result.pixels.resize(1);
        assert(!raster::copyRows(result, &target[2], 24, 12, false, false));
        assert(decode(pcx(1), true, result) == raster::Decoded);
        assert(raster::copyRows(result, &target[2], 24, 4, true, false));
        assert(memcmp(&target[2], &result.pixels[0], 3) == 0);
        assert(memcmp(&target[6], &result.pixels[3], 3) == 0);
    }
    const unsigned int flags[] = {0, 0x80, 0x120, 0x1a0};
    for (size_t f = 0; f < sizeof(flags) / sizeof(flags[0]); ++f) {
        const std::vector<unsigned char> valid = pid(flags[f]);
        assert(decode(valid, false, result) == raster::Decoded);
        assert(result.width == 3 && result.height == 2 && result.channels == 1);
        const unsigned char skipExpected[] = {9, 9, 7, 1, 2, 3};
        const unsigned char runExpected[] = {5, 5, 5, 5, 6, 7};
        assert(std::memcmp(&result.pixels[0], flags[f] & 0x20 ? skipExpected : runExpected, 6) == 0);
        for (size_t size = 0; size < valid.size(); ++size) invalid(std::vector<unsigned char>(valid.begin(), valid.begin() + size), false);
        std::vector<unsigned char> bad = valid; bad[32] = flags[f] & 0x20 ? 0x84 : 0xc7; invalid(bad, false);
        bad = valid; bad[32] = flags[f] & 0x20 ? 0 : 0xc0; invalid(bad, false);
        bad = valid; word(bad, 8, 0xffffffffU, 4); invalid(bad, false);
        bad = valid; word(bad, 12, 0, 4); invalid(bad, false);
        assert(raster::decodePid(&valid[0], valid.size(), result, 1) == raster::LimitExceeded);
    }
    {
        // The final pixel of each row is a separate packet. A width-1 boundary
        // would drop it and misinterpret subsequent rows during 16-bit remapping.
        const unsigned char runs[] = {2, 1, 2, 1, 3, 0x82, 1, 4};
        unsigned short colors[256];
        for (size_t i = 0; i < 256; ++i) colors[i] = static_cast<unsigned short>(0x1200 + i);
        std::vector<unsigned char> converted;
        assert(raster::convertSkipRunsTo16(runs, sizeof(runs), 3, 2, colors, converted));
        const unsigned char expected[] = {2, 1, 0x12, 2, 0x12, 1, 3, 0x12, 0x82, 1, 4, 0x12};
        assert(converted.size() == sizeof(expected) && memcmp(&converted[0], expected, sizeof(expected)) == 0);
        const std::vector<unsigned char> previous = converted;
        for (size_t size = 0; size < sizeof(runs); ++size) {
            assert(!raster::convertSkipRunsTo16(runs, size, 3, 2, colors, converted));
            assert(converted == previous);
        }
        assert(!raster::convertSkipRunsTo16(runs, sizeof(runs), 3, 2, NULL, converted));
        assert(!raster::convertSkipRunsTo16(runs, sizeof(runs), 3, 2, colors, converted, 1));
    }
    {
        std::vector<unsigned char> bytes = pid(0x20);
        const size_t encoded = bytes.size() - 32;
        bytes.resize(bytes.size() + 64, 0xff);
        assert(decode(bytes, false, result) == raster::Decoded && result.encodedBytes == encoded);
        bytes = pid(0);
        const size_t byteRunSize = bytes.size() - 32;
        bytes.resize(bytes.size() + 64, 0xff);
        assert(decode(bytes, false, result) == raster::Decoded && result.encodedBytes == byteRunSize);
    }
    {
        std::vector<unsigned char> legacy = pid(0x20);
        word(legacy, 0, 0, 4);
        assert(decode(legacy, false, result) == raster::Decoded);
        word(legacy, 0, 11, 4);
        invalid(legacy, false);
    }
    unsigned int random = 27;
    for (size_t trial = 0; trial < 3000; ++trial) {
        const bool isPcx = (trial & 1) != 0;
        std::vector<unsigned char> bytes = isPcx ? pcx(3) : pid(0x1a0);
        for (size_t n = 0; n < 3; ++n) {
            random = random * 1664525U + 1013904223U; const size_t at = random % bytes.size();
            random = random * 1664525U + 1013904223U; bytes[at] = static_cast<unsigned char>(random >> 24);
        }
        if (isPcx) raster::decodePcx(&bytes[0], bytes.size(), result, 1024 * 1024);
        else raster::decodePid(&bytes[0], bytes.size(), result, 1024 * 1024);
    }
    for (int arg = 1; arg < argc; ++arg) {
        io::File file; rez::Archive archive;
        assert(file.open(argv[arg], io::ReadOnly)); assert(rez::decode(file, archive) == rez::Decoded);
        size_t pcxCount = 0, pidCount = 0;
        for (size_t d = 0; d < archive.directories.size(); ++d) {
            const std::vector<rez::Resource>& resources = archive.directories[d].resources;
            for (size_t r = 0; r < resources.size(); ++r) {
                const rez::Resource& member = resources[r];
                if (member.type != 0x504358 && member.type != 0x504944) continue;
                std::vector<unsigned char> bytes(member.size);
                assert(file.readAt(member.offset, bytes.empty() ? NULL : &bytes[0], bytes.size()));
                const bool isPcx = member.type == 0x504358;
                const raster::DecodeError error = decode(bytes, isPcx, result);
                if (error != raster::Decoded) std::fprintf(stderr, "Image decode error %d: %s/%s (%s)\n", error,
                    archive.directories[d].name.c_str(), member.name.c_str(), isPcx ? "PCX" : "PID");
                assert(error == raster::Decoded);
                if (isPcx) ++pcxCount; else ++pidCount;
            }
        }
        std::printf("Decoded %lu PCX and %lu PID members from %s\n", static_cast<unsigned long>(pcxCount), static_cast<unsigned long>(pidCount), argv[arg]);
    }
    std::puts("Bounded PCX/PID decoding tests passed.");
}
