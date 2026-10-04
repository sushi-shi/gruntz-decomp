#include <Io/File.h>
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
    assert(std::remove(path.c_str()) == 0);
    return 0;
}
