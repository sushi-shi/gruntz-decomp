#ifndef GRUNTZ_IO_FILETRANSACTION_H
#define GRUNTZ_IO_FILETRANSACTION_H
#include <Io/File.h>

namespace io {
// Owns an exclusively created sibling until successful publication. Destruction
// removes abandoned output. The destination is resolved before any writes.
class FileTransaction : public Output {
public:
    explicit FileTransaction(const std::string& destination);
    virtual ~FileTransaction();
    virtual bool write(const void* data, size_t count);
    virtual bool good() const { return m_good && !m_committed; }
    bool finish();
    bool commit();
    // Publish this manifest only after its referenced unique file has closed.
    // On failure both staged files remain owned and are removed by destruction.
    bool commitReferencing(FileTransaction& dependency);
    const std::string& uniquePath() const { return m_temporary; }
private:
    bool commitUnique();
    FileTransaction(const FileTransaction&);
    FileTransaction& operator=(const FileTransaction&);
    File m_file;
    std::string m_destination;
    std::string m_temporary;
    bool m_good;
    bool m_finished;
    bool m_committed;
};
}
#endif
