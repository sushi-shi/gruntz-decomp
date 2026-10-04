#include <StdAfx.h>
#include <Rez/RezArchive.h>
#include <Rez/RezArchiveEntry.h>
#include <Io/File.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

static void word(std::vector<unsigned char>& bytes, size_t offset, unsigned int value) {
    for (int i = 0; i < 4; ++i) bytes[offset + i] = static_cast<unsigned char>(value >> (8 * i));
}
static void resource(std::vector<unsigned char>& bytes, unsigned int offset, const char* name) {
    size_t at = bytes.size(); bytes.resize(at + 28);
    word(bytes, at, 0); word(bytes, at + 4, offset); word(bytes, at + 8, 4);
    word(bytes, at + 20, 0x504944);
    for (size_t i = 0; i <= strlen(name); ++i) bytes.push_back(static_cast<unsigned char>(name[i]));
    bytes.push_back(0);
}
static std::vector<unsigned char> fixture() {
    std::vector<unsigned char> bytes(190, 0);
    bytes[0] = bytes[62] = bytes[124] = 13;
    bytes[1] = bytes[63] = bytes[125] = 10; bytes[126] = 26; bytes[167] = 1;
    word(bytes, 127, 1); word(bytes, 131, 190);
    memcpy(&bytes[168], "AAAA", 4); memcpy(&bytes[181], "BBBB", 4);
    resource(bytes, 168, "A"); resource(bytes, 181, "B");
    word(bytes, 135, bytes.size() - 190);
    return bytes;
}
static void save(const std::string& path, const std::vector<unsigned char>& bytes) {
    io::File file;
    assert(file.open(path, io::Replace));
    assert(file.write(&bytes[0], bytes.size()) && file.finish());
}
static void check(CRezItm* item, const char* value) {
    assert(item && item->GetSize() == 4);
    char bytes[4]; assert(item->Get(bytes) && memcmp(bytes, value, 4) == 0);
    assert(!item->Get(bytes, 3, 2));
    assert(!item->Get(bytes, 0xffffffffU, 2));
    assert(!item->Get(static_cast<void*>(NULL), 0, 1));
    assert(item->Get(static_cast<void*>(NULL), 4, 0));
    assert(item->Seek(0));
    assert(item->Read(bytes, 0xffffffffU) == 4 && memcmp(bytes, value, 4) == 0);
    assert(item->Read(bytes, 1) == 0 && item->GetChar() == 0);
    assert(!item->Seek(5) && item->GetSeekPos() == 4);
    assert(item->Read(bytes, 1, 5) == 0 && item->GetSeekPos() == 4);
}
int main(int argc, char** argv) {
    assert(argc == 2);
    const std::string path(argv[1]);
    const std::string extra = path + ".extra";
    const std::string bad = path + ".bad";
    std::vector<unsigned char> original = fixture();
    save(path, original);
    std::vector<unsigned char> replacement = original;
    memcpy(&replacement[168], "CCCC", 4); save(extra, replacement);
    word(replacement, 135, 0xffffffffU); save(bad, replacement);
    {
        CRezMgr archive(path.c_str());
        assert(archive.IsOpen());
        const RezTypeTag tag = archive.StrToType("PID");
        CRezDir* root = archive.GetRootDir();
        CRezItm* first = root->GetRez("A", tag);
        check(first, "AAAA");
        assert(root->Load()); // Resource data has gaps; sum-of-sizes caching was unsafe.
        check(root->GetRez("B", tag), "BBBB");
        assert(root->IsLoaded()); first->UnLoad(); assert(!root->IsLoaded());
        assert(root->Load());
        assert(!archive.OpenAdditional(bad.c_str(), true));
        assert(archive.GetRootDir() == root && root->GetRez("A", tag) == first);
        check(first, "AAAA");
        assert(!archive.Open(bad.c_str()));
        check(first, "AAAA");
        assert(archive.OpenAdditional(extra.c_str(), false)); check(first, "AAAA");
        for (int iteration = 0; iteration < 250; ++iteration) {
            assert(archive.OpenAdditional(extra.c_str(), true));
            check(root->GetRez("A", tag), "CCCC");
        }
        assert(root->Load() && root->IsLoaded());
        assert(root->UnLoad() && !root->IsLoaded());
        check(root->GetRez("B", tag), "BBBB");
        assert(archive.Reset());
        check(archive.GetRootDir()->GetRez("A", tag), "AAAA");
        assert(archive.Close() && !archive.IsOpen());
        assert(archive.Close());
        assert(archive.Open(path.c_str()));
    }
    {
        CRezMgr rejected(bad.c_str());
        assert(!rejected.IsOpen() && !rejected.GetRootDir());
        assert(rejected.Close());
    }
    assert(remove(path.c_str()) == 0 && remove(extra.c_str()) == 0 && remove(bad.c_str()) == 0);
    puts("REZ production import, replacement, cache, ranges and lifecycle tests passed.");
    return 0;
}
