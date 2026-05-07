#include "decryptor.h"
#include "path_resolver.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace {

struct Stats {
    std::size_t totalEncrypted = 0;
    std::size_t success = 0;
    std::size_t failed = 0;
};

bool ReadBinaryFile(const std::filesystem::path& path, std::vector<std::uint8_t>& data, std::string& error) {
    std::ifstream input(path, std::ios::binary);
    if (!input.is_open()) {
        error = "Failed to open file for reading: " + path.string();
        return false;
    }

    input.seekg(0, std::ios::end);
    const std::streamoff size = input.tellg();
    if (size < 0) {
        error = "Failed to determine file size: " + path.string();
        return false;
    }

    input.seekg(0, std::ios::beg);
    data.resize(static_cast<std::size_t>(size));
    if (size > 0 && !input.read(reinterpret_cast<char*>(data.data()), size)) {
        error = "Failed to read file bytes: " + path.string();
        return false;
    }

    return true;
}

bool WriteBinaryFile(const std::filesystem::path& path, const std::vector<std::uint8_t>& data, std::string& error) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        error = "Failed to open file for writing: " + path.string();
        return false;
    }

    if (!data.empty()) {
        output.write(reinterpret_cast<const char*>(data.data()), static_cast<std::streamsize>(data.size()));
        if (!output.good()) {
            error = "Failed to write file bytes: " + path.string();
            return false;
        }
    }

    return true;
}

int PrintUsageAndExit() {
    std::cerr << "Usage:\n";
    std::cerr << "  rpgmv_decrypt.exe \"<game_root>\" \"<output_root>\"\n\n";
    return 1;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        return PrintUsageAndExit();
    }

    const std::filesystem::path inputRoot = argv[1];
    const std::filesystem::path outputRoot = argv[2];

    std::string error;
    const auto paths = rpgmv::ResolveGamePaths(inputRoot, error);
    if (!paths.has_value()) {
        std::cerr << "Error: " << error << "\n";
        return 2;
    }

    std::array<std::uint8_t, rpgmv::kHeaderLength> keyBytes{};
    if (!rpgmv::ParseEncryptionKeyFromSystemJson(paths->systemJsonPath, keyBytes, error)) {
        std::cerr << "Error: " << error << "\n";
        return 3;
    }

    std::error_code ec;
    if (std::filesystem::exists(outputRoot, ec) && !std::filesystem::is_directory(outputRoot, ec)) {
        std::cerr << "Error: output path exists and is not a directory: " << outputRoot.string() << "\n";
        return 4;
    }
    std::filesystem::create_directories(outputRoot, ec);
    if (ec) {
        std::cerr << "Error: failed to create output directory: " << outputRoot.string() << "\n";
        std::cerr << "System message: " << ec.message() << "\n";
        return 5;
    }

    std::cout << "Input root : " << paths->inputRoot.string() << "\n";
    std::cout << "Game root  : " << paths->gameRoot.string() << "\n";
    std::cout << "System.json: " << paths->systemJsonPath.string() << "\n";
    std::cout << "Scan root  : " << paths->imgRoot.string() << "\n";
    std::cout << "Output root: " << outputRoot.string() << "\n\n";

    Stats stats;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(paths->imgRoot)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const std::filesystem::path sourcePath = entry.path();
        if (!rpgmv::IsEncryptedExtension(sourcePath)) {
            continue;
        }

        ++stats.totalEncrypted;
        std::filesystem::path relativePath;
        relativePath = std::filesystem::relative(sourcePath, paths->imgRoot, ec);
        if (ec) {
            ++stats.failed;
            std::cerr << "[FAIL] " << sourcePath.string() << " | relative-path error: " << ec.message() << "\n";
            ec.clear();
            continue;
        }

        const std::filesystem::path mappedRelativePath = rpgmv::MapDecryptedRelativePath(relativePath);
        const std::filesystem::path outputPath = outputRoot / mappedRelativePath;

        std::filesystem::create_directories(outputPath.parent_path(), ec);
        if (ec) {
            ++stats.failed;
            std::cerr << "[FAIL] " << sourcePath.string() << " | mkdir error: " << ec.message() << "\n";
            ec.clear();
            continue;
        }

        std::vector<std::uint8_t> encryptedData;
        if (!ReadBinaryFile(sourcePath, encryptedData, error)) {
            ++stats.failed;
            std::cerr << "[FAIL] " << sourcePath.string() << " | " << error << "\n";
            continue;
        }

        std::vector<std::uint8_t> decryptedData;
        if (!rpgmv::DecryptRpgmData(encryptedData, keyBytes, decryptedData, error)) {
            ++stats.failed;
            std::cerr << "[FAIL] " << sourcePath.string() << " | " << error << "\n";
            continue;
        }

        if (!WriteBinaryFile(outputPath, decryptedData, error)) {
            ++stats.failed;
            std::cerr << "[FAIL] " << sourcePath.string() << " | " << error << "\n";
            continue;
        }

        ++stats.success;
        std::cout << "[ OK ] " << sourcePath.string() << " -> " << outputPath.string() << "\n";
    }

    std::cout << "\nDone.\n";
    std::cout << "Encrypted files found: " << stats.totalEncrypted << "\n";
    std::cout << "Success: " << stats.success << "\n";
    std::cout << "Failed : " << stats.failed << "\n";

    if (stats.totalEncrypted == 0) {
        std::cout << "No .rpgmvp/.rpgmvo/.rpgmvm files found under img directory.\n";
    }

    return (stats.failed == 0) ? 0 : 6;
}
