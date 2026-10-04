#include <Io/FileTransaction.h>
#include <Io/SavePaths.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include <iterator>
#include <sys/stat.h>
#include <unistd.h>
#ifndef __EMSCRIPTEN__
#include <sys/resource.h>
#include <signal.h>
#endif

static std::string readFile(const std::string& path) {
    std::ifstream file(path.c_str(), std::ios::binary);
    assert(file.good());
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}
static void writeFile(const std::string& path, const std::string& bytes) {
    std::ofstream file(path.c_str(), std::ios::binary);
    file.write(bytes.data(), bytes.size());
    file.close();
    assert(file.good());
}
static bool exists(const std::string& path) {
    struct stat info;
    return stat(path.c_str(), &info) == 0;
}

void checkSaveTransactions(const std::string& path) {
    writeFile(path, "previous");
    std::string staged;
    {
        io::FileTransaction file(path);
        assert(file.good());
        staged = file.uniquePath();
        assert(staged != path && exists(staged));
        assert(file.write("replacement", 11) && file.finish());
        assert(readFile(path) == "previous");
        assert(readFile(staged) == "replacement");
        assert(file.commit());
        assert(!file.commit());
    }
    assert(!exists(staged) && readFile(path) == "replacement");
    {
        io::FileTransaction abandoned(path);
        staged = abandoned.uniquePath();
        assert(abandoned.write("incomplete", 10));
    }
    assert(!exists(staged) && readFile(path) == "replacement");
    {
        io::FileTransaction failed(path);
        staged = failed.uniquePath();
        assert(!failed.write(NULL, 1) && !failed.good());
        assert(!failed.finish() && !failed.commit());
    }
    assert(!exists(staged) && readFile(path) == "replacement");
    {
        io::FileTransaction finished(path);
        staged = finished.uniquePath();
        assert(finished.finish());
        assert(!finished.write("late", 4) && !finished.commit());
    }
    assert(!exists(staged) && readFile(path) == "replacement");

    writeFile(path + ".stage-0", "reserved");
    {
        io::FileTransaction first(path);
        io::FileTransaction second(path);
        assert(first.good() && second.good());
        assert(first.uniquePath() != second.uniquePath());
        assert(first.uniquePath() != path + ".stage-0");
        assert(second.uniquePath() != path + ".stage-0");
    }
    assert(readFile(path + ".stage-0") == "reserved");
    assert(std::remove((path + ".stage-0").c_str()) == 0);

    const std::string directory = path + "-directory";
    assert(mkdir(directory.c_str(), 0700) == 0);
    writeFile(directory + "/keep", "untouched");
    {
        io::FileTransaction denied(directory);
        staged = denied.uniquePath();
        assert(denied.write("new", 3) && denied.finish());
        assert(!denied.commit());
    }
    assert(!exists(staged) && readFile(directory + "/keep") == "untouched");
    assert(std::remove((directory + "/keep").c_str()) == 0);
    assert(rmdir(directory.c_str()) == 0);
    {
        io::FileTransaction missing(directory + "/missing");
        assert(!missing.good() && !missing.commit());
    }

    // Model publication ordering: an existing manifest refers to an old snapshot.
    const std::string oldSnapshot = path + "-old";
    writeFile(oldSnapshot, "old-state|old-preview");
    writeFile(path, oldSnapshot);
    {
        io::FileTransaction snapshot(path + "-snapshot");
        staged = snapshot.uniquePath();
        assert(snapshot.write("new-state|new-preview", 21) && snapshot.finish());
        io::FileTransaction progress(path);
        assert(!progress.write(NULL, 1));
        assert(!progress.commitReferencing(snapshot));
    }
    assert(!exists(staged));
    assert(readFile(path) == oldSnapshot && readFile(readFile(path)) == "old-state|old-preview");
#ifndef __EMSCRIPTEN__
    // Force a real delayed stdio write/close failure without replacing our file API.
    struct rlimit previousLimit;
    assert(getrlimit(RLIMIT_FSIZE, &previousLimit) == 0);
    struct rlimit noWrites = previousLimit;
    noWrites.rlim_cur = 0;
    void (*previousSignal)(int) = signal(SIGXFSZ, SIG_IGN);
    assert(previousSignal != SIG_ERR);
    {
        io::FileTransaction snapshot(path + "-snapshot");
        staged = snapshot.uniquePath();
        assert(snapshot.write("new-state|new-preview", 21) && snapshot.finish());
        io::FileTransaction progress(path);
        assert(progress.good());
        assert(setrlimit(RLIMIT_FSIZE, &noWrites) == 0);
        progress.write(staged.data(), staged.size());
        assert(!progress.commitReferencing(snapshot));
        assert(setrlimit(RLIMIT_FSIZE, &previousLimit) == 0);
    }
    {
        io::FileTransaction snapshot(path + "-snapshot");
        staged = snapshot.uniquePath();
        io::FileTransaction progress(path);
        assert(progress.write(staged.data(), staged.size()) && progress.finish());
        assert(setrlimit(RLIMIT_FSIZE, &noWrites) == 0);
        snapshot.write("new-state|new-preview", 21);
        assert(!progress.commitReferencing(snapshot));
        assert(setrlimit(RLIMIT_FSIZE, &previousLimit) == 0);
    }
    assert(signal(SIGXFSZ, previousSignal) != SIG_ERR);
    assert(!exists(staged));
    assert(readFile(path) == oldSnapshot && readFile(readFile(path)) == "old-state|old-preview");
#endif
    {
        io::FileTransaction snapshot(path + "-snapshot");
        staged = snapshot.uniquePath();
        assert(snapshot.write("new-state|", 10) && snapshot.write("new-preview", 11));
        assert(snapshot.finish());
        io::FileTransaction progress(path);
        assert(progress.write(staged.data(), staged.size()) && progress.commitReferencing(snapshot));
        assert(!snapshot.good());
    }
    assert(readFile(path) == staged && readFile(readFile(path)) == "new-state|new-preview");
    assert(readFile(oldSnapshot) == "old-state|old-preview");
    assert(std::remove(staged.c_str()) == 0);
    assert(std::remove(oldSnapshot.c_str()) == 0);

    {
        io::FileTransaction snapshot(path + "-snapshot");
        staged = snapshot.uniquePath();
        assert(snapshot.write("snapshot", 8) && snapshot.finish());
        assert(!snapshot.commitReferencing(snapshot));
        io::FileTransaction aliasesSnapshot(staged);
        assert(aliasesSnapshot.write("metadata", 8));
        assert(!aliasesSnapshot.commitReferencing(snapshot));
        assert(readFile(staged) == "snapshot");
    }
    assert(!exists(staged));

    // A deleted or missing snapshot's filename can be allocated again. Cleanup
    // must preserve that new file even when the old record uses Windows paths.
    assert(mkdir(directory.c_str(), 0700) == 0);
    const char previousName[] = "C:\\Gruntz\\Save\\Slot1.sav.stage-0";
    const std::string slotBase = directory + "/Slot1.sav";
    for (int scenario = 0; scenario < 2; ++scenario) {
        if (scenario == 0) {
            writeFile(slotBase + ".stage-0", "deleted");
            assert(std::remove((slotBase + ".stage-0").c_str()) == 0);
        }
        io::FileTransaction snapshot(slotBase);
        staged = snapshot.uniquePath();
        assert(staged == slotBase + ".stage-0");
        assert(snapshot.write("new-state|new-preview", 21));
        io::FileTransaction progress(path);
        assert(progress.write(staged.data(), staged.size()) && progress.commitReferencing(snapshot));
        std::string name;
        assert(io::snapshotName(0, staged.c_str(), staged.size() + 1, name));
        io::removePreviousSnapshot(0, directory, previousName, sizeof(previousName), name);
        assert(readFile(readFile(path)) == "new-state|new-preview");
        writeFile(slotBase, "obsolete");
        const char legacyName[] = "C:\\Gruntz\\Save\\Slot1.sav";
        io::removePreviousSnapshot(0, directory, legacyName, sizeof(legacyName), name);
        assert(!exists(slotBase) && exists(staged));
        assert(std::remove(staged.c_str()) == 0);
    }
    assert(rmdir(directory.c_str()) == 0);

    char cwd[4096];
    assert(getcwd(cwd, sizeof(cwd)));
    assert(mkdir(directory.c_str(), 0700) == 0);
    assert(chdir(directory.c_str()) == 0);
    {
        io::FileTransaction relative("saved");
        assert(chdir(cwd) == 0);
        assert(relative.write("stable", 6) && relative.commit());
    }
    assert(readFile(directory + "/saved") == "stable");
    assert(std::remove((directory + "/saved").c_str()) == 0);
    assert(rmdir(directory.c_str()) == 0);

    std::string name = "unchanged";
    assert(io::slotBaseName(0) == "Slot1.sav" && io::slotBaseName(9) == "Slot10.sav");
    assert(io::slotBaseName(10).empty());
    const char* valid[] = {"Slot1.sav", "C:\\GRUNTZ\\SAVE\\Slot1.sav", "/long/location/Slot1.sav.stage-1023"};
    for (size_t i = 0; i < sizeof(valid) / sizeof(valid[0]); ++i) {
        assert(io::snapshotName(0, valid[i], std::strlen(valid[i]) + 1, name));
        assert(name.find('/') == std::string::npos && name.find('\\') == std::string::npos);
    }
    const char* invalid[] = {"", "Slot2.sav", "Slot1.sav.stage-1024", "Slot1.sav.stage-", "Slot1.sav.stage-x", "../Gruntz.sav", "Slot1.sav/other"};
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i) {
        name = "unchanged";
        assert(!io::snapshotName(0, invalid[i], std::strlen(invalid[i]) + 1, name));
        assert(name == "unchanged");
    }
    const char unterminated[] = {'S','l','o','t','1','.','s','a','v'};
    assert(!io::snapshotName(0, unterminated, sizeof(unterminated), name));
    assert(!io::snapshotName(0, NULL, 10, name));
    assert(std::remove(path.c_str()) == 0);
}
