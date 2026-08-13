//
// Created by mattfor on 8/13/26.
//

#include "../../include/utilities/FileOperations.h"

#include <fstream>

std::vector<FileOperations::FileInfo> scanInternal(const std::filesystem::path& directory, const bool recursive)
{
    std::vector<FileOperations::FileInfo> result;

    std::error_code ec;

    if (!std::filesystem::exists(directory, ec) || !std::filesystem::is_directory(directory, ec))
    {
        return result;
    }

    const auto addFile = [&result](const std::filesystem::directory_entry& entry)
    {
        std::error_code fileEc;

        // Follow neither directory nor symlink traversal.
        if (!entry.is_regular_file(fileEc))
        {
            return;
        }

        FileOperations::FileInfo info;
        info.is_regular_file = true;
        info.path            = entry.path();
        info.is_symlink      = entry.is_symlink(fileEc);
        info.size            = entry.file_size(fileEc);

        if (fileEc)
        {
            info.size = 0;
        }

        result.emplace_back(std::move(info));
    };

    if (recursive)
    {
        std::filesystem::recursive_directory_iterator iterator(directory, std::filesystem::directory_options::skip_permission_denied, ec);

        const std::filesystem::recursive_directory_iterator end{};

        while (iterator != end)
        {
            if (!ec)
            {
                addFile(*iterator);
            }

            iterator.increment(ec);
        }
    }
    else
    {
        std::filesystem::directory_iterator iterator(directory, std::filesystem::directory_options::skip_permission_denied, ec);

        const std::filesystem::directory_iterator end{};

        while (iterator != end)
        {
            if (!ec)
            {
                addFile(*iterator);
            }

            iterator.increment(ec);
        }
    }

    return result;
}

std::vector<FileOperations::FileInfo> FileOperations::scan(const std::filesystem::path& directory, const bool recursive)
{
    return scanInternal(directory, recursive);
}

std::vector<FileOperations::FileInfo> FileOperations::scanByExtension(const std::filesystem::path& directory, const std::string& extension, const bool recursive)
{
    auto files = scanInternal(directory, recursive);

    std::string normalizedExtension = extension;

    if (!normalizedExtension.empty() && normalizedExtension.front() != '.')
    {
        normalizedExtension.insert(normalizedExtension.begin(), '.');
    }

    std::erase_if(files, [&normalizedExtension](const FileInfo& file)
    {
        return file.path.extension().string() != normalizedExtension;
    });

    return files;
}

std::vector<FileOperations::FileInfo> FileOperations::findByName(const std::filesystem::path& directory, const std::string& name, const bool recursive)
{
    auto files = scanInternal(directory, recursive);

    std::erase_if(files, [&name](const FileInfo& file)
    {
        return file.path.filename().string().find(name) == std::string::npos;
    });

    return files;
}

bool FileOperations::openFile(const std::filesystem::path& file)
{
    if (!isFile(file))
    {
        return false;
    }

#if defined(_WIN32)
    const std::string command = "start \"\" \"" + file.string() + "\"";

#elif defined(__APPLE__)
    const std::string command = "open \"" + file.string() + "\"";

#elif defined(__linux__)
    const std::string command = "xdg-open \"" + file.string() + "\"";

#else
    return false;

#endif
    return std::system(command.c_str()) == 0;
}

std::optional<std::vector<std::byte>> FileOperations::readBinary(const std::filesystem::path& file)
{
    if (!isFile(file))
    {
        return std::nullopt;
    }

    std::ifstream stream(file, std::ios::binary | std::ios::ate);

    if (!stream)
    {
        return std::nullopt;
    }

    const std::streamsize size = stream.tellg();

    if (size < 0)
    {
        return std::nullopt;
    }

    stream.seekg(0, std::ios::beg);

    std::vector<std::byte> buffer(static_cast<std::size_t>(size));

    if (!buffer.empty())
    {
        stream.read(reinterpret_cast<char*>(buffer.data()), size);

        if (!stream)
        {
            return std::nullopt;
        }
    }

    return buffer;
}

bool FileOperations::isFile(const std::filesystem::path& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec) && std::filesystem::is_regular_file(path, ec);
}

bool FileOperations::isDirectory(const std::filesystem::path& path)
{
    std::error_code ec;
    return std::filesystem::exists(path, ec) && std::filesystem::is_directory(path, ec);
}
