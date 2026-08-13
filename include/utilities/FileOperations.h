//
// Created by mattfor on 8/13/26.
//

#ifndef ABBA_FILEOPERATIONS_H
#define ABBA_FILEOPERATIONS_H

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

class FileOperations
{
public:
    struct FileInfo
    {
        bool is_symlink{};
        bool is_regular_file{};

        std::uintmax_t        size{};
        std::filesystem::path path;
    };

    /**
     * Scan a directory for files.
     *
     * @param directory Directory to scan.
     * @param recursive Whether subdirectories should also be scanned.
     *
     * @return A vector of discovered files.
     */
    [[nodiscard]] static std::vector<FileInfo> scan(const std::filesystem::path& directory, bool recursive = true);

    /**
     * Scan a directory and only return files with the specified extension.
     *
     * Example: scanByExtension("/tmp", ".exe");
     */
    [[nodiscard]] static std::vector<FileInfo> scanByExtension(const std::filesystem::path& directory, const std::string& extension, bool recursive = true);

    /**
     * Find files whose filename contains a substring, case-sensitive.
     */
    [[nodiscard]] static std::vector<FileInfo> findByName(const std::filesystem::path& directory, const std::string& name, bool recursive = true);

    /**
     * Open a file using the operating system's default application.
     *
     * Windows: start
     * Linux:   xdg-open
     * macOS:   open
     *
     * @return true if the open command was successfully launched.
     */
    [[nodiscard]] static bool openFile(const std::filesystem::path& file);

    /**
     * Read an entire file as binary data.
     */
    [[nodiscard]] static std::optional<std::vector<std::byte>> readBinary(const std::filesystem::path& file);

    /**
     * Check whether a path exists and is a regular file.
     */
    [[nodiscard]] static bool isFile(const std::filesystem::path& path);

    /**
     * Check whether a path exists and is a directory.
     */
    [[nodiscard]] static bool isDirectory(const std::filesystem::path& path);
};

#endif // ABBA_FILEOPERATIONS_H
