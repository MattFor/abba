//
// Created by Grzegorz on 8/16/2026.
//

#ifndef ABBA_TEMPFILE_H
#define ABBA_TEMPFILE_H
#include <filesystem>
#include <fstream>
#include <vector>

class TempFile {
    std::filesystem::path path_;
public:
    explicit TempFile(const std::vector<unsigned char>& bytes)
        : path_(std::filesystem::temp_directory_path() / "abba_test.bin")
    {
        std::ofstream out(path_, std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }
    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove(path_,ec);
    }
    [[nodiscard]] const std::filesystem::path& path() const { return path_; }
    TempFile(const TempFile&)                   = delete;
    TempFile& operator=(const TempFile&)        = delete;
};

#endif //ABBA_TEMPFILE_H
