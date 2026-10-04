#include <Io/File.h>
#include <Io/Settings.h>
#include <Font/FontData.h>
#include <Io/StreamArchive.h>
#include <cassert>
#include <cstring>
#include <string>
#include <cstdio>
#include <unistd.h>
int main(int argc, char** argv) {
    assert(argc == 2);
#ifdef __EMSCRIPTEN__
    static_assert(sizeof(void*) == 4, "This suite must exercise wasm32");
#endif
    const std::string path = argv[1];
    io::File closed;
    assert(!closed.write("x", 1));
    assert(closed.error() == io::NotOpen && !closed.finish());
    io::File file;
    assert(!file.open(path + "/missing", io::ReadOnly));
    assert(file.error() == io::NotFound);
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

    const std::string configPath = path + ".cfg";
    Settings settings;
    assert(settings.load(configPath));
    assert(settings.loaded() && settings.getInt("Sound", 7) == 7);
    settings.setInt("Sound", 0);
    settings.setInt("Negative", -2147483647 - 1);
    settings.setInt("Maximum", 2147483647);
    settings.setString("Player Name", "a=b%\n\xC3\xA9");
    assert(settings.getInt("SOUND", 7) == 0);
    assert(settings.getInt("Player Name", 12) == 12);
    assert(settings.getString("Sound", "typed") == "typed");
    assert(settings.save());
    Settings loaded;
    assert(loaded.load(configPath));
    assert(loaded.getString("player name") == "a=b%\n\xC3\xA9");
    assert(loaded.getInt("negative") == (-2147483647 - 1));
    assert(loaded.getInt("maximum") == 2147483647);
    loaded.setInt("sound", 1);
    assert(loaded.save());
    assert(settings.load(configPath) && settings.getInt("Sound") == 1);
    io::MemoryOutput configBytes;
    assert(settings.encode(configBytes));
    io::MemoryInput configInput(&configBytes.bytes()[0], configBytes.bytes().size());
    Settings memorySettings;
    assert(memorySettings.decode(configInput) && memorySettings.getInt("Sound") == 1);
    const char crlf[] = "GRUNTZ CONFIG 1\r\ni sound=1\r\n";
    io::MemoryInput crlfInput(crlf, sizeof(crlf) - 1);
    assert(memorySettings.decode(crlfInput) && memorySettings.getInt("sound") == 1);
    const char* invalidConfigs[] = {
        "", "GRUNTZ CONFIG 2\n", "GRUNTZ CONFIG 1\ni a=2147483648\n",
        "GRUNTZ CONFIG 1\ns a=%XX\n", "GRUNTZ CONFIG 1\ns a=%00\n",
        "GRUNTZ CONFIG 1\ni A=1\ni a=2\n", "GRUNTZ CONFIG 1\nx a=b\n"
    };
    for (size_t index = 0; index < sizeof(invalidConfigs) / sizeof(invalidConfigs[0]); ++index) {
        io::MemoryInput invalid(invalidConfigs[index], std::strlen(invalidConfigs[index]));
        assert(!memorySettings.decode(invalid));
        assert(memorySettings.getInt("sound") == 1);
    }
    settings.setString("bad", std::string("nul\0value", 9));
    assert(!settings.save());
    assert(loaded.load(configPath) && loaded.getInt("sound") == 1);
    Settings unavailable;
    assert(unavailable.load(path + "-missing/config"));
    assert(!unavailable.save());

    // Resolve a relative settings path once, then change the working directory.
    char previousDirectory[4096];
    assert(getcwd(previousDirectory, sizeof(previousDirectory)));
#ifdef __EMSCRIPTEN__
    const std::string directory = "/tmp/";
#else
    const std::string directory = path.substr(0, path.find_last_of('/') + 1);
#endif
    assert(chdir(directory.c_str()) == 0);
    Settings relativeSettings;
    assert(relativeSettings.load("relative.cfg"));
    assert(chdir(previousDirectory) == 0);
    relativeSettings.setInt("Sound", 1);
    assert(relativeSettings.save());
    Settings relativeReload;
    assert(relativeReload.load(directory + "relative.cfg") && relativeReload.getInt("sound") == 1);
    assert(std::remove((directory + "relative.cfg").c_str()) == 0);
    const unsigned char integers[] = {0,0,0,0, 255,255,255,127, 0,0,0,128, 255,255,255,255};
    io::MemoryInput integerInput(integers, sizeof(integers));
    io::BinaryReader numbers(integerInput);
    unsigned int number = 1;
    assert(numbers.u32(number) && number == 0);
    assert(numbers.u32(number) && number == 0x7fffffffU);
    assert(numbers.u32(number) && number == 0x80000000U);
    assert(numbers.u32(number) && number == 0xffffffffU);
    assert(!numbers.u32(number) && number == 0xffffffffU);
    io::MemoryInput invalidMemory(NULL, 1);
    assert(!invalidMemory.good());
    io::MemoryInput empty(NULL, 0);
    io::BinaryReader emptyReader(empty);
    assert(emptyReader.bytes(NULL, 0));
    io::File missing;
    assert(!missing.open(static_cast<const char*>(NULL), io::ReadOnly));
    io::MemoryOutput invalidOutput;
    assert(!invalidOutput.write(NULL, 1) && !invalidOutput.good());
    io::MemoryOutput rejectedSnapshot;
    CStreamArchive invalidArchive(rejectedSnapshot);
    assert(invalidArchive.Open());
    assert(!invalidArchive.Write(integers, -1) && !invalidArchive.Ready());
    assert(file.open(path, io::Replace));
    assert(io::writeSizedBytes(file, &colors[0], colors.size()) && file.finish());
    assert(file.open(path, io::ReadOnly));
    std::vector<unsigned char> fileColors;
    assert(io::readSizedBytes(file, fileColors) && fileColors == colors && file.finish());
    assert(std::remove(configPath.c_str()) == 0);
    assert(std::remove(path.c_str()) == 0);
    std::printf("I/O, codecs, snapshots and settings passed (%u-bit pointers).\n",
                static_cast<unsigned int>(sizeof(void*) * 8));
    return 0;
}
