// Copyright (c) 2017-2019 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_FS_H
#define BITCOIN_FS_H

#include <stdio.h>
#include <string>
#if defined WIN32 && defined __GLIBCXX__
#include <ext/stdio_filebuf.h>
#endif

#include <filesystem>
#include <fstream>

/** Filesystem operations and types */
namespace fs {

using namespace std::filesystem;

/**
 * Path class wrapper to prepare application code for transition from
 * boost::filesystem::path to std::filesystem::path.
 *
 * The new std::filesystem::path class lacks imbue functionality boost provided
 * to make implicit path/string functionality work safely on windows, so this
 * class hides the unsafe methods, and provides explicit PathToString /
 * PathFromString functions which be needed after the transition from boost to
 * convert to native path strings, and explicit u8string / u8path functions to
 * convert to UTF-8 strings. See
 * https://github.com/bitcoin/bitcoin/pull/20744#issuecomment-916627496 for more
 * information about the boost path transition and windows encoding ambiguities.
 */
class path : public std::filesystem::path
{
public:
    using std::filesystem::path::path;
    path(std::filesystem::path path) : std::filesystem::path::path(std::move(path)) {}
    path(const std::string& string) = delete;
    path& operator=(std::string&) = delete;
    std::string string() const = delete;
    std::string u8string() const { return std::filesystem::path::string(); }
};

static inline path operator+(path p1, path p2)
{
    p1 += std::move(p2);
    return p1;
}

static inline std::string PathToString(const std::filesystem::path& path)
{
    return path.string();
}

static inline path PathFromString(const std::string& string)
{
    return std::filesystem::path(string);
}

static inline path u8path(const std::string& string)
{
    return std::filesystem::path(string);
}
}

/** Bridge operations to C stdio */
namespace fsbridge {
    FILE *fopen(const fs::path& p, const char *mode);

    /**
     * Helper function for joining two paths
     *
     * @param[in] base  Base path
     * @param[in] path  Path to combine with base
     * @returns path unchanged if it is an absolute path, otherwise returns base joined with path. Returns base unchanged if path is empty.
     * @pre  Base path must be absolute
     * @post Returned path will always be absolute
     */
    fs::path AbsPathJoin(const fs::path& base, const fs::path& path);

    class FileLock
    {
    public:
        FileLock() = delete;
        FileLock(const FileLock&) = delete;
        FileLock(FileLock&&) = delete;
        explicit FileLock(const fs::path& file);
        ~FileLock();
        bool TryLock();
        std::string GetReason() { return reason; }

    private:
        std::string reason;
#ifndef WIN32
        int fd = -1;
#else
        void* hFile = (void*)-1; // INVALID_HANDLE_VALUE
#endif
    };

    std::string get_filesystem_error_message(const fs::filesystem_error& e);

    typedef std::ifstream ifstream;
    typedef std::ofstream ofstream;
};

#endif // BITCOIN_FS_H
