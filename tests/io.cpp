#include <Io/File.h>
#include <Font/FontData.h>
#include <Io/StreamArchive.h>
#include <cassert>
#include <cstring>
#include <string>
#include <cstdio>
int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string path = argv[1];
    io::File closed;
    assert(!closed.write("x", 1));
    assert(closed.error() == io::NotOpen && !closed.finish());
    io::File file;
    assert(!file.open(path + "/missing", io::ReadOnly));
    assert(file.error() == io::OpenFailed);
    assert(file.open(path, io::Replace));
    assert(file.write("abcd", 4));
    assert(file.size() == 4 && file.position() == 4);
    assert(file.seek(-2, io::End));
    assert(file.write("XY", 2));
    assert(file.finish());
    assert(file.open(path, io::Update));
    assert(file.size() == 4);
    char bytes[8] = {};
    assert(file.read(bytes, 4) == 4 && std::memcmp(bytes, "abXY", 4) == 0);
    assert(file.seek(0, io::End));
    assert(file.write("!", 1));
    assert(file.finish());
    assert(file.open(path, io::ReadOnly));
    assert(file.read(bytes, 8) == 5);
    assert(file.error() == io::ReadFailed);
    assert(!file.seek(0, io::Start));
    assert(!file.finish());
    assert(file.open(path, io::Replace));
    assert(file.size() == 0);
    assert(file.write(NULL, 0));
    assert(file.finish());
    assert(!file.open(std::string("bad\0name", 8), io::Replace));
#ifndef __EMSCRIPTEN__
    io::File full;
    if (full.open("/dev/full", io::Update)) {
        full.write("cannot persist", 14);
        assert(!full.finish());
    }
#endif

    const unsigned char fixture[] = {1,0,0,0, 2,0,0,0, 1,0,0,0, 12,34};
    io::MemoryInput memory(fixture, sizeof(fixture));
    assets::FontData font;
    assert(assets::readFont(memory, font));
    assert(font.glyphs.size() == 1 && font.glyphs[0].width == 2 && font.glyphs[0].height == 1);
    assert(font.glyphs[0].pixels[1] == 34);
    io::MemoryOutput encoded;
    assert(assets::writeFont(encoded, font));
    assert(encoded.bytes().size() == sizeof(fixture));
    assert(std::memcmp(&encoded.bytes()[0], fixture, sizeof(fixture)) == 0);
    assert(file.open(path, io::Replace));
    assert(assets::writeFont(file, font) && file.finish());
    assert(file.open(path, io::ReadOnly));
    assets::FontData diskFont;
    assert(assets::readFont(file, diskFont) && file.finish());
    assert(diskFont.glyphs[0].pixels == font.glyphs[0].pixels);
    for (size_t n = 0; n < sizeof(fixture); ++n) {
        io::MemoryInput truncated(fixture, n);
        assert(!assets::readFont(truncated, font));
        assert(font.glyphs[0].pixels[1] == 34);
    }
    unsigned char oversized[] = {1,0,0,0, 255,255,255,127, 255,255,255,127};
    io::MemoryInput badDimensions(oversized, sizeof(oversized));
    assert(!assets::readFont(badDimensions, font));
    const unsigned char shade[] = {3,0,0,0, 17,29,31};
    io::MemoryInput shadeInput(shade, sizeof(shade));
    std::vector<unsigned char> colors;
    assert(io::readSizedBytes(shadeInput, colors) && colors.size() == 3 && colors[2] == 31);
    io::MemoryOutput shadeOutput;
    assert(io::writeSizedBytes(shadeOutput, &colors[0], colors.size()));
    assert(std::memcmp(&shadeOutput.bytes()[0], shade, sizeof(shade)) == 0);
    io::MemoryInput shortShade(shade, sizeof(shade) - 1);
    assert(!io::readSizedBytes(shortShade, colors) && colors.size() == 3);
    unsigned char owned[] = {7,9};
    io::MemoryInput copied(owned, sizeof(owned));
    owned[0] = 0;
    assert(copied.read(bytes, 2) == 2 && bytes[0] == 7);
    assert(shadeOutput.write(&shadeOutput.bytes()[0], shadeOutput.bytes().size()));
    assert(shadeOutput.bytes().size() == 2 * sizeof(shade));
    io::MemoryOutput snapshot;
    CStreamArchive save(snapshot);
    assert(save.Open() && save.Write(fixture, sizeof(fixture)) && save.Ready());
    save.Close();
    assert(snapshot.good());
    io::MemoryInput saved(&snapshot.bytes()[0], snapshot.bytes().size());
    CStreamArchive restore(saved);
    unsigned char restored[sizeof(fixture)];
    assert(restore.Open() && restore.Read(restored, sizeof(restored)) && restore.Ready());
    assert(std::memcmp(restored, fixture, sizeof(fixture)) == 0);
    assert(!restore.Read(restored, 1) && !restore.Ready());
    assert(std::remove(path.c_str()) == 0);
    return 0;
}
